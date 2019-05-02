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
(float l, float r, float t, float b, shared_ptr<font> f, std::vector<Unicode> g):
Box(l, r, t, b), m_font(f), glyph(g), m_join(nullptr) {};


//---------------------------------------------------------------------------//

void text_element::merge_letters(text_element& matcher)
{
   // paste the left glyph to the right glyph
  this->concat_glyph(matcher.glyph);

  // make the right glyph now contain both glyphs
  matcher.glyph = this->glyph;

  // make the right glyph now start where the left glyph started
  matcher.set_left(this->get_left());

  // Ensure bottom is the lowest value of the two glyphs
  if(this->get_bottom() < matcher.get_bottom())
    matcher.set_bottom(this->get_bottom());

  // The checked glyph is now consumed - move to the next
  this->consume();
}

//---------------------------------------------------------------------------//

bool text_element::is_elligible_to_join(const text_element& other) const
{
  return  !other.is_consumed()                     &&
           other.is_beyond(*this)                  &&
           other.is_on_same_line_as(*this)         &&
          !other.is_way_beyond(*this)              &&
          !this->cannot_join_left_of(other)         ;
}

//---------------------------------------------------------------------------//

void text_element::join_words(text_element& other)
{
    // This element is elligible for joining - start by adding a space to it
    this->glyph.push_back(0x0020);

    // If the gap is wide enough, add two spaces
    if(other.get_left() - this->get_right() > 1 * this->get_size())
    {
      this->glyph.push_back(0x0020);
    }

    // Stick contents together
    concat(this->glyph, other.get_glyph());

    // The rightmost glyph's right edge properties are also copied over
    this->set_right(other.get_right());
    if(other.is_right_edge()) this->make_right_edge();

    // The word will take up the size of its largest glyph
    this->set_top(max(this->get_size(), other.get_size()) + this->get_bottom());

    // The element on the right is now consumed
    other.consume();
}

//---------------------------------------------------------------------------//

void text_element::concat_glyph(const std::vector<Unicode>& other)
{
  concat(glyph, other);
}

//---------------------------------------------------------------------------//
// Converts textbox to text_table
text_table::text_table(const textbox& text_box):
Box((Box) text_box)
{
  for(auto ptr = text_box.cbegin(); ptr != text_box.cend(); ++ptr)
  {
    auto& element = *ptr;
    if(!element->is_consumed())
    {
      this->text.push_back(element->utf());
      this->left.push_back(element->get_left());
      this->bottom.push_back(element->get_bottom());
      this->right.push_back(element->get_right());
      this->fonts.push_back(element->get_font());
      this->top.push_back(element->get_top());
      this->size.push_back(element->get_top() - element->get_bottom());
    }
  }
}

//---------------------------------------------------------------------------//

void textbox::remove_duplicates()
{
  for (auto this_row = m_data.begin(); this_row != m_data.end(); ++this_row)
  {
    if ((*this_row)->is_consumed()) continue;
    for (auto other_row = this_row; other_row != m_data.end(); ++other_row)
    {
      if(other_row == this_row) continue;

      if (**other_row == **this_row)
      {
        (*other_row)->consume();
      }
    }
  }
}

//---------------------------------------------------------------------------//
// Join another text table to this one

void text_table::join(text_table& other)
{
  this->merge(other);
  concat(this->text, other.text);
  concat(this->left, other.left);
  concat(this->bottom, other.bottom);
  concat(this->right, other.right);
  concat(this->fonts, other.fonts);
  concat(this->top, other.top);
}

/*--------------------------------------------------------------------------*/
// converts (16-bit) Unicode code points to multibyte utf-8 encoding.

string text_element::utf()
{
  std::string result_string {}; // empty string for results
  for(auto& point : this->glyph) // for each uint16_t in the input vector...
  {
    // values less than 128 are just single-byte ASCII
    if(point < 0x0080)
    {
      result_string.push_back(point & 0x007f);
      continue;
    }

    // values of 128 - 2047 are two bytes. The first byte starts 110xxxxx
    // and the second starts 10xxxxxx. The remaining 11 x's are filled with the
    // 11 bits representing a number between 128 and 2047. e.g. Unicode point
    // U+061f (decimal 1567) is 11000011111 in 11 bits of binary, which we split
    // into length-5 and length-6 pieces 11000 and 011111. These are appended on
    // to 110 and 10 respectively to give the 16-bit number 110 11000 10 011111,
    // which as two bytes is 11011000 10011111 or d8 9f. Thus the UTF-8
    // encoding for character U+061f is the two-byte sequence d8 9f.
    if(point > 0x007f && point < 0x0800)
    {
      // construct byte with bits 110 and first 5 bits of unicode point number
      result_string.push_back((0x00c0 | ((point >> 6) & 0x001f)));

      // construct byte with bits 10 and final 6 bits of unicode point number
      result_string.push_back(0x0080 | (point & 0x003f));
      continue;
    }

    // Unicode values between 2048 (0x0800) and the maximum uint16_t value
    // (65535 or 0xffff) are given by 16 bits split over three bytes in the
    // following format: 1110xxxx 10xxxxxx 10xxxxxx. Each x here takes one of
    // the 16 bits representing 2048 - 65535.
    if(point > 0x07ff)
    {
      // First we specifically change ligatures to appropriate Ascii values
      if(point == 0xFB00) {result_string += "ff"; continue;}
      if(point == 0xFB01) {result_string += "fi"; continue;}
      if(point == 0xFB02) {result_string += "fl"; continue;}
      if(point == 0xFB03) {result_string += "ffi"; continue;}
      if(point == 0xFB04) {result_string += "ffl"; continue;}

      // construct byte with 1110 and first 4 bits of unicode point number
      result_string.push_back(0x00e0 | ((point >> 12) & 0x000f));

      // construct byte with 10 and bits 5-10 of unicode point number
      result_string.push_back(0x0080 | ((point >> 6) & 0x003f));

      // construct byte with bits 10 and final 6 bits of unicode point number
      result_string.push_back(0x0080 | ((point) & 0x003f));
    }
    // Although higher Unicode points are defined and can be encoded in utf8,
    // the hex-strings in pdf seem to be two bytes wide at most. These are
    // therefore not supported at present.
  }
  return result_string;
}
