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

/* This small header file gives the default glyph widths for the common
 * characters used in the 'built-in' fonts used in pdfs. In theory, later
 * versions of pdf require specification of all glyph widths, but for back-
 * compatability, the widths of the 14 core fonts still need to be defined.
 *
 * All widths are given as ints in "text space", which is 1/1000 of the point
 * size of the text being described.
 *
 * The widths are available as an open online resource from Adobe.
 *
 * To preserve encapsulation, this header is included only by the glyphwidths
 * class. It is only made seperate to reduce clutter in the glyphwidths class,
 * as it consists of a large amount of read-only data and would make
 * gylphwidths.cpp unwieldy to navigate to put all the data in that file too.
 */

#ifndef PDFR_WIDTH
#define PDFR_WIDTH

#include "document.h"

class glyphwidths
{
private:
  std::unordered_map<RawChar, int> Width;
  dictionary fontref;
  document* d;
  void parsewidtharray(std::string);
  void getCoreFont(std::string);
  void parseDescendants(dictionary&, document*);
  void parseWidths(dictionary&, document*);
  void getWidthTable(dictionary&, document*);
  static std::unordered_map<Unicode, int> courierwidths;
  static std::unordered_map<Unicode, int> courierboldwidths;
  static std::unordered_map<Unicode, int> courierboldobliquewidths;
  static std::unordered_map<Unicode, int> courierobliquewidths;
  static std::unordered_map<Unicode, int> helveticawidths;
  static std::unordered_map<Unicode, int> helveticaboldwidths;
  static std::unordered_map<Unicode, int> helveticaboldobliquewidths;
  static std::unordered_map<Unicode, int> helveticaobliquewidths;
  static std::unordered_map<Unicode, int> symbolwidths;
  static std::unordered_map<Unicode, int> timesboldwidths;
  static std::unordered_map<Unicode, int> timesbolditalicwidths;
  static std::unordered_map<Unicode, int> timesitalicwidths;
  static std::unordered_map<Unicode, int> timesromanwidths;
  static std::unordered_map<Unicode, int> dingbatswidths;

public:
  glyphwidths(dictionary& dic, document* doc, std::string bf);
  int getwidth(RawChar);
  std::vector<RawChar> widthKeys();
  bool widthFromCharCodes;
};

#endif
