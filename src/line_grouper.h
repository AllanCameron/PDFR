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

#include "textbox.h"


//---------------------------------------------------------------------------//
/* The LineGrouper class takes the output of the whitespace class, which is
 * a vector of TextBoxes - that is, a box containing a vector of text elements.
 * What we want is to change this so that we have a 1:1 correspondence between
 * boxes and text elements, but for the text elements to be joined-up, logical
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
  using TextPointer = std::shared_ptr<TextElement>;

  // Constructor takes the output of WordGrouper - a vector of TextBoxes
  LineGrouper(PageBox page_box_from_whitespace);

  // The output is also a vector of TextBoxes
  inline TextBox Output() { return text_boxes_.CastToTextBox();}

 private:
  void FindBreaks_(TextBox&);   // Identifies paragraph breaks
  void LineEndings_(TextBox&);  // Adjusts line endings to facilitate pasting
  void PasteLines_(TextBox&);   // Pastes TextElements in the TextBoxes together

  // Defines the reading order for elements in a text box. If an element is
  // higher than another, it comes before it. If it is at the same height but
  // to the left of the other element, it comes before it. In all other cases,
  // it comes afterwards.
  struct ReadingOrder_
  {
    bool operator() (const TextPointer& row1, const TextPointer& row2) const
    {
      if (row1->GetBottom()  > row2->GetBottom() ) return true;
      if (row1->GetBottom() == row2->GetBottom() &&
          row1->GetLeft()    < row2->GetLeft()   ) return true;
      return false;
    }
  };

  // private data member
  PageBox text_boxes_;
};


//---------------------------------------------------------------------------//

#endif
