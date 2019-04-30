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
(float l, float r, float t, float b, string f, std::vector<Unicode> g):
Box(l, r, t, b), font(f), glyph(g), r_join(make_pair(-1, -1)), m_flags(0) {};


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
  if(other.is_consumed() ||
     (other.get_left() < this->get_right()) ||
     (other.get_bottom() - this->get_bottom() > 0.7 * this->get_size()) ||
     (this->get_bottom() - other.get_bottom() > 0.7 * this->get_size()) ||
     (other.get_left() - this->get_right() > 2 * this->get_size()) ||
      ((other.is_left_edge()  || other.is_centred())           &&
      (other.get_left() - this->get_right() > 0.51 * this->get_size())) ||
     ((this->is_right_edge() || this->is_centred())   &&
      (other.get_left() - this->get_right() > 0.51 * this->get_size()))  )
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
Box(text_box.get_left(), text_box.get_right(),
    text_box.get_top(), text_box.get_bottom())
{
  for(auto ptr = text_box.cbegin(); ptr != text_box.cend(); ++ptr)
  {
    auto& element = *ptr;
    if(!element->is_consumed())
    {
      this->text.push_back(element->get_glyph());
      this->left.push_back(element->get_left());
      this->bottom.push_back(element->get_bottom());
      this->right.push_back(element->get_right());
      this->fonts.push_back(element->get_font());
      this->top.push_back(element->get_top());
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

std::vector<float> text_table::get_size()
{
  std::vector<float> res;
  if(bottom.size() != top.size() || bottom.empty()) return res;
  for(size_t i = 0; i < bottom.size(); ++i) res.push_back(bottom[i] + top[i]);
  return res;
}

//---------------------------------------------------------------------------//

std::vector<text_element> text_table::transpose()
{
  std::vector<text_element> res;
  if(!left.empty())
    for(size_t i = 0; i < left.size(); ++i)
      res.emplace_back(text_element(left[i], right[i], top[i],
                                    bottom[i], fonts[i], text[i]));
  return res;
}

//---------------------------------------------------------------------------//
// Join another text table to this one

void text_table::join(const text_table& other)
{
  this->merge(other);
  concat(this->text, other.text);
  concat(this->left, other.left);
  concat(this->bottom, other.bottom);
  concat(this->right, other.right);
  concat(this->fonts, other.fonts);
  concat(this->top, other.top);
}
