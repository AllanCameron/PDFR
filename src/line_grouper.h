//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR LineGrouper header file                                             //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_LINEGOUPER

//---------------------------------------------------------------------------//

#define PDFR_LINEGOUPER

#include "whitespace.h"

//---------------------------------------------------------------------------//
/*  The LineGrouper class takes the output of the whitespace class, which is
 * a vector of pairs : each pair contains a box representing an area on the
 * page, and a vector of the text elements contained within that box. What we
 * want is to change this so that we have a 1:1 correspondence between boxes
 * and text elements, but for the text elements to be joined-up, logical
 * components of the document such as paragraphs, headers, table entries and so
 * on.
 *
 * This requires a few different processes. First, we need to arrange all the
 * text elements in the boxes into the correct "reading order". Since we have
 * already split elements by whitespace, this should be a simple matter of
 * sorting top to bottom and left to right.
 *
 * Secondly, we need to determine whether there are logical breaks between
 * the lines of text, or whether they are supposed to join together. We do this
 * by taking clues such as the size of line spacing and the alignment of text to
 * spot paragraph breaks.
 *
 * Thirdly, we need to work out how lines are meant to be joined together.
 * Usually, they should be joined with a space. However, if a line is to be
 * joined to the one below but already ends in a space or ends in a hyphen,
 * it should be joined without a space.
 *
 * The LineGrouper class modifies the std::vector<TextBox> class, so we only
 * need to pass a pointer to this
 *
 */

class LineGrouper
{
public:
  LineGrouper(std::vector<TextBox> text_box_from_word_grouper);
  std::vector<TextBox>& Output();

private:
  void FindBreaks(TextBox&);
  void LineEndings(TextBox&);
  void PasteLines(TextBox&);
  void SplitBox(TextBox& box_to_be_split, float divide_at_this_y_value);

  struct ReadingOrder
  {
    bool operator() (const TextPointer& row1, const TextPointer& row2) const
    {
      if (row1->GetBottom()  > row2->GetBottom() ) return true;
      if (row1->GetBottom() == row2->GetBottom() &&
          row1->GetLeft()    < row2->GetLeft()) return true;
      return false;
    }
  };

  // private data members
  std::vector<TextBox> text_boxes_;
};


//---------------------------------------------------------------------------//

#endif
