//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR font header file                                                    //
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

#ifndef PDFR_FONT
#define PDFR_FONT

#include "pdfr.h"
#include "Rex.h"

using namespace std;

struct font
{
  string FontRef, FontName, FontID;
  vector<int> FontBBox;
  string BaseEncoding;
  GlyphMap glyphmap;
  string BaseFont;
  bool hasUnicodeMap, hasMappings;
  map<uint16_t, int> Width;
  map<uint16_t, uint16_t> EncodingMap;
  void mapUnicode(dictionary&, document&);
  void getEncoding(dictionary&, document&);
  void getWidthTable(dictionary&, document&);
  void getCoreFont(string);
  void makeGlyphTable();
  void getFontName();
  void parsewidtharray(string);
  void processUnicodeChars(Rex&);
  void processUnicodeRange(Rex&);
  vector<pair<uint16_t, int>> mapString(const string&);
  font(document&, dictionary, const string&);
  font(){};
};

#endif
