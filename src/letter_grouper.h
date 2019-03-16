//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR letter_grouper header file                                          //
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

#ifndef PDFR_LGROUPER

//---------------------------------------------------------------------------//

#define PDFR_LGROUPER

/* The letter_grouper class co-ordinates the grouping together of words. In
 * terms of program structure, this comes directly after the parser step that
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
// Simple struct that acts as a method for sorting a vector of textrows
// left-to-right

struct sort_left_right
{
  inline bool operator() (const textrow& row1, const textrow& row2)
  {
      return (row1.left < row2.left);
  }
};

//---------------------------------------------------------------------------//
// The letter_grouper class contains a constructor, an output map of results,
// and a method for passing out the minimum text bounding box found in page
// construction. Its private methods are used only in construction of the
// output. The main private member is a map of vectors of textrows, each
// vector representing all glyphs in one of 256 equally sized cells on the page.
// Each glyph is addressable by two numbers - the grid number and the position
// of the glyph in the cell's vector.

class letter_grouper
{
public:
  // constructor.
  letter_grouper(GSoutput&);

  // public methods
  // Passes text elements to word_grouper for further construction if needed
  textrows output();
  GSoutput out(); // output table to interface if ungrouped words needed

private:
  // private data members
  constexpr static float CLUMP_H = 0.1; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1; // vertical clumping, high = sticky
  GSoutput& gslist;            // a copy of the parser output used to create grid
  std::vector<float> minbox;

  // the main data member. A 16 x 16 grid of cells, each with textrow vector
  std::unordered_map<uint8_t, std::vector<textrow>> gridmap;

  // private methods
  void makegrid();                    // assigns each glyph to a 16 x 16 grid
  void compareCells();                // co-ordinates matching between cells
  void matchRight(textrow&, uint8_t); // compare all glyphs in cell
  void merge();                       // join matching glyphs together
};

//---------------------------------------------------------------------------//

#endif