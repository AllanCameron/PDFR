//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextElement implementation file                                     //
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

void TextElement::MergeLetters(TextElement& t_matcher)
{
   // paste the left glyph to the right glyph
  this->ConcatenateUnicode(t_matcher.glyph_);

  // make the right glyph now contain both glyphs
  t_matcher.glyph_ = this->glyph_;

  // make the right glyph now start where the left glyph started
  t_matcher.SetLeft(this->GetLeft());

  // Ensure bottom is the lowest value of the two glyphs
  if(this->GetBottom() < t_matcher.GetBottom())
    t_matcher.SetBottom(this->GetBottom());

  // The checked glyph is now consumed - move to the next
  this->Consume();
}

//---------------------------------------------------------------------------//

bool TextElement::IsElligibleToJoin(const TextElement& t_other) const
{
  return  !t_other.IsConsumed()                      &&
           t_other.IsBeyond(*this)                   &&
           t_other.IsOnSameLineAs(*this)             &&
          !t_other.IsWayBeyond(*this)                &&
          !this->CannotJoinLeftOf(t_other)            ;
}

//---------------------------------------------------------------------------//

void TextElement::JoinWords(TextElement& t_other)
{
    // This element is elligible for joining - start by adding a space to it
    this->glyph_.push_back(0x0020);

    // If the gap is wide enough, add two spaces
    if(t_other.GetLeft() - this->GetRight() > 1 * this->GetSize())
    {
      this->glyph_.push_back(0x0020);
    }

    // Stick contents together
    Concatenate(this->glyph_, t_other.GetGlyph());

    // The rightmost glyph's right edge properties are also copied over
    this->SetRight(t_other.GetRight());
    if(t_other.IsRightEdge()) this->MakeRightEdge();

    // The word will take up the size of its largest glyph
    this->SetTop(max(this->GetSize(), t_other.GetSize()) + this->GetBottom());

    // The element on the right is now consumed
    t_other.Consume();
}

//---------------------------------------------------------------------------//

void TextElement::ConcatenateUnicode(const std::vector<Unicode>& t_other)
{
  Concatenate(glyph_, t_other);
}

//---------------------------------------------------------------------------//
// Converts TextBox to TextTable
TextTable::TextTable(const TextBox& t_text_box):
Box((Box) t_text_box)
{
  for(auto ptr = t_text_box.cbegin(); ptr != t_text_box.cend(); ++ptr)
  {
    auto& element = *ptr;
    if(!element->IsConsumed())
    {
      this->text_.push_back(element->Utf());
      this->lefts_.push_back(element->GetLeft());
      this->bottoms_.push_back(element->GetBottom());
      this->rights_.push_back(element->GetRight());
      this->fonts_.push_back(element->GetFontName());
      this->tops_.push_back(element->GetTop());
      this->sizes_.push_back(element->Height());
    }
  }
}

//---------------------------------------------------------------------------//

void TextBox::RemoveDuplicates()
{
  for (auto this_row = data_.begin(); this_row != data_.end(); ++this_row)
  {
    if ((*this_row)->IsConsumed()) continue;
    for (auto other_row = this_row; other_row != data_.end(); ++other_row)
    {
      if(other_row == this_row) continue;

      if (**other_row == **this_row)
      {
        (*other_row)->Consume();
      }
    }
  }
}

//---------------------------------------------------------------------------//
// Join another text table to this one

void TextTable::Join(TextTable& t_other)
{
  this->Merge(t_other);
  Concatenate(this->text_,    t_other.text_);
  Concatenate(this->lefts_,   t_other.lefts_);
  Concatenate(this->bottoms_, t_other.bottoms_);
  Concatenate(this->rights_,  t_other.rights_);
  Concatenate(this->fonts_,   t_other.fonts_);
  Concatenate(this->tops_,    t_other.tops_);
}

/*--------------------------------------------------------------------------*/
// converts (16-bit) Unicode code points to multibyte utf-8 encoding.

string TextElement::Utf()
{
  std::string result_string {}; // empty string for results
  for(auto& point : this->glyph_) // for each uint16_t in the input vector...
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
