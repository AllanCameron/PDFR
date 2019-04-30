//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR text_element implementation file                                         //
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

#include "text_element.h"

using namespace std;


//---------------------------------------------------------------------------//

text_element::text_element
(float l, float r, float b, float t, string f, vector<Unicode> g):
  left(l), right(r), bottom(b), top(t), font(f), glyph(g),
  r_join(make_pair(-1, -1)), m_flags(0) {};


//---------------------------------------------------------------------------//

void text_element::merge_letters(text_element& matcher)
{
   // paste the left glyph to the right glyph
  concat_glyph(matcher.glyph);

  // make the right glyph now contain both glyphs
  matcher.glyph = this->glyph;

  // make the right glyph now start where the left glyph started
  matcher.left = this->left;

  // Ensure bottom is the lowest value of the two glyphs
  if(this->bottom < matcher.bottom) matcher.bottom = this->bottom;

  // The checked glyph is now consumed - move to the next
  this->consume();
}

//---------------------------------------------------------------------------//

bool text_element::is_elligible_to_join(const text_element& other) const
{
  if(other.is_consumed() ||
     (other.left < this->right) ||
     (other.bottom - this->bottom > 0.7 * this->get_size()) ||
     (this->bottom - other.bottom > 0.7 * this->get_size()) ||
     (other.left - this->right > 2 * this->get_size()) ||
      ((other.is_left_edge()  || other.is_centred())           &&
      (other.left - this->right > 0.51 * this->get_size())) ||
     ((this->is_right_edge() || this->is_centred())   &&
      (other.left - this->right > 0.51 * this->get_size()))  )
    return false;
  else
    return true;
}

//---------------------------------------------------------------------------//

void text_element::join_words(text_element& other)
{
    // This element is elligible for joining - start by adding a space to it
    this->glyph.push_back(0x0020);

    // If the gap is wide enough, add two spaces
    if(other.left - this->right > 1 * this->get_size())
    {
      this->glyph.push_back(0x0020);
    }

    // Stick contents together
    concat(this->glyph, other.glyph);

    // The rightmost glyph's right edge properties are also copied over
    this->right = other.right;
    if(other.is_right_edge()) this->make_right_edge();

    // The word will take up the size of its largest glyph
    this->top = max(this->get_size(), other.get_size()) + this->bottom;

    // The element on the right is now consumed
    other.consume();
}


void text_element::concat_glyph(const std::vector<Unicode>& other)
{
  concat(glyph, other);
}
