//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR encodings header file                                               //
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


/* Most strings we want to show will be Ascii-based, but for some higher-byte
 * characters, the actual glyphs intended by the author have to encoded
 * somehow. There are different ways to encode these glyphs as numbers.
 * We therefore need to know the encoding used if we want to recover the correct
 * glyphs from the string. We do this by reading the encoding entry of the
 * fonts dictionary. This allows us to convert directly to a pdf-standard
 * name ("/glyphname") for each character. This can be converted as needed
 * for output on the system running the software.
 *
 * We need to start with a base encoding, if specified in the font dictionary.
 * Sometimes, none is specified in which case we use standard encoding.
 * Sometimes, some or all glyph names and their byte values are given; these
 * supercede the base encoding). Sometimes the encoding is given as /Identity-H
 * which means the encoding is specified in a CMAP.
 *
 * Since this library aims to extract usable text rather than beautiful layout,
 * some glyphs need to be converted to pairs of lower-byte glyphs to make
 * text extraction more useful, particularly the ligatures.
 */

#ifndef PDFR_ENCODE
#define PDFR_ENCODE

#include "pdfr.h"
#include "document.h"
#include "stringfunctions.h"
#include "streams.h"
#include "encodings.h"

enum ENCODING
{
  DEFAULT = 0,
  WINANSI,
  MACROMAN,
  PDFDOC,
  STANDARD
};

static std::map<uint16_t, std::string> ligatures =
{
  {0xFB00, "ff"},
  {0xFB01, "fi"},
  {0xFB02, "fl"},
  {0xFB03, "ffi"},
  {0xFB04, "ffl"},
  {0xFB06, "st"}
};

char UnicodeToChar(uint16_t, ENCODING);

#endif
