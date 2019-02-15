//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR grid header file                                                    //
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

#ifndef PDFR_GRID

//---------------------------------------------------------------------------//

#define PDFR_GRID

/* The grid class co-ordinates the grouping together of words. In terms of
 * program structure, this comes directly after the graphic_state step that
 * reads the page description program. The goal of this class is to clump
 * adjoining glyphs to form strings. Mostly, these will form words, but if
 * actual spaces are included as glyphs then grouped strings of words will be
 * included in the output.
 *
 * This is the first step of a "bottom-up" document reconstruction, which
 * will use these strings as the atoms from which to form structures such
 * as paragraphs, headers and tables.
 */

#include "tokenizer.h"

//---------------------------------------------------------------------------//
// The GSrow is a struct which acts as a "row" of information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found as part of grid's
// construction, and a boolean flag to indicate whether it is "consumed" when
// the glyphs are stuck together into words.

struct GSrow
{
  float       left,             // position of left edge of text on page
              right,            // position of right edge of text on page
              width,            // width of text element
              bottom,           // y position of bottom edge of text
              size;             // point size of font used to draw text
  std::string font;             // Name of font used to draw text
  std::vector<Unicode> glyph;   // The actual Unicode glyphs encoded
  bool        consumed;         // Should element be ignored in output?
  std::pair<int, int> rightjoin;// address of closest adjacent element
};

struct gridoutput
{
  std::vector<float> left, right, width, bottom, size;
  std::vector<std::string> font, text;
};

//---------------------------------------------------------------------------//
// Simple struct that acts as a method for sorting a vector of GSrows
// left-to-right

struct sort_left_right
{
  inline bool operator() (const GSrow& row1, const GSrow& row2)
  {
      return (row1.left < row2.left);
  }
};

//---------------------------------------------------------------------------//
// The grid class contains a public constructor, an output map of results,
// and a method for passing out the minimum text bounding box found in page
// construction. Its private methods are used only in construction of the
// output. The main private member is a map of vectors of GSrows, each
// vector representing all glyphs in one of 256 equally sized cells on the page.
// Each glyph is addressable by two numbers - the grid number and the position
// of the glyph in the cell's vector.

class grid
{
public:
  // constructor
  grid(const graphic_state&);

  // public methods
  std::unordered_map<uint8_t, std::vector<GSrow>> output(); // result
  std::vector<float> getBox();                              // gets minbox
  gridoutput out();

private:
  // private data members
  constexpr static float CLUMP_H = 0.1; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.7; // vertical clumping, high = sticky
  graphic_state gs;                     // a copy of the gs used to create grid
  std::vector<float> minbox;            // page's minimum bounding box

  // the main data member. A 16 x 16 grid of cells, each with GSrow vector
  std::unordered_map<uint8_t, std::vector<GSrow>> gridmap;

  // private methods
  void makegrid();                  // assigns each glyph to a grid
  void compareCells();              // co-ordinates matching between cells
  void matchRight(GSrow&, uint8_t); // compare all glyphs in cell to index glyph
  void merge();                     // join matching glyphs
};

//---------------------------------------------------------------------------//

#endif
