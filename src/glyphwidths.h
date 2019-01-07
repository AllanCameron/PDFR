//---------------------------------------------------------------------------//
//                                                                           //
// PDFR glyphwidth header file                                               //
//                                                                           //
// Copyright (C) 2019 by Allan Cameron                                       //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining     //
// a copy of this software and associated documentation files                //
// (the "Software"), to deal in the Software without restriction, including  //
// without limitation the rights to use, copy, modify, merge, publish,       //
// distribute, sublicense, and/or sell copies of the Software, and to        //
// permit persons to whom the Software is furnished to do so, subject to     //
// the following conditions:                                                 //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.    //
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY      //
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,      //
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE         //
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                    //
//                                                                           //
//---------------------------------------------------------------------------//


#ifndef PDFR_WIDTH
#define PDFR_WIDTH

#include "pdfr.h"
#include "corefonts.h"
#include "adobetounicode.h"
#include "chartounicode.h"

class glyphwidths
{
private:
  map<RawChar, int> Width;
  document* d;
  font* f;
  dictionary fontref;
  void parsewidtharray(string);
  void getCoreFont(string);
  void parseDescendants(dictionary&, document*);
  void parseWidths(dictionary&, document*);
  void getWidthTable(dictionary&, document*);

public:
  glyphwidths(font*);
  int getwidth(RawChar);
  vector<RawChar> widthKeys();
  bool widthFromCharCodes;
};

#endif
