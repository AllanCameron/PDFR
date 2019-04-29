//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR line_grouper implementation file                                    //
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

#include "line_grouper.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
//

line_grouper::line_grouper(vector<textbox> t): m_textboxes(t)
{
  size_t i = 0;
  if(m_textboxes.empty()) throw runtime_error("No textboxes on page");
  while(i < m_textboxes.size())
  {
    if (m_textboxes[i].size() < 2){++i; continue;}
    sort(m_textboxes[i].begin(), m_textboxes[i].end(), reading_order());
    find_breaks(m_textboxes[i]);
    if (m_textboxes[i].size() < 2){++i; continue;}
    line_endings(m_textboxes[i]);
    paste_lines(m_textboxes[i]);
    ++i;
  }
};

//---------------------------------------------------------------------------//
//

void line_grouper::find_breaks(textbox& text_box)
{
  for(size_t i = 1; i < text_box.size(); ++i)
  {
    if(text_box[i]->get_left() - text_box[i - 1]->get_left() > 0.1
         &&
       text_box[i]->get_bottom() < text_box[i - 1]->get_bottom())
    {
      m_textboxes.push_back(splitbox(text_box, text_box[i - 1]->get_bottom()));
      break;
    }

  }
}

//---------------------------------------------------------------------------//
//

void line_grouper::line_endings(textbox& text_box)
{
  for(size_t i = 0; i < text_box.size() - 1; ++i)
  {
    auto& element = text_box[i];
    switch(element->get_glyph().back())
    {
      case 0x0020:                             break;
      case 0x00A0:                             break;
      case 0x002d: element->pop_last_glyph(); break;
      case 0x2010: element->pop_last_glyph(); break;
      case 0x2011: element->pop_last_glyph(); break;
      case 0x2012: element->pop_last_glyph(); break;
      case 0x2013: element->pop_last_glyph(); break;
      case 0x2014: element->pop_last_glyph(); break;
      case 0x2015: element->pop_last_glyph(); break;
      default:     element->add_space();
    }
  }
}

//---------------------------------------------------------------------------//
//

void line_grouper::paste_lines(textbox& text_box)
{
  for(size_t i = 1; i < text_box.size(); ++i)
  {
    text_box[0]->concat_glyph(text_box[i]->get_glyph());
  }
  text_box.resize(1);
}

//---------------------------------------------------------------------------//
//

textbox line_grouper::splitbox(textbox& old_one, float top_edge)
{
  auto& old_box      = old_one.m_box;
  auto& old_contents = old_one.m_data;
  Box new_box        = old_box;
  auto break_point = find_if(old_contents.begin(), old_contents.end(),
                          [&](text_ptr& textptr) -> bool {
                            return textptr->get_bottom() < top_edge;
                          });
  vector<text_ptr> new_contents(break_point, old_contents.end());
  old_contents.erase(break_point, old_contents.end());
  old_box.set_bottom(old_contents.back()->get_bottom());
  new_box.set_top(old_contents.back()->get_bottom());
  return textbox(new_contents, new_box);
}


//---------------------------------------------------------------------------//

vector<textbox>& line_grouper::output()
{
  return m_textboxes;
}

