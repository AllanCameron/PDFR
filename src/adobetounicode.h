//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR adobe names to unicode header file                                  //
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

#ifndef PDFR_ADOBETOUNICODE

//---------------------------------------------------------------------------//

#define PDFR_ADOBETOUNICODE

/* This is a very small header file to declare a single large global object -
 * an unordered map of all the Adobe glyph names and their corresponding
 * Unicode code points ('Unicode' type in the declaration is just a synonym for
 * uint16_t).
 *
 * This object is only required for one stage of the pdf reading process:
 * interpreting the "/Differences" entry of a font's encoding dictionary.
 *
 * The /Differences entry describes a mapping of single-byte (or sometimes two-
 * byte) characters from a pdf stream to the intended Adobe glyph. The most
 * portable way to deal with this is to convert the glyphs to Unicode code
 * points. These can then be output safely to a variety of systems as needed.
 *
 * The conversions from Adobe code points to Unicode are widely available as
 * open source resources online.
 */

#include<string>
#include<unordered_map>

// Make the fact that the mapped values represent Unicode specific by typedef
typedef uint16_t Unicode;

// The map is declared 'extern' as a promise to the linker that this global
// object is defined elsewhere but can be used by any file that includes this
// header file. If the whole object was defined in the header, it would slow
// down compilation of any files that #included it since it is 123 KB in size.

extern std::unordered_map<std::string, Unicode> AdobeToUnicode;

//---------------------------------------------------------------------------//

#endif

