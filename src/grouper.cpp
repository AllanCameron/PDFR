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

constexpr int EDGECOUNT = 4;

//---------------------------------------------------------------------------//

grouper::grouper(grid* g): theGrid(g)
{
  std::unordered_map<uint8_t, std::vector<GSrow>> gridout = theGrid->output();
  vector<GSrow> tmpvec;
  for(uint8_t i = 0; i < 255; i++)
    concat(tmpvec, gridout[i]);
  concat(tmpvec, gridout[0xff]);
  for(auto& i : tmpvec) if(!i.consumed) allRows.emplace_back(move(i));
  findEdges();
  assignEdges();
  findRightMatch();
};

//---------------------------------------------------------------------------//


std::vector<GSrow> grouper::output()
{
  return allRows;
}

//---------------------------------------------------------------------------//

gridoutput grouper::out()
{
  std::vector<float> left, right, width, bottom, size;
  std::vector<std::string> font, text;
  for(auto& j : allRows)
  {
    if(!j.consumed)
    {
      text.push_back(utf({j.glyph}));
      left.push_back(j.left);
      right.push_back(j.right);
      size.push_back(j.size);
      bottom.push_back(j.bottom);
      font.push_back(j.font);
      width.push_back(j.right - j.left);
    }
  }
  return gridoutput{left, right, width, bottom, size, font, text };
}

//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Mimics the table() function in R. Instantiates a key for every unique member
// of supplied vector, and increments any duplicates.


void grouper::tabulate(const vector<float>& a, unordered_map<int, size_t>& b)
{
  for (auto i : a)
  {
    int j = 10 * i;
    if(b.find(j) == b.end())
      b[j] = 1;
    else b[j]++;
  }
  for(auto& i : getKeys(b))
    if(b[i] < EDGECOUNT)
      b.erase(b.find(i)) ;
}


void grouper::findEdges()
{
  gridoutput GO = theGrid->out();
  tabulate(GO.left, leftEdges);
  tabulate(GO.right, rightEdges);
  vector<float> midvec;
  for(size_t i = 0; i < GO.right.size(); i++)
    midvec.emplace_back((GO.right[i] + GO.left[i])/2);
  tabulate(midvec, mids);
}

void grouper::assignEdges()
{
  for(auto& i : allRows)
  {
    if(leftEdges.find((int) (i.left * 10)) != leftEdges.end())
      i.isLeftEdge = true;
    if(rightEdges.find((int) (i.right * 10)) != rightEdges.end())
      i.isRightEdge = true;
    if(mids.find((int) ((i.right + i.left) * 5)) != mids.end())
      i.isMid = true;
  }
}

void grouper::findRightMatch()
{
  for(auto i = allRows.begin(); i != allRows.end(); i++)
  {
    if(i->isRightEdge || i->isMid || i->consumed) continue;
    for(auto& j : allRows)
    {
      if(j.consumed) continue;
      if(j.left < i->right) continue;
      if(j.bottom - i->bottom > 0.5 * i->size) continue;
      if(i->bottom - j.bottom > 0.5 * i->size) continue;
      if(j.left - i->right > 4 * i->size) continue;
      if(j.isLeftEdge || j.isMid) continue;
      i->glyph.push_back(0x0020);
      if(j.left - i->right > 2 * i->size) i->glyph.push_back(0x0020);
      concat(i->glyph, j.glyph);
      i->right = j.right;
      i->isRightEdge = j.isRightEdge;
      i->width = i->right - i->left;
      j.consumed = true;
      i--;
    }
  }
}
