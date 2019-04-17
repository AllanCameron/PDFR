//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR letter_grouper implementation file                                  //
//                                                                           //
//  Copyright (C) 2018 by Allan Cameron                                      //
//                                                                           //
//  Permission is hereby granted, free of charge, to any person obtaining    //
//  a copy of this software and associated documentation files               //
//  (the "Software"), to deal in the Software without restriction, including //
//  without limitation the rights to use, copy, modify, merge, publish,      //
//  distribute, sublicense, and/or sell copies of the Software, and to       //
//  permit persons to whom the Software is furnished to do so, subject to    //
//  the following conditions:                                                //
//                                                                           //
//  The above copyright notice and this permission notice shall be included  //
//  in all copies or substantial portions of the Software.                   //
//                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  //
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               //
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   //
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY     //
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,     //
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        //
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   //
//                                                                           //
//---------------------------------------------------------------------------//

#include "letter_grouper.h"
#include <cstdlib>   // for abs()

using namespace std;

//---------------------------------------------------------------------------//
// Output words without first joining them into lines

GSoutput letter_grouper::out()
{

  std::vector<float> left, right, bottom, size;
  std::vector<std::string> font;
  std::vector<std::vector<Unicode>> text;
  for(auto& cell : m_grid)
  {
    // for each cell in the grid...
    for(auto& element : cell.second)
    {
      if(!element->is_consumed())
      {
        // Copy its contents over
        text.push_back(element->glyph);
        left.push_back(element->left);
        right.push_back(element->right);
        size.push_back(element->size);
        bottom.push_back(element->bottom);
        font.push_back(element->font);
      }
    }
  }
  return GSoutput{move(text), move(left), move(bottom), move(right),
                  move(font), move(size), move(minbox)};
}

//---------------------------------------------------------------------------//
// The letter_grouper constructor calls three subroutines. These split the
// page into an easily addressable 16 x 16 grid, find glyphs in close proximity
// to each other, and glue them together, respectively.

letter_grouper::letter_grouper(textrows&& GS) :
  gslist(move(GS)), minbox(gslist.minbox)
{
  makegrid();     // Split the glyphs into 256 cells to reduce search space
  compareCells(); // Find adjacent glyphs
  merge();        // Glue adjacent glyphs together
}

//---------------------------------------------------------------------------//
// This method creates a 16 x 16 grid of equally-sized bins across the page and
// places each textrow from the parser into a vector in each bin. The reason for
// doing this is to narrow the search space of potentially adjoining glyphs. The
// naive method would involve comparing the right and bottom edge of every glyph
// to every other glyph. By putting the glyphs into bins, we only need to
// compare the right edge of each glyph with glyphs in the same bin or the bin
// immediately to the right. For completeness, to capture letters with low
// descenders, subscript and superscript letters, we also check the two cells
// above and two cells below each character. This still leaves us with a search
// space of approximately 6/256 * n^2 as opposed to n^2, which is 40 times
// smaller. The grid position is easily stored as a single byte, with the first
// 4 bits representing the x position and the second representing the y.

void letter_grouper::makegrid()
{
  float dx = (minbox[2] - minbox[0]) / 16; // Grid column width in user space
  float dy = (minbox[3] - minbox[1]) / 16; // Grid row height in user space

  // For each glyph
  for(auto& element : gslist)
  {
    // Calculate the row and column number the glyph's bottom left corner is in
    // There will be exactly 16 rows and columns, each numbered 0-15 (4 bits)
    uint8_t column = (element->left - minbox[0]) / dx;
    uint8_t row = 15 - (element->bottom - minbox[1]) / dy;

    // Convert the two 4-bit row and column numbers to a single byte
    uint8_t index = (row << 4) | column;

    // Append a pointer to the glyph's textrow to the vector in that cell
    m_grid[index].push_back(element);
  }
}

//---------------------------------------------------------------------------//
// Allows the main data object to be output after calculations done

