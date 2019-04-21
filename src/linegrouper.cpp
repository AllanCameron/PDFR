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

linegrouper::linegrouper(textboxes t): m_textboxes(t)
{
  for(auto& i : m_textboxes)
  {
    auto& textrow_vector = i.second;
    if (textrow_vector.size() < 2) continue;
    sort(textrow_vector.begin(), textrow_vector.end(), reading_order());
    find_breaks(textrow_vector);
    line_endings(textrow_vector);
    paste_lines(textrow_vector);
  }
};

//---------------------------------------------------------------------------//

void linegrouper::find_breaks(vector<shared_ptr<textrow>>& textrow_vector)
{

}

//---------------------------------------------------------------------------//

void linegrouper::line_endings(vector<shared_ptr<textrow>>& textrow_vector)
{
for(size_t i = 0; i < textrow_vector.size() - 1; ++i)
  {
    auto& text_row = textrow_vector[i];
    switch(text_row->glyph.back())
    {
      case 0x0020:                             break;
      case 0x00A0:                             break;
      case 0x002d: text_row->glyph.pop_back(); break;
      case 0x2010: text_row->glyph.pop_back(); break;
      case 0x2011: text_row->glyph.pop_back(); break;
      case 0x2012: text_row->glyph.pop_back(); break;
      case 0x2013: text_row->glyph.pop_back(); break;
      case 0x2014: text_row->glyph.pop_back(); break;
      case 0x2015: text_row->glyph.pop_back(); break;
      default:     text_row->glyph.push_back(0x0020);
    }
  }
}

//---------------------------------------------------------------------------//

void linegrouper::paste_lines(vector<shared_ptr<textrow>>& textrow_vector)
{
  for(size_t i = 1; i < textrow_vector.size(); ++i)
  {
    concat(textrow_vector[0]->glyph, textrow_vector[i]->glyph);
  }
  textrow_vector.resize(1);
}

//---------------------------------------------------------------------------//

textboxes& linegrouper::output()
{
  return m_textboxes;
}

