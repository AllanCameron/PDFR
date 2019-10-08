//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextElement implementation file                                     //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "box.h"
#include "font.h"
#include "text_element.h"

using namespace std;

//---------------------------------------------------------------------------//

void TextElement::MergeLetters(TextElement& p_matcher)
{
   // paste the left glyph to the right glyph
  this->ConcatenateUnicode(p_matcher.glyph_);

  // make the right glyph now contain both glyphs
  p_matcher.glyph_ = this->glyph_;

  // make the right glyph now start where the left glyph started
  p_matcher.SetLeft(this->GetLeft());

  // Ensure bottom is the lowest value of the two glyphs
  if (this->GetBottom() < p_matcher.GetBottom())
    p_matcher.SetBottom(this->GetBottom());

  // The checked glyph is now consumed - move to the next
  this->Consume();
}

//---------------------------------------------------------------------------//

bool TextElement::IsElligibleToJoin(const TextElement& p_other) const
{
  return  !p_other.IsConsumed()                      &&
           p_other.IsBeyond(*this)                   &&
           p_other.IsOnSameLineAs(*this)             &&
          !p_other.IsWayBeyond(*this)                &&
          !this->CannotJoinLeftOf(p_other)            ;
}

//---------------------------------------------------------------------------//

void TextElement::JoinWords(TextElement& p_other)
{
    // This element is elligible for joining - start by adding a space to it
    this->glyph_.push_back(0x0020);

    // If the gap is wide enough, add two spaces
    if (p_other.GetLeft() - this->GetRight() > 1 * this->GetSize())
    {
      this->glyph_.push_back(0x0020);
    }

    // Stick contents together
    Concatenate(this->glyph_, p_other.GetGlyph());

    // The rightmost glyph's right edge properties are also copied over
    this->SetRight(p_other.GetRight());
    if (p_other.IsRightEdge()) this->MakeRightEdge();

    // The word will take up the size of its largest glyph
    this->SetTop(max(this->GetSize(), p_other.GetSize()) + this->GetBottom());

    // The element on the right is now consumed
    p_other.Consume();
}

//---------------------------------------------------------------------------//

void TextElement::ConcatenateUnicode(const std::vector<Unicode>& p_other)
{
  Concatenate(glyph_, p_other);
}

/*--------------------------------------------------------------------------*/
// converts (16-bit) Unicode code points to multibyte utf-8 encoding.

string TextElement::Utf()
{
  std::string result_string {}; // empty string for results
  for (auto& point : this->glyph_) // for each uint16_t in the input vector...
  {
    // values less than 128 are just single-byte ASCII
    if (point < 0x0080)
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
    if (point > 0x007f && point < 0x0800)
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
    if (point > 0x07ff)
    {
      // First we specifically change ligatures to appropriate Ascii values
      if (point == 0xFB00) {result_string += "ff"; continue;}
      if (point == 0xFB01) {result_string += "fi"; continue;}
      if (point == 0xFB02) {result_string += "fl"; continue;}
      if (point == 0xFB03) {result_string += "ffi"; continue;}
      if (point == 0xFB04) {result_string += "ffl"; continue;}

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

/*--------------------------------------------------------------------------*/
// Although this method looks like it should be inlined, doing so would mean
// having to include font.h in the header file

string TextElement::GetFontName() const
{
  return this->font_->GetFontName();
}
