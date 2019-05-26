//----------------------------------------------------------------------------//
//                                                                            //
//  PDFR LetterGrouper header file                                            //
//                                                                            //
//  Copyright (C) 2018 - 2019 by Allan Cameron                                //
//                                                                            //
//  Licensed under the MIT license - see https://mit-license.org              //
//  or the LICENSE file in the project root directory                         //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef PDFR_LGROUPER

//----------------------------------------------------------------------------//

#define PDFR_LGROUPER

/* The LetterGrouper class co-ordinates the grouping together of words. In
 * terms of program structure, this comes directly after the parser step that
 * reads the page description program. The goal of this class is to clump
 * adjoining glyphs to form strings. Mostly, these will form words, but if
 * actual spaces are included as glyphs then grouped strings of words will be
 * included in the output.
 *
 * This is the first step of a "meet-in-the-middle" document reconstruction,
 * which will use these strings as the atoms from which to form structures such
 * as paragraphs, headers and tables.
 */

#include "text_element.h"

//----------------------------------------------------------------------------//
// The LetterGrouper class contains a constructor, an output map of results,
// and a method for passing out the minimum text bounding box found in page
// construction. Its private methods are used only in construction of the
// output. The main private member is a map of vectors of TextElements, each
// vector representing all glyphs in one of 256 equally sized cells on the page.
// Each glyph is addressable by two numbers - the grid number and the position
// of the glyph in the cell's vector.

class LetterGrouper
{
 public:
  // constructor.
  LetterGrouper(std::unique_ptr<TextBox>);

  // Passes text elements to WordGrouper for further construction if needed
  std::unique_ptr<TextBox> Output();
  TextTable Out(); // output table to interface if ungrouped words needed

 private:
  // A copy of the parser output used to create grid
  std::unique_ptr<TextBox> text_box_;

  // Main data member - a 16 x 16 grid of cells, each with a TextPointer vector
  std::unordered_map<uint8_t, std::vector<TextPointer>> grid_;

  // private methods
  void MakeGrid_();                       // Assigns glyphs to a 16 x 16 grid
  void CompareCells_();                   // Co-ordinates matching between cells
  void MatchRight_(TextPointer, uint8_t); // Compares all glyphs in cell
  void Merge_();                          // Joins matching glyphs together
};

//----------------------------------------------------------------------------//

#endif
