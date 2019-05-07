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

line_grouper::line_grouper(vector<TextBox> t): m_textboxes(t)
{
  size_t i = 0;
  if(m_textboxes.empty()) throw runtime_error("No textboxes on page");

  while (i < m_textboxes.size())
  {
    if (m_textboxes[i].size() < 2){++i; continue;}
    sort(m_textboxes[i].begin(), m_textboxes[i].end(), reading_order());
    find_breaks(m_textboxes[i]);
    if (m_textboxes[i].size() < 2){++i; continue;}
    line_endings(m_textboxes[i]);
    paste_lines(m_textboxes[i++]);
  }
};

//---------------------------------------------------------------------------//
//

void line_grouper::find_breaks(TextBox& text_box)
{
  for(size_t i = 1; i < text_box.size(); ++i)
  {
    if(text_box[i]->GetLeft() - text_box[i - 1]->GetLeft() > 0.1
         &&
       text_box[i]->GetBottom() < text_box[i - 1]->GetBottom())
    {
      splitbox(text_box, text_box[i - 1]->GetBottom());
      break;
    }

  }
}

//---------------------------------------------------------------------------//
//

void line_grouper::line_endings(TextBox& text_box)
{
  for(size_t i = 0; i < text_box.size() - 1; ++i)
  {
    auto& element = text_box[i];
    switch(element->GetGlyph().back())
    {
      case 0x0020:                             break;
      case 0x00A0:                             break;
      case 0x002d: element->PopLastGlyph(); break;
      case 0x2010: element->PopLastGlyph(); break;
      case 0x2011: element->PopLastGlyph(); break;
      case 0x2012: element->PopLastGlyph(); break;
      case 0x2013: element->PopLastGlyph(); break;
      case 0x2014: element->PopLastGlyph(); break;
      case 0x2015: element->PopLastGlyph(); break;
      default:     element->AddSpace();
    }
  }
}

//---------------------------------------------------------------------------//
//

void line_grouper::paste_lines(TextBox& text_box)
{
  for(size_t i = 1; i < text_box.size(); ++i)
  {
    text_box[0]->ConcatGlyph(text_box[i]->GetGlyph());
  }
  text_box.resize(1);
}

//---------------------------------------------------------------------------//
//

void line_grouper::splitbox(TextBox& old_one, float top_edge)
{
  if(old_one.empty()) return;
  TextBox new_one = old_one;
  new_one.clear();
  size_t breakpoint = 0;

  for(size_t i = 0; i < old_one.size(); ++i)
  {
    if(old_one[i]->GetBottom() < top_edge)
    {
      if(breakpoint != 0) breakpoint = i;
      new_one.push_back(old_one[i]);
    }
  }
  if(breakpoint > 0) old_one.resize(breakpoint - 1);

  old_one.SetBottom(old_one.back()->GetBottom());
  new_one.SetTop(old_one.front()->GetTop());
  m_textboxes.push_back(new_one);
}


//---------------------------------------------------------------------------//

vector<TextBox>& line_grouper::output()
{
  return m_textboxes;
}

