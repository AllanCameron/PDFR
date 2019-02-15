//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR grouper implementation file                                            //
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

#include "grouper.h"

using namespace std;

grouper::grouper(unordered_map<uint8_t, vector<GSrow>> gridout)
{
  vector<GSrow> tmpvec;
  for(uint8_t i = 0; i < 255; i++)
    concat(tmpvec, gridout[i]);
  concat(tmpvec, gridout[0xff]);
  for(auto& i : tmpvec) if(!i.consumed) allRows.emplace_back(move(i));
  findLeftEdges();
  findRightEdges();
  findMids();
  findTops();
  findBottoms();
};


std::vector<GSrow> grouper::output()
{
  return allRows;
}

gridoutput grouper::out()
{
  std::vector<float> left, right, width, bottom, size;
  std::vector<std::string> font, text;
  for(auto& j : allRows)
  {
      text.push_back(utf({j.glyph}));
      left.push_back(j.left);
      right.push_back(j.right);
      size.push_back(j.size);
      bottom.push_back(j.bottom);
      font.push_back(j.font);
      width.push_back(j.right - j.left);
  }
  return gridoutput{left, right, width, bottom, size, font, text };
}


void grouper::findLeftEdges()
{

}

void grouper::findRightEdges()
{

}

void grouper::findMids()
{

}

void grouper::findTops()
{

}

void grouper::findBottoms()
{

}
