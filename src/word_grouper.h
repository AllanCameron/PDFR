//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR word_grouper header file                                            //
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

#ifndef PDFR_WGROUPER

//---------------------------------------------------------------------------//

#define PDFR_WGROUPER

/* The word grouper takes all of the words stuck together by the letter grouper
 * and attempts to join them into lines of text. It does this primarily by
 * identifying whether two adjacent words are close enough to be joined by a
 * single space character.
 *
 * There are a few caveats to this. Often text will be in columns, and we don't
 * want words at the right edge of one column to join to words in the adjacent
 * column if they are close together. The word grouper attempts to prevent this
 * by identifying words on the page whose left edges are aligned. If several
 * words have matching left edges, then they probably form a left-aligned
 * column. Any word with its left edge on a left-aligned column should not be
 * allowed to be joined to a word to its right.
 *
 * This isn't perfect, since we may get false positives, when words
 * coincidentally line up within a body of text. The higher we stipulate the
 * number of words that must be aligned to count as a column, the less likely
 * this is to happen, but we will then run the risk of false negatives, where
 * adjacent columns get stuck together. Therefore, the more left edges we find
 * and the higher the likliehood of a column being present, the smaller the gap
 * that is allowed to be bridged.
 *
 * We carry out a similar process for right-aligned and centre-aligned text.
 * Right-aligned text is intolerant of anything to the left joining and centre-
 * aligned text is intolerant of left or right joins.
 */

#include "letter_grouper.h"

//---------------------------------------------------------------------------//
// The word grouper class takes a pointer to a letter grouper object in its
// constructor. It makes a table of the x values of the left, right and centre
// points of each word, and uses these to infer which word pairs are elligible
// for sticking together.

class word_grouper
{
public:
  // Constructor
  word_grouper(textbox&&);

  // Output individual text elements for next phase of layout analysis
  textbox& output();

  // Output text elements with sizes, fonts, positions to API
  GSoutput out();

private:
  // Make a table of values in a vector of floats rounded to one decimal place
  void tabulate(const std::vector<float>&, std::unordered_map<int, size_t>&);

  // Use tabulate function to find likely left, right or mid-aligned columns
  void findEdges();

  // Tell the text elements whether they form an edge or not
  void assignEdges();

  // Join elligible adjacent glyphs together and merge their properties
  void findRightMatch();

// private data members

  // The tables of edges
  std::unordered_map<int, size_t> m_leftEdges, m_rightEdges, m_mids;

  // The main data member
  textbox m_allRows;
};

//---------------------------------------------------------------------------//

#endif
