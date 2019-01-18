//---------------------------------------------------------------------------//
//                                                                           //
// PDFR corefonts header file                                                //
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

#ifndef PDFR_COREFONTS

//---------------------------------------------------------------------------//

#define PDFR_COREFONTS

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

// This header should compile with only a std::unordered_map #include'd

#include<unordered_map>

// Adding a typedef of uint16_t as Unicode makes it explicit that we are
// referring to the width of glyphs AFTER they have been converted from
// their representation in the pdf string to the intended glyphs.

typedef uint16_t Unicode;

/* This header declares the 14 global objects as 'extern' so that they are
 * available to any file which includes them, but the objects themselves only
 * need to be compiled once.
 *
 * The name of each map is self-explanatory
*/

extern std::unordered_map<Unicode, int> courierwidths;
extern std::unordered_map<Unicode, int> courierboldwidths;
extern std::unordered_map<Unicode, int> courierboldobliquewidths;
extern std::unordered_map<Unicode, int> courierobliquewidths;
extern std::unordered_map<Unicode, int> helveticawidths;
extern std::unordered_map<Unicode, int> helveticaboldwidths;
extern std::unordered_map<Unicode, int> helveticaboldobliquewidths;
extern std::unordered_map<Unicode, int> helveticaobliquewidths;
extern std::unordered_map<Unicode, int> symbolwidths;
extern std::unordered_map<Unicode, int> timesboldwidths;
extern std::unordered_map<Unicode, int> timesbolditalicwidths;
extern std::unordered_map<Unicode, int> timesitalicwidths;
extern std::unordered_map<Unicode, int> timesromanwidths;
extern std::unordered_map<Unicode, int> dingbatswidths;

//---------------------------------------------------------------------------//

#endif