textrows letter_grouper::output()
{
  // This lambda is used to find text_ptrs that aren't flagged for deletion
  auto extant = [&](const text_ptr& elem) -> bool
                        {return !(elem->is_consumed());};

  // This lambda defines a text_ptr sort from left to right
  auto left_sort = [](const text_ptr& a, const text_ptr& b) -> bool
                     { return a->left < b->left;};

  // Now copy all the text_ptrs from the grid to a vector
  vector<text_ptr> v;
  for(auto& cell : m_grid)
  {
    copy_if(cell.second.begin(), cell.second.end(), back_inserter(v), extant);
  }

  // Sort left to right
  sort(v.begin(), v.end(), left_sort);

  return textrows(v, minbox);

}

//---------------------------------------------------------------------------//
// This method co-ordinates the proximity matching of individual glyphs to
// stick them together into words. It does so by taking each glyph and comparing
// its left side, right side and bottom edge against other glyphs. Rather than
// doing this for every other glyph on the page, it only compares each glyph to
// nearby glyphs. These are those found in the same cell and the cells to the
// North and South. If there is no match found in these it will also look in
// the cells at the NorthEast, East and SouthEast.

void letter_grouper::compareCells()
{
  // For each of 16 columns of cells on page
  for(uint8_t i = 0; i < 16; ++i)
  {
    // For each cell in the column
    for(uint8_t j = 0; j < 16; ++j)
    {
      uint8_t key = i | (j << 4);   // Get the 1-byte address of the cell

      // Get a reference to the cell's contents
      vector<text_ptr>& maingroup = m_grid[key];
      if(maingroup.empty()) continue;  // empty cell - nothing to be done
      for(auto& k : maingroup) // for each glyph in the cell
      {
        // Check for matches in this cell
        matchRight(k, key);
        if(j < 15) matchRight(k, i | ((j + 1) << 4)); // and cell to North
        if(j > 0)  matchRight(k, i | ((j - 1) << 4)); // and cell to South
        if(k->r_join.first != -1) continue; // If match, look no further

        if(i < 15)
        {
          matchRight(k, (i + 1) | (j << 4)); // else check to the East,
          if(j < 15) matchRight(k, (i + 1) | ((j + 1) << 4)); // NE,
          if(j > 0)  matchRight(k, (i + 1) | ((j - 1) << 4)); // and SE
        }
      }
    }
  }
}

//---------------------------------------------------------------------------//
// This is the algorithm that finds adjoining glyphs. Each glyph is addressable
// by its cell and the order it appears in the vector of glyphs contained in
// that cell. The glyphs are called textrows here because each glyph forms a row
// of output from the parser class

void letter_grouper::matchRight(text_ptr row, uint8_t key)
{
  // The key is the address of the cell in the m_grid.
  auto& cell = m_grid[key];

  // some cells are empty - nothing to do
  if(cell.empty()) return;

  // For each glyph in the cell
  for(uint16_t i = 0; i < cell.size(); ++i)
  {
    // If in good position to be next glyph
    if(row->is_adjoining_letter(*(cell[i])))
    {
      // Consume if identical. Skip if already consumed
      if(*cell[i] == *row) row->consume();
      if(cell[i]->is_consumed()) continue; // ignore if marked for deletion

      if(row->r_join.first == -1)
      {
        row->r_join = {key, i};
        continue; // don't bother checking next statement
      }

      // If already a match but this one is better...
      if(m_grid[row->r_join.first][row->r_join.second]->left > cell[i]->left)
      {
        row->r_join = {key, i};
      }
    }
  }
}

//---------------------------------------------------------------------------//
// Takes each glyph and sticks it onto any right-adjoining glyph, updating the
// the latter's size and position parameters and declaring the leftward glyph
// "consumed"

void letter_grouper::merge()
{
  // For each column in the x-axis
  for(uint8_t i = 0; i < 16; ++i)
  {
    // For each cell in that column
    for(uint8_t j = 0; j < 16; ++j)
    {
      // Get the cell's contents
      vector<text_ptr>& cell = m_grid[i | (j << 4)];

      // If the cell is empty there's nothing to do
      if(cell.empty()) continue;

      // For each glyph in the cell
      for(auto& element : cell)
      {
        // If glyph is viable and matches another glyph to the right
        if(element->is_consumed() || element->r_join.first == -1) continue;

        // Look up the right-matching glyph
        auto& matcher = m_grid[element->r_join.first][element->r_join.second];

        // Use the textrow member function to merge the two glyphs
        element->merge_letters(*matcher);
      }
    }
  }
}
