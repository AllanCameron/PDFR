//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR char to Unicode header file                                         //
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

#ifndef PDFR_CHARTOUNICODE

//---------------------------------------------------------------------------//

#define PDFR_CHARTOUNICODE

/* This small header declares three std::unordered_maps as global objects
 * which are required to translate pdf strings to Unicode characters. The
 * translation is a little different for each of the three base encodings which
 * can be declared in the /Encoding entry of a font dictionary.
 *
 * There are actually two base encodings which are not considered here -
 * standardEncoding, which for these purposes is just defined as a straight
 * widechar to Unicode mapping, and /identity-H. The latter means that the
 * encoding is described in a seperate CMap stream. This requires special
 * consideration and is dealt with by the Encoding class.
 *
 * To try to better encapsulate the Encoding class, this file is only directly
 * #included and used by that class.
 */

#include<string>
#include<unordered_map>

// Although these maps are direct uint16_t to uint16_t mappings, it is easier
// to keep track of pre-translation and post-translation code points if we
// give them different type names. It also tips our hat at the attempt to
// encapsulate Encoding's interface

typedef uint16_t Unicode;
typedef uint16_t RawChar;

// The maps themselves are declared extern. This means that the Encoding class
// can #include them without any 'downstream' classes having to recompile the
// actual objects. This therefore speeds up compilation.

extern std::unordered_map<RawChar, Unicode> macRomanEncodingToUnicode;
extern std::unordered_map<RawChar, Unicode> winAnsiEncodingToUnicode;
extern std::unordered_map<RawChar, Unicode> pdfDocEncodingToUnicode;

//---------------------------------------------------------------------------//

#endif
