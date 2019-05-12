//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR WordGrouper header file                                            //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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

class WordGrouper
{
 public:
  // Constructor
  WordGrouper(TextBox&&);

  // Output individual text elements for next phase of layout analysis
  TextBox& Output();

  // Output text elements with sizes, fonts, positions to API
  TextTable Out();

 private:
  // Make a table of values in a vector of floats rounded to one decimal place
  void Tabulate_(const std::vector<float>&, std::unordered_map<int, size_t>&);

  // Use tabulate function to find likely left, right or mid-aligned columns
  void FindEdges_();

  // Tell the text elements whether they form an edge or not
  void AssignEdges_();

  // Join elligible adjacent glyphs together and merge their properties
  void FindRightMatch_();

// private data members
  std::unordered_map<int, size_t> left_edges_,        // The tables of edges
                                  right_edges_,
                                  mids_;
  TextBox                         textbox_;           // The main data member
};

//---------------------------------------------------------------------------//

#endif
