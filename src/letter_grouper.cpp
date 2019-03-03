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
// This is a tabular output for the class to provide a method for outputting
// words to the interface without any higher level grouping

gridoutput letter_grouper::out()
{
  std::vector<float> left, right, width, bottom, size; // these vectors are
  std::vector<std::string> font, text;                 // components of the
  for(uint8_t i = 0; i < 256; i++)                     // gridout struct
  {// for each cell in the grid...
    for(auto& j : gridmap[i])
    {// for each textrow in the cell...
      if(!j.consumed) //if it isn't marked for deletion...
      {// copy its contents over
        text.push_back(utf({j.glyph}));
        left.push_back(j.left);
        right.push_back(j.right);
        size.push_back(j.size);
        bottom.push_back(j.bottom);
        font.push_back(j.font);
        width.push_back(j.right - j.left);
      }
    }
    if(i == 255) break; // prevent overflow back to 0 and endless loop
  }
  return gridoutput{left, right, width, bottom, size, font, text };
}

//---------------------------------------------------------------------------//
// The letter_grouper constructor calls three subroutines. These split the
// page into an easily addressable 16 x 16 grid, find glyphs in close proximity
// to each other, and glue them together, respectively.

letter_grouper::letter_grouper(const parser& GS) : gs(GS)
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
  minbox = gs.getminbox();  // gets the minbox found on page creation
  GSoutput* gslist = gs.output(); // Take the output of parser
  float dx = (minbox[2] - minbox[0])/16; // calculate width of cells
  float dy = (minbox[3] - minbox[1])/16; // calculate height of cells
  for(unsigned i = 0; i < gslist->width.size(); i++) // for each glyph...
  {
    textrow newrow = {  gslist->left[i],   //------//
                      gslist->right[i],          //
                      gslist->width[i],          //
                      gslist->bottom[i],         //    create a struct with all
                      gslist->size[i],           //--> pertinent size & position
                      gslist->fonts[i],          //    information
                     {gslist->text[i]},          //
                      false,                     //
                      make_pair(-1, -1),         //
                      false,                     //
                      false,                     //
                      false              //------//
                    };
    // push the struct to the vector in the cell
    gridmap[((uint8_t)(newrow.left / dx))  |
            ((uint8_t)(15 - (newrow.bottom / dy)) << 4 )].push_back(newrow);
  }
  // sort the contents of the cell from left to right
  for(auto& vec : gridmap)
    std::sort(vec.second.begin(), vec.second.end(), sort_left_right());
}

//---------------------------------------------------------------------------//
// Allows the main data object to be output after calculations done

std::unordered_map<uint8_t, std::vector<textrow>> letter_grouper::output()
{
  return gridmap;
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
  for(uint8_t i = 0; i < 16; i++) // for each of 16 columns of cells on page
  {
    for(uint8_t j = 0; j < 16; j++) // for each cell in the column
    {
      uint8_t key = i | (j << 4);   // get the address of the cell
      vector<textrow>& maingroup = gridmap[key]; // get the cell's contents
      if(maingroup.empty()) continue;  // empty cell - nothing to be done
      for(auto& k : maingroup) // for each glyph in the cell
      {
        matchRight(k, key); // check for matches in this cell
        if(j < 15) matchRight(k, (i) | ((j + 1) << 4)); // and cell to North
        if(j > 0) matchRight(k, (i) | ((j - 1) << 4)); // and cell to South
        if(k.rightjoin.first != -1) continue; // If match, look no further
        if(i < 15) matchRight(k, (i + 1) | (j << 4)); // else check to the East,
        if(i < 15 && j < 15) matchRight(k, (i + 1) | ((j + 1) << 4)); // NE,
        if(i < 15 && j > 0) matchRight(k, (i + 1) | ((j - 1) << 4)); // and SE
      }
    }
  }
}

//---------------------------------------------------------------------------//
// This is the algorithm that finds adjoining glyphs. Each glyph is addressable
// by its cell and the order it appears in the vector of glyphs contained in
// that cell. The glyphs are called textrows here because each glyph forms a row
// of output from the parser class

void letter_grouper::matchRight(textrow& row, uint8_t key)
{
  // The key is the address of the cell in the gridmap
  std::vector<textrow>& cell = gridmap[key]; // get the vector of matching cell
  if(cell.empty()) return; // some cells are empty - nothing to do
  for(uint16_t i = 0; i < cell.size(); i++) // for each glyph in the cell...
    if (cell[i].left > row.left &&
        abs(cell[i].bottom - row.bottom) < (CLUMP_V * row.size) &&
        (abs(cell[i].left  -  row.right ) < (CLUMP_H * row.size) ||
        (cell[i].left < row.right)) )
    {// if in good position to be next glyph
      // mark row for deletion if it is identical
      if(cell[i] == row) row.consumed = true;
      if(cell[i].consumed) continue; // ignore if marked for deletion
      if(row.rightjoin.first == -1) // if no previous match found
      {
        row.rightjoin = make_pair((int) key, (int) i); // store match address
        continue; // don't bother checking next statement
      }
      if(gridmap[row.rightjoin.first][row.rightjoin.second].left >
           cell[i].left) // if already a match but this one is better...
        row.rightjoin = make_pair((int) key, (int) i); // ...store match address
    }
}

//---------------------------------------------------------------------------//
// Takes each glyph and sticks it onto any right-adjoining glyph, updating the
// the latter's size and position parameters and declaring the leftward glyph
// "consumed"

void letter_grouper::merge()
{
  for(uint8_t i = 0; i < 16; i++)  // for each column in the x-axis
  {
    for(uint8_t j = 0; j < 16; j++) // for each cell in that column
    {
      uint8_t key = i | (j << 4);         // get the cell's address
      vector<textrow>& cell = gridmap[key]; // get the cell's contents
      if(cell.size() == 0) continue;      // Cell empty - nothing to do
      for(auto& k : cell)                 // for each glyph in the cell
      {
        if(k.consumed || k.rightjoin.first == -1) continue; // nothing joins
        // look up the right-matching glyph
        textrow& matcher = gridmap[k.rightjoin.first][k.rightjoin.second];
        // paste the left glyph to the right glyph
        concat(k.glyph, matcher.glyph);
        if(matcher.size < k.size && matcher.bottom != k.bottom)
          matcher.size = k.size;
        // make the right glyph now contain both glyphs
        matcher.glyph = k.glyph;
        // make the right glyph now start where the left glyph started
        matcher.left = k.left;
        // and ensure the width is correct
        matcher.width = matcher.right - matcher.left;
        // Ensure bottom is the lowest value of the two glyphs
        if(k.bottom < matcher.bottom) matcher.bottom = k.bottom;
        // The checked glyph is now consumed - move to the next
        k.consumed = true;
        matcher.consumed = false;
      }
    }
  }
}

//---------------------------------------------------------------------------//
// Passes the minbox calculated on page creation to allow plotting etc

std::vector<float> letter_grouper::getBox()
{
  return minbox;
}
