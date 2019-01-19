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

//---------------------------------------------------------------------------//

#define PDFR_WIDTH

/* Calculating the width of each glyph is necessary for working out the spacing
 * between letters, words, paragraphs and other text elements, and is one of
 * the more complex tasks in text extraction. The glyph widths in pdf are
 * given in units of text space, where 1000 = 1 point = 1/72 inch in 1-point
 * font size.
 *
 * There are various ways to describe glyph widths in pdf files. The most
 * explicit way is by listing the font widths at each code point after the
 * text string has been decoded, in an array. The array usually starts with the
 * first code point that is being described, then gives a list of numbers for
 * the widths of consecutive code points. Sometimes it is just an array of
 * widths, and the first code point is given seperately in the font
 * dictionary. Sometimes there is a default width for missing glyphs.
 *
 * In older pdfs, the widths may not be specified at all if the font used is
 * one of 14 core fonts in the pdf specification. A conforming reader is
 * supposed to know the glyph widths for these fonts.
 *
 * The glyphwidth class attempts to work out the method used to describe
 * glyph widths and produce a map of the intended glyphs to their intended
 * widths, without bothering any other classes with its implementation.
 *
 * Among the tools it needs to do this, it requires navigating the document,
 * reading dictionaries and streams, and parsing a width description array.
 * It therefore needs the document.h header which wraps these capabilities.
 *
 * It also needs a group of static objects listing the widths of each of the
 * characters used in the 'built-in' fonts used in pdfs. In theory, later
 * versions of pdf require specification of all glyph widths, but for back-
 * compatability, the widths of the 14 core fonts still need to be defined.
 *
 * The widths are available as an open online resource from Adobe.
 *
 * To preserve encapsulation, this header is included only by the fonts
 * class. The fonts class merges its width map with the encoding map to
 * produce the glyphmap, which gives the intended Unicode code point and
 * width as a paired value for any given input character in a pdf string.
 */

//---------------------------------------------------------------------------//

#include "document.h"

//---------------------------------------------------------------------------//
// The glyphwidths class contains private methods to find the description of
// widths for each character in a font. It only makes sense to the font class,
// from whence it is created and accessed.
//
// The core font widths are declared static private because they are only
// needed by this class, and we don't want an extra copy of all of them if
// several fonts are created. This also prevents them polluting the global
// namespace

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
