//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR font implementation file                                            //
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

#include "font.h"


#define DEFAULT_WIDTH 500;
using namespace std;

/*---------------------------------------------------------------------------*/

font::font(document* doc, dictionary Fontref, const string& fontid) :
d(doc), fontref(Fontref), FontID(fontid)
{
  FontRef = "In file";
  BaseFont = fontref.get("/BaseFont");
  getFontName();
  makeGlyphTable();
}

/*---------------------------------------------------------------------------*/

void font::getFontName()
{
  size_t BaseFont_start = 1;
  if(BaseFont.size() > 7)
    if(BaseFont[7] == '+')
      BaseFont_start = 8;
  FontName = BaseFont.substr(BaseFont_start, BaseFont.size() - BaseFont_start);
}

/*---------------------------------------------------------------------------*/

vector<pair<Unicode, int>> font::mapRawChar(vector<RawChar> raw)
{
  vector<pair<Unicode, int>> res;
  for(auto i : raw)
    if(glyphmap.find(i) != glyphmap.end())
      res.push_back(glyphmap[i]);
  return res;
}

/*---------------------------------------------------------------------------*/

void font::makeGlyphTable()
{
  Encoding Enc = Encoding(this->fontref, this->d);
  glyphwidths Wid = glyphwidths(this->fontref, this->d, this->BaseFont);
  vector<RawChar> inkeys = Enc.encKeys();
  if(Wid.widthFromCharCodes)
    for(auto i : inkeys)
      glyphmap[i] = make_pair(Enc.Interpret(i), Wid.getwidth(i));
  else
    for(auto i : inkeys)
      glyphmap[i] = make_pair(Enc.Interpret(i), Wid.getwidth(Enc.Interpret(i)));
}


