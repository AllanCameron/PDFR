//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR linegrouper implementation file                                     //
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

#include "linegrouper.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
//

linegrouper::linegrouper(textboxes t): m_textboxes(t)
{
  size_t i = 0;
  while(i < m_textboxes.size())
  {
    auto& element = m_textboxes[i];
    auto& textrow_vector = element.second;
    if (textrow_vector.size() < 2){++i; continue;}
    sort(textrow_vector.begin(), textrow_vector.end(), reading_order());
    find_breaks(element);
    if (textrow_vector.size() < 2){++i; continue;}
    line_endings(textrow_vector);
    paste_lines(textrow_vector);
    ++i;
  }
};

//---------------------------------------------------------------------------//
//

void linegrouper::find_breaks(textbox& this_box)
{
  auto& textrow_vector = this_box.second;
  for(size_t i = 1; i < textrow_vector.size(); ++i)
  {
    if(textrow_vector[i]->get_left() - textrow_vector[i - 1]->get_left() > 0.1
         &&
       textrow_vector[i]->get_bottom() < textrow_vector[i - 1]->get_bottom())
    {
      m_textboxes.push_back(splitbox(this_box,
                                     textrow_vector[i - 1]->get_bottom()));
      break;
    }

  }
}

//---------------------------------------------------------------------------//
//

void linegrouper::line_endings(vector<text_ptr>& textrow_vector)
{
  for(size_t i = 0; i < textrow_vector.size() - 1; ++i)
  {
    auto& text_row = textrow_vector[i];
    switch(text_row->get_glyph().back())
    {
      case 0x0020:                             break;
      case 0x00A0:                             break;
      case 0x002d: text_row->pop_last_glyph(); break;
      case 0x2010: text_row->pop_last_glyph(); break;
      case 0x2011: text_row->pop_last_glyph(); break;
      case 0x2012: text_row->pop_last_glyph(); break;
      case 0x2013: text_row->pop_last_glyph(); break;
      case 0x2014: text_row->pop_last_glyph(); break;
      case 0x2015: text_row->pop_last_glyph(); break;
      default:     text_row->add_space();
    }
  }
}

//---------------------------------------------------------------------------//
//

void linegrouper::paste_lines(vector<text_ptr>& textrow_vector)
{
  for(size_t i = 1; i < textrow_vector.size(); ++i)
  {
    textrow_vector[0]->concat_glyph(textrow_vector[i]->get_glyph());
  }
  textrow_vector.resize(1);
}

//---------------------------------------------------------------------------//
//

textbox linegrouper::splitbox(textbox& old_one, float top_edge)
{
  auto& old_box      = old_one.first;
  auto& old_contents = old_one.second;
  Box new_box        = old_box;
  auto break_point = find_if(old_contents.begin(), old_contents.end(),
                          [&](text_ptr& textrow_ptr) -> bool {
                            return textrow_ptr->get_bottom() < top_edge;
                          });
  vector<text_ptr> new_contents(break_point, old_contents.end());
  old_contents.erase(break_point, old_contents.end());
  old_box.bottom = new_box.top = old_contents.back()->get_bottom();
  return make_pair(new_box, new_contents);
}


//---------------------------------------------------------------------------//

textboxes& linegrouper::output()
{
  return m_textboxes;
}

