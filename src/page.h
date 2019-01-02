//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR page header file                                                    //
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


#ifndef PDFR_PAGE
#define PDFR_PAGE

#include "pdfr.h"

class page
{
public:
  page(document& pdfdoc, int pagenum);
  int pagenumber, objectnumber, parent;
  dictionary header, resources, fonts, realfonts;
  std::string contentstring, xobjstring;
  std::map<std::string, std::string> XObjects;
  std::vector<int> resourceobjs, contents;
  std::vector<float> bleedbox, cropbox, mediabox, trimbox, artbox, minbox;
  double rotate;
  std::vector<std::string> fontnames;
  std::vector<bool> hasUnicodeMap;
  std::vector<dictionary> UnicodeMaps, fontrefs;
  std::map<std::string, font> fontmap;

private:
  void parseXObjStream(document& d);
  void boxes();
  void getHeader(document& d);
  void getResources(document& d);
  void getFonts(document& d);
  void getContents(document& d);
};

#endif
