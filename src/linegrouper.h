//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR linegrouper header file                                             //
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

#ifndef PDFR_LINEGOUPER

//---------------------------------------------------------------------------//

#define PDFR_LINEGOUPER

#include "whitespace.h"

//---------------------------------------------------------------------------//
/*  The linegrouper class takes the output of the whitespace class, which is
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
 * The linegrouper class modifies the std::vector<textbox> class, so we only
 * need to pass a pointer to this
 *
 */

class linegrouper
{
public:
  linegrouper(std::vector<textbox> t);
  std::vector<textbox>& output();

private:
  void find_breaks(textbox&);
  void line_endings(textbox&);
  void paste_lines(textbox&);
  textbox splitbox(textbox&, float);

  struct reading_order
  {
    bool operator() (const text_ptr& row1, const text_ptr& row2) const
    {
      if (row1->get_bottom()  > row2->get_bottom() ) return true;
      if (row1->get_bottom() == row2->get_bottom() &&
          row1->get_left()    < row2->get_left()) return true;
      return false;
    }
  };

  // private data members
  std::vector<textbox> m_textboxes;
};


//---------------------------------------------------------------------------//

#endif
