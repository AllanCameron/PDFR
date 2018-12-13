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

struct font
{
  std::string FontRef, FontName, FontID;
  std::vector<int> FontBBox;
  std::string BaseEncoding;
  GlyphMap glyphmap;

  std::map<std::string, std::string> UnicodeMap;
  bool hasUnicodeMap, hasMappings;
  std::map<uint16_t, int> Width;
  EncMap EncodingMap;
  std::map<std::string, std::string> mappings;
  void mapUnicode(dictionary& dict, document& d);
  void getEncoding(dictionary& fontref, document& d);
  void getWidthTable(dictionary& dict, document& d);
  void getCoreFont(std::string s);
  void makeGlyphTable();
  void parsewidtharray(std::string s);
  std::vector<std::pair<std::string, int>> mapString(const std::string& s);
  font(document& doc, const dictionary& fontref, const std::string& fontid);
  font(){};
};

#endif