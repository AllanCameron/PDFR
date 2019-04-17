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

/* This is the joint 6th in a series of daisy-chained headers that build up the
 * tools to read and parse pdfs. It is logically paired with encoding.h
 * in that they both come after document.h and together form the basis for the
 * next step, which is font creation.
 *
 * Calculating the width of each glyph is necessary for working out the spacing
 * between letters, words, paragraphs and other text elements. The glyph widths
 * in pdf are given in units of text space, where 1000 = 1 point = 1/72 inch in
 * 1-point font size.
 *
 * Getting the glyph widths is one of the more complex tasks in extracting text,
 * since there are various ways for pdf files to describe them. The most
 * explicit way is by listing the font widths at each code point in an array.
 * The array is preceeded by the first code point that is being described,
 * then the array itself comprises numbers for the widths of sequential code
 * points. Often there are several consecutive arrays like this specifying
 * groups of sequential code points. Sometimes the entry is just an array of
 * widths, and the first code point is given seperately in the font
 * dictionary. Sometimes there is a default width for missing glyphs. Sometimes
 * the width array is in the font dictionary; sometimes it is in a descendant
 * font dictionary; other times it is in an encoded stream; still other times
 * it comprises an entire non-dictionary object on its own.
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
 * It therefore needs the document.h header which wraps most of these
 * capabilities. The class defines its own lexer for interpreting the special
 * width arrays.
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
// namespace.

class glyphwidths
{
public:
  // Constructor
  glyphwidths(dictionary& dic, std::shared_ptr<document> doc);

  // public methods
  int getwidth(const RawChar&);                // Main data lookup
  std::vector<RawChar> widthKeys();            // Returns all map keys
  bool widthsAreForRaw();                      // returns widthfromcharcodes

private:
  // private data
  std::unordered_map<RawChar, int> Width;         // The main data member
  dictionary fontref;                             // the font dictionary
  std::shared_ptr<document> d;                    // pointer to document
  std::string basefont;                           // the base font (if any)
  bool widthFromCharCodes;                        // are widths for code points?

  // private methods
  void parsewidtharray(const std::string&);       // lexer
  void getCoreFont();                             // corefont getter
  void parseDescendants();                        // read descendant dictionary
  void parseWidths();                             // parse the width array
  void getWidthTable();                           // co-ordinates creation

//-- The core fonts as defined in corefonts.cpp ------------------------//
                                                                        //
  static std::unordered_map<Unicode, int> courierwidths;                //
  static std::unordered_map<Unicode, int> helveticawidths;              //
  static std::unordered_map<Unicode, int> helveticaboldwidths;          //
  static std::unordered_map<Unicode, int> symbolwidths;                 //
  static std::unordered_map<Unicode, int> timesboldwidths;              //
  static std::unordered_map<Unicode, int> timesbolditalicwidths;        //
  static std::unordered_map<Unicode, int> timesitalicwidths;            //
  static std::unordered_map<Unicode, int> timesromanwidths;             //
  static std::unordered_map<Unicode, int> dingbatswidths;               //
                                                                        //
//----------------------------------------------------------------------//
};

//---------------------------------------------------------------------------//

#endif
