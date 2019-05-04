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

TextTable letter_grouper::out()
{
  return TextTable(m_text_box);
}

//---------------------------------------------------------------------------//
// The letter_grouper constructor calls three subroutines. These split the
// page into an easily addressable 16 x 16 grid, find glyphs in close proximity
// to each other, and glue them together, respectively.

letter_grouper::letter_grouper(TextBox text_box) : m_text_box(move(text_box))
{
  m_text_box.remove_duplicates();
  makegrid();     // Split the glyphs into 256 cells to reduce search space
  compareCells(); // Find adjacent glyphs
  merge();        // Glue adjacent glyphs together
}

//---------------------------------------------------------------------------//
// This method creates a 16 x 16 grid of equally-sized bins across the page and
// places each TextElement from the parser into a vector in each bin. The
// reason for doing this is speed up the search of potentially adjoining glyphs.
// The naive method would compare the right and bottom edge of every glyph
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
  // Grid column width in user space
  float dx = (m_text_box.width()) / 16;

  // Grid row height in user space
  float dy = (m_text_box.height()) / 16;

  // For each glyph
  for(auto& element : m_text_box)
  {
    // Calculate the row and column number the glyph's bottom left corner is in
    // There will be exactly 16 rows and columns, each numbered 0-15 (4 bits)
    uint8_t column = (element->get_left() - m_text_box.get_left()) / dx;
    uint8_t row = 15 - (element->get_bottom() - m_text_box.get_bottom()) / dy;

    // Convert the two 4-bit row and column numbers to a single byte
    uint8_t index = (row << 4) | column;

    // Append a pointer to the glyph's TextElement to the vector in that cell
    m_grid[index].push_back(element);
  }
}

//---------------------------------------------------------------------------//
// Allows the main data object to be output after calculations done

TextBox letter_grouper::output()
{
  // This lambda is used to find text_ptrs that aren't flagged for deletion
  auto extant = [&](const TextPointer& elem) -> bool
                        {return !(elem->is_consumed());};

  // This lambda defines a TextPointer sort from left to right
  auto left_sort = [](const TextPointer& a, const TextPointer& b) -> bool
                     { return a->get_left() < b->get_left();};

  // Now copy all the text_ptrs from the grid to a vector
  vector<TextPointer> v;
  for(auto& cell : m_grid)
  {
    copy_if(cell.second.begin(), cell.second.end(), back_inserter(v), extant);
  }

  // Sort left to right
  sort(v.begin(), v.end(), left_sort);

  m_text_box.swap_data(v);

  return m_text_box;

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
  for(uint8_t column = 0; column < 16; ++column)
  {
    // For each row in the column
    for(uint8_t row = 0; row < 16; ++row)
    {
      // Get the 1-byte address of the cell
      uint8_t key = column | (row << 4);

      // Get a reference to the cell's contents
      vector<TextPointer>& maingroup = m_grid[key];

      // Empty cell - nothing to be done
      if(maingroup.empty()) continue;

      // For each glyph in the cell
      for(auto& element : maingroup)
      {
        for(int index = 0; index < 6; ++index)
        {
          if(column == 15 &&  index > 2) break;
          if(element->has_join() && index == 3 ) break;
          if(row == 0 && (index % 3 == 2)) continue;
          if(row == 15 && (index % 3 == 1)) continue;

          uint8_t cell = (column + (index / 3)) | ((row + index % 3 - 1) << 4);
          matchRight(element, cell);
        }
      }
    }
  }
}

//---------------------------------------------------------------------------//
// This is the algorithm that finds adjoining glyphs. Each glyph is addressable
// by its cell and the order it appears in the vector of glyphs contained in
// that cell.

void letter_grouper::matchRight(TextPointer element, uint8_t key)
{
  // The key is the address of the cell in the m_grid.
  auto& cell = m_grid[key];

  // some cells are empty - nothing to do
  if(cell.empty()) return;

  // For each glyph in the cell
  for(uint16_t i = 0; i < cell.size(); ++i)
  {
    // If in good position to be next glyph
    if(element->is_adjoining_letter(*(cell[i])))
    {
      // Consume if identical. Skip if already consumed
      if(*cell[i] == *element) element->consume();
      if(cell[i]->is_consumed()) continue; // ignore if marked for deletion

      if(!element->has_join())
      {
        element->set_join(cell[i]);
        continue; // don't bother checking next statement
      }

      // If already a match but this one is better...
      if(element->get_join()->get_left() > cell[i]->get_left())
      {
        element->set_join(cell[i]);
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
  for(uint8_t column = 0; column < 16; ++column)
  {
    // For each cell in that column
    for(uint8_t row = 0; row < 16; ++row)
    {
      // Get the cell's contents
      vector<TextPointer>& cell = m_grid[column | (row << 4)];

      // If the cell is empty there's nothing to do
      if(cell.empty()) continue;

      // For each glyph in the cell
      for(auto& element : cell)
      {
        // If glyph is viable and matches another glyph to the right
        if(element->is_consumed() || !element->has_join()) continue;

        // Look up the right-matching glyph
        auto matcher = element->get_join();

        // Use the TextElement member function to merge the two glyphs
        element->merge_letters(*matcher);
      }
    }
  }
}
