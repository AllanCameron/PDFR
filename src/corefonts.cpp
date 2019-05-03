//---------------------------------------------------------------------------//
//                                                                           //
// PDFR core fonts implementation file                                       //
//                                                                           //
// Copyright (C) 2018 by Allan Cameron                                       //
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

#include "glyphwidths.h"

/* This file defines characteristics of the 14 core or built-in pdf fonts.
 * These are necessary for backwards-compatability with earlier versions of the
 * pdf standard which did not mandate specifying all character widths in a pdf
 * file.
 *
 * The following data by their nature are not really human-readable and can
 * only be checked by testing their output
 */

//---------------------------------------------------------------------------//
// The pdf font files include a 4-point vector for each font defining a
// bounding-box in text space. At present bounding boxes are not used in the
// program. However, the vectors are included here in case they are required
// for future feature development. The names are self-explanatory

static std::vector<float> courier_box                = {-23,  -250, 715,   805};
static std::vector<float> courier_bold_oblique_box   = {-57,  -250, 869,   801};
static std::vector<float> courier_bold_box           = {-113, -250, 749,   801};
static std::vector<float> courier_oblique_box        = {-27,  -250, 849,   805};
static std::vector<float> helvetica_box              = {-166, -225, 1000,  931};
static std::vector<float> helvetica_bold_box         = {-170, -228, 1003,  962};
static std::vector<float> helvetica_bold_oblique_box = {-174, -228, 1114,  962};
static std::vector<float> helvetica_oblique_box      = {-170, -225, 1116,  931};
static std::vector<float> symbol_box                 = {-180, -293, 1090, 1010};
static std::vector<float> times_bold_box             = {-168, -218, 1000,  935};
static std::vector<float> times_bold_italic_box      = {-200, -218, 996,   921};
static std::vector<float> times_italic_box           = {-169, -217, 1010,  883};
static std::vector<float> times_roman_box            = {-168, -218, 1000,  898};
static std::vector<float> dingbats_box               = {-1,   -143, 981,   820};

//---------------------------------------------------------------------------//
// Courier is a monospaced font and it seems a bit silly to specifically declare
// all its widths manually. I have done so here for consistency and simplicity
// rather than defining a loop to create the widths. In any case, the code
// points are not all contiguous so a simple loop would require further coding
// to make it an accurate and memory-minimal map. I thought it simplest to just
// define the whole map for Courier and copy that map to the Courier variants
// which are all also of the same width and map the same code points.

std::unordered_map<Unicode, int> glyphwidths::courier_widths =
{
  {0x0020, 0x0258}, {0x0021, 0x0258}, {0x0022, 0x0258}, {0x0023, 0x0258},
  {0x0024, 0x0258}, {0x0025, 0x0258}, {0x0026, 0x0258}, {0x0027, 0x0258},
  {0x0028, 0x0258}, {0x0029, 0x0258}, {0x002a, 0x0258}, {0x002b, 0x0258},
  {0x002c, 0x0258}, {0x002d, 0x0258}, {0x002e, 0x0258}, {0x002f, 0x0258},
  {0x0030, 0x0258}, {0x0031, 0x0258}, {0x0032, 0x0258}, {0x0033, 0x0258},
  {0x0034, 0x0258}, {0x0035, 0x0258}, {0x0036, 0x0258}, {0x0037, 0x0258},
  {0x0038, 0x0258}, {0x0039, 0x0258}, {0x003a, 0x0258}, {0x003b, 0x0258},
  {0x003c, 0x0258}, {0x003d, 0x0258}, {0x003e, 0x0258}, {0x003f, 0x0258},
  {0x0040, 0x0258}, {0x0041, 0x0258}, {0x0042, 0x0258}, {0x0043, 0x0258},
  {0x0044, 0x0258}, {0x0045, 0x0258}, {0x0046, 0x0258}, {0x0047, 0x0258},
  {0x0048, 0x0258}, {0x0049, 0x0258}, {0x004a, 0x0258}, {0x004b, 0x0258},
  {0x004c, 0x0258}, {0x004d, 0x0258}, {0x004e, 0x0258}, {0x004f, 0x0258},
  {0x0050, 0x0258}, {0x0051, 0x0258}, {0x0052, 0x0258}, {0x0053, 0x0258},
  {0x0054, 0x0258}, {0x0055, 0x0258}, {0x0056, 0x0258}, {0x0057, 0x0258},
  {0x0058, 0x0258}, {0x0059, 0x0258}, {0x005a, 0x0258}, {0x005b, 0x0258},
  {0x005c, 0x0258}, {0x005d, 0x0258}, {0x005e, 0x0258}, {0x005f, 0x0258},
  {0x0060, 0x0258}, {0x0061, 0x0258}, {0x0062, 0x0258}, {0x0063, 0x0258},
  {0x0064, 0x0258}, {0x0065, 0x0258}, {0x0066, 0x0258}, {0x0067, 0x0258},
  {0x0068, 0x0258}, {0x0069, 0x0258}, {0x006a, 0x0258}, {0x006b, 0x0258},
  {0x006c, 0x0258}, {0x006d, 0x0258}, {0x006e, 0x0258}, {0x006f, 0x0258},
  {0x0070, 0x0258}, {0x0071, 0x0258}, {0x0072, 0x0258}, {0x0073, 0x0258},
  {0x0074, 0x0258}, {0x0075, 0x0258}, {0x0076, 0x0258}, {0x0077, 0x0258},
  {0x0078, 0x0258}, {0x0079, 0x0258}, {0x007a, 0x0258}, {0x007b, 0x0258},
  {0x007c, 0x0258}, {0x007d, 0x0258}, {0x007e, 0x0258}, {0x00a1, 0x0258},
  {0x00a2, 0x0258}, {0x00a3, 0x0258}, {0x00a4, 0x0258}, {0x00a5, 0x0258},
  {0x00a6, 0x0258}, {0x00a7, 0x0258}, {0x00a8, 0x0258}, {0x00a9, 0x0258},
  {0x00aa, 0x0258}, {0x00ab, 0x0258}, {0x00ac, 0x0258}, {0x00ad, 0x0258},
  {0x00ae, 0x0258}, {0x00af, 0x0258}, {0x00b1, 0x0258}, {0x00b2, 0x0258},
  {0x00b3, 0x0258}, {0x00b4, 0x0258}, {0x00b6, 0x0258}, {0x00b7, 0x0258},
  {0x00b8, 0x0258}, {0x00b9, 0x0258}, {0x00ba, 0x0258}, {0x00bb, 0x0258},
  {0x00bc, 0x0258}, {0x00bd, 0x0258}, {0x00bf, 0x0258}, {0x00c1, 0x0258},
  {0x00c2, 0x0258}, {0x00c3, 0x0258}, {0x00c4, 0x0258}, {0x00c5, 0x0258},
  {0x00c6, 0x0258}, {0x00c7, 0x0258}, {0x00c8, 0x0258}, {0x00ca, 0x0258},
  {0x00cb, 0x0258}, {0x00cd, 0x0258}, {0x00ce, 0x0258}, {0x00cf, 0x0258},
  {0x00d0, 0x0258}, {0x00e1, 0x0258}, {0x00e3, 0x0258}, {0x00e8, 0x0258},
  {0x00e9, 0x0258}, {0x00ea, 0x0258}, {0x00eb, 0x0258}, {0x00f1, 0x0258},
  {0x00f5, 0x0258}, {0x00f8, 0x0258}, {0x00f9, 0x0258}, {0x00fa, 0x0258},
  {0x00fb, 0x0258}
};

//---------------------------------------------------------------------------//
// There were no obvious shortcuts to simply defining the remaining widths
// manually. It can be done in less space but would involve numerous
// substitutions and be error-prone. Safest to just define them character
// by character (this was obviously automated and the numbers lifted directly
// from the font description files then translated to 0xffff format)
//
// The data themselves are not really human-readable, so any problems will only
// be found by testing the output.

std::unordered_map<Unicode, int> glyphwidths::helvetica_widths =
{
  {0x0020, 0x0116}, {0x0021, 0x0116}, {0x0022, 0x0163}, {0x0023, 0x022c},
  {0x0024, 0x022c}, {0x0025, 0x0379}, {0x0026, 0x029b}, {0x0027, 0x00de},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x0185}, {0x002b, 0x0248},
  {0x002c, 0x0116}, {0x002d, 0x014d}, {0x002e, 0x0116}, {0x002f, 0x0116},
  {0x0030, 0x022c}, {0x0031, 0x022c}, {0x0032, 0x022c}, {0x0033, 0x022c},
  {0x0034, 0x022c}, {0x0035, 0x022c}, {0x0036, 0x022c}, {0x0037, 0x022c},
  {0x0038, 0x022c}, {0x0039, 0x022c}, {0x003a, 0x0116}, {0x003b, 0x0116},
  {0x003c, 0x0248}, {0x003d, 0x0248}, {0x003e, 0x0248}, {0x003f, 0x022c},
  {0x0040, 0x03f7}, {0x0041, 0x029b}, {0x0042, 0x029b}, {0x0043, 0x02d2},
  {0x0044, 0x02d2}, {0x0045, 0x029b}, {0x0046, 0x0263}, {0x0047, 0x030a},
  {0x0048, 0x02d2}, {0x0049, 0x0116}, {0x004a, 0x01f4}, {0x004b, 0x029b},
  {0x004c, 0x022c}, {0x004d, 0x0341}, {0x004e, 0x02d2}, {0x004f, 0x030a},
  {0x0050, 0x029b}, {0x0051, 0x030a}, {0x0052, 0x02d2}, {0x0053, 0x029b},
  {0x0054, 0x0263}, {0x0055, 0x02d2}, {0x0056, 0x029b}, {0x0057, 0x03b0},
  {0x0058, 0x029b}, {0x0059, 0x029b}, {0x005a, 0x0263}, {0x005b, 0x0116},
  {0x005c, 0x0116}, {0x005d, 0x0116}, {0x005e, 0x01d5}, {0x005f, 0x022c},
  {0x0060, 0x00de}, {0x0061, 0x022c}, {0x0062, 0x022c}, {0x0063, 0x01f4},
  {0x0064, 0x022c}, {0x0065, 0x022c}, {0x0066, 0x0116}, {0x0067, 0x022c},
  {0x0068, 0x022c}, {0x0069, 0x00de}, {0x006a, 0x00de}, {0x006b, 0x01f4},
  {0x006c, 0x00de}, {0x006d, 0x0341}, {0x006e, 0x022c}, {0x006f, 0x022c},
  {0x0070, 0x022c}, {0x0071, 0x022c}, {0x0072, 0x014d}, {0x0073, 0x01f4},
  {0x0074, 0x0116}, {0x0075, 0x022c}, {0x0076, 0x01f4}, {0x0077, 0x02d2},
  {0x0078, 0x01f4}, {0x0079, 0x01f4}, {0x007a, 0x01f4}, {0x007b, 0x014e},
  {0x007c, 0x0104}, {0x007d, 0x014e}, {0x007e, 0x0248}, {0x00a1, 0x014d},
  {0x00a2, 0x022c}, {0x00a3, 0x022c}, {0x00a4, 0x00a7}, {0x00a5, 0x022c},
  {0x00a6, 0x022c}, {0x00a7, 0x022c}, {0x00a8, 0x022c}, {0x00a9, 0x00bf},
  {0x00aa, 0x014d}, {0x00ab, 0x022c}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x01f4}, {0x00af, 0x01f4}, {0x00b1, 0x022c}, {0x00b2, 0x022c},
  {0x00b3, 0x022c}, {0x00b4, 0x0116}, {0x00b6, 0x0219}, {0x00b7, 0x015e},
  {0x00b8, 0x00de}, {0x00b9, 0x014d}, {0x00ba, 0x014d}, {0x00bb, 0x022c},
  {0x00bc, 0x03e8}, {0x00bd, 0x03e8}, {0x00bf, 0x0263}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x03e8}, {0x00e1, 0x03e8}, {0x00e3, 0x0172}, {0x00e8, 0x022c},
  {0x00e9, 0x030a}, {0x00ea, 0x03e8}, {0x00eb, 0x016d}, {0x00f1, 0x0379},
  {0x00f5, 0x0116}, {0x00f8, 0x00de}, {0x00f9, 0x0263}, {0x00fa, 0x03b0},
  {0x00fb, 0x0263}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::helvetica_bold_widths =
{
  {0x0020, 0x0116}, {0x0021, 0x014d}, {0x0022, 0x01da}, {0x0023, 0x022c},
  {0x0024, 0x022c}, {0x0025, 0x0379}, {0x0026, 0x02d2}, {0x0027, 0x0116},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x0185}, {0x002b, 0x0248},
  {0x002c, 0x0116}, {0x002d, 0x014d}, {0x002e, 0x0116}, {0x002f, 0x0116},
  {0x0030, 0x022c}, {0x0031, 0x022c}, {0x0032, 0x022c}, {0x0033, 0x022c},
  {0x0034, 0x022c}, {0x0035, 0x022c}, {0x0036, 0x022c}, {0x0037, 0x022c},
  {0x0038, 0x022c}, {0x0039, 0x022c}, {0x003a, 0x014d}, {0x003b, 0x014d},
  {0x003c, 0x0248}, {0x003d, 0x0248}, {0x003e, 0x0248}, {0x003f, 0x0263},
  {0x0040, 0x03cf}, {0x0041, 0x02d2}, {0x0042, 0x02d2}, {0x0043, 0x02d2},
  {0x0044, 0x02d2}, {0x0045, 0x029b}, {0x0046, 0x0263}, {0x0047, 0x030a},
  {0x0048, 0x02d2}, {0x0049, 0x0116}, {0x004a, 0x022c}, {0x004b, 0x02d2},
  {0x004c, 0x0263}, {0x004d, 0x0341}, {0x004e, 0x02d2}, {0x004f, 0x030a},
  {0x0050, 0x029b}, {0x0051, 0x030a}, {0x0052, 0x02d2}, {0x0053, 0x029b},
  {0x0054, 0x0263}, {0x0055, 0x02d2}, {0x0056, 0x029b}, {0x0057, 0x03b0},
  {0x0058, 0x029b}, {0x0059, 0x029b}, {0x005a, 0x0263}, {0x005b, 0x014d},
  {0x005c, 0x0116}, {0x005d, 0x014d}, {0x005e, 0x0248}, {0x005f, 0x022c},
  {0x0060, 0x0116}, {0x0061, 0x022c}, {0x0062, 0x0263}, {0x0063, 0x022c},
  {0x0064, 0x0263}, {0x0065, 0x022c}, {0x0066, 0x014d}, {0x0067, 0x0263},
  {0x0068, 0x0263}, {0x0069, 0x0116}, {0x006a, 0x0116}, {0x006b, 0x022c},
  {0x006c, 0x0116}, {0x006d, 0x0379}, {0x006e, 0x0263}, {0x006f, 0x0263},
  {0x0070, 0x0263}, {0x0071, 0x0263}, {0x0072, 0x0185}, {0x0073, 0x022c},
  {0x0074, 0x014d}, {0x0075, 0x0263}, {0x0076, 0x022c}, {0x0077, 0x030a},
  {0x0078, 0x022c}, {0x0079, 0x022c}, {0x007a, 0x01f4}, {0x007b, 0x0185},
  {0x007c, 0x0118}, {0x007d, 0x0185}, {0x007e, 0x0248}, {0x00a1, 0x014d},
  {0x00a2, 0x022c}, {0x00a3, 0x022c}, {0x00a4, 0x00a7}, {0x00a5, 0x022c},
  {0x00a6, 0x022c}, {0x00a7, 0x022c}, {0x00a8, 0x022c}, {0x00a9, 0x00ee},
  {0x00aa, 0x01f4}, {0x00ab, 0x022c}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x0263}, {0x00af, 0x0263}, {0x00b1, 0x022c}, {0x00b2, 0x022c},
  {0x00b3, 0x022c}, {0x00b4, 0x0116}, {0x00b6, 0x022c}, {0x00b7, 0x015e},
  {0x00b8, 0x0116}, {0x00b9, 0x01f4}, {0x00ba, 0x01f4}, {0x00bb, 0x022c},
  {0x00bc, 0x03e8}, {0x00bd, 0x03e8}, {0x00bf, 0x0263}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x03e8}, {0x00e1, 0x03e8}, {0x00e3, 0x0172}, {0x00e8, 0x0263},
  {0x00e9, 0x030a}, {0x00ea, 0x03e8}, {0x00eb, 0x016d}, {0x00f1, 0x0379},
  {0x00f5, 0x0116}, {0x00f8, 0x0116}, {0x00f9, 0x0263}, {0x00fa, 0x03b0},
  {0x00fb, 0x0263}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::symbol_widths =
{
  {0x0020, 0x00fa}, {0x0021, 0x014d}, {0x0022, 0x02c9}, {0x0023, 0x01f4},
  {0x0024, 0x0225}, {0x0025, 0x0341}, {0x0026, 0x030a}, {0x0027, 0x01b7},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x01f4}, {0x002b, 0x0225},
  {0x002c, 0x00fa}, {0x002d, 0x0225}, {0x002e, 0x00fa}, {0x002f, 0x0116},
  {0x0030, 0x01f4}, {0x0031, 0x01f4}, {0x0032, 0x01f4}, {0x0033, 0x01f4},
  {0x0034, 0x01f4}, {0x0035, 0x01f4}, {0x0036, 0x01f4}, {0x0037, 0x01f4},
  {0x0038, 0x01f4}, {0x0039, 0x01f4}, {0x003a, 0x0116}, {0x003b, 0x0116},
  {0x003c, 0x0225}, {0x003d, 0x0225}, {0x003e, 0x0225}, {0x003f, 0x01bc},
  {0x0040, 0x0225}, {0x0041, 0x02d2}, {0x0042, 0x029b}, {0x0043, 0x02d2},
  {0x0044, 0x0264}, {0x0045, 0x0263}, {0x0046, 0x02fb}, {0x0047, 0x025b},
  {0x0048, 0x02d2}, {0x0049, 0x014d}, {0x004a, 0x0277}, {0x004b, 0x02d2},
  {0x004c, 0x02ae}, {0x004d, 0x0379}, {0x004e, 0x02d2}, {0x004f, 0x02d2},
  {0x0050, 0x0300}, {0x0051, 0x02e5}, {0x0052, 0x022c}, {0x0053, 0x0250},
  {0x0054, 0x0263}, {0x0055, 0x02b2}, {0x0056, 0x01b7}, {0x0057, 0x0300},
  {0x0058, 0x0285}, {0x0059, 0x031b}, {0x005a, 0x0263}, {0x005b, 0x014d},
  {0x005c, 0x035f}, {0x005d, 0x014d}, {0x005e, 0x0292}, {0x005f, 0x01f4},
  {0x0060, 0x01f4}, {0x0061, 0x0277}, {0x0062, 0x0225}, {0x0063, 0x0225},
  {0x0064, 0x01ee}, {0x0065, 0x01b7}, {0x0066, 0x0209}, {0x0067, 0x019b},
  {0x0068, 0x025b}, {0x0069, 0x0149}, {0x006a, 0x025b}, {0x006b, 0x0225},
  {0x006c, 0x0225}, {0x006d, 0x0240}, {0x006e, 0x0209}, {0x006f, 0x0225},
  {0x0070, 0x0225}, {0x0071, 0x0209}, {0x0072, 0x0225}, {0x0073, 0x025b},
  {0x0074, 0x01b7}, {0x0075, 0x0240}, {0x0076, 0x02c9}, {0x0077, 0x02ae},
  {0x0078, 0x01ed}, {0x0079, 0x02ae}, {0x007a, 0x01ee}, {0x007b, 0x01e0},
  {0x007c, 0x00c8}, {0x007d, 0x01e0}, {0x007e, 0x0225}, {0x00a0, 0x02ee},
  {0x00a1, 0x026c}, {0x00a2, 0x00f7}, {0x00a3, 0x0225}, {0x00a4, 0x00a7},
  {0x00a5, 0x02c9}, {0x00a6, 0x01f4}, {0x00a7, 0x02f1}, {0x00a8, 0x02f1},
  {0x00a9, 0x02f1}, {0x00aa, 0x02f1}, {0x00ab, 0x0412}, {0x00ac, 0x03db},
  {0x00ad, 0x025b}, {0x00ae, 0x03db}, {0x00af, 0x025b}, {0x00b0, 0x0190},
  {0x00b1, 0x0225}, {0x00b2, 0x019b}, {0x00b3, 0x0225}, {0x00b4, 0x0225},
  {0x00b5, 0x02c9}, {0x00b6, 0x01ee}, {0x00b7, 0x01cc}, {0x00b8, 0x0225},
  {0x00b9, 0x0225}, {0x00ba, 0x0225}, {0x00bb, 0x0225}, {0x00bc, 0x03e8},
  {0x00bd, 0x025b}, {0x00be, 0x03e8}, {0x00bf, 0x0292}, {0x00c0, 0x0337},
  {0x00c1, 0x02ae}, {0x00c2, 0x031b}, {0x00c3, 0x03db}, {0x00c4, 0x0300},
  {0x00c5, 0x0300}, {0x00c6, 0x0337}, {0x00c7, 0x0300}, {0x00c8, 0x0300},
  {0x00c9, 0x02c9}, {0x00ca, 0x02c9}, {0x00cb, 0x02c9}, {0x00cc, 0x02c9},
  {0x00cd, 0x02c9}, {0x00ce, 0x02c9}, {0x00cf, 0x02c9}, {0x00d0, 0x0300},
  {0x00d1, 0x02c9}, {0x00d2, 0x0316}, {0x00d3, 0x0316}, {0x00d4, 0x037a},
  {0x00d5, 0x0337}, {0x00d6, 0x0225}, {0x00d7, 0x00fa}, {0x00d8, 0x02c9},
  {0x00d9, 0x025b}, {0x00da, 0x025b}, {0x00db, 0x0412}, {0x00dc, 0x03db},
  {0x00dd, 0x025b}, {0x00de, 0x03db}, {0x00df, 0x025b}, {0x00e0, 0x01ee},
  {0x00e1, 0x0149}, {0x00e2, 0x0316}, {0x00e3, 0x0316}, {0x00e4, 0x0312},
  {0x00e5, 0x02c9}, {0x00e6, 0x0180}, {0x00e7, 0x0180}, {0x00e8, 0x0180},
  {0x00e9, 0x0180}, {0x00ea, 0x0180}, {0x00eb, 0x0180}, {0x00ec, 0x01ee},
  {0x00ed, 0x01ee}, {0x00ee, 0x01ee}, {0x00ef, 0x01ee}, {0x00f1, 0x0149},
  {0x00f2, 0x0112}, {0x00f3, 0x02ae}, {0x00f4, 0x02ae}, {0x00f5, 0x02ae},
  {0x00f6, 0x0180}, {0x00f7, 0x0180}, {0x00f8, 0x0180}, {0x00f9, 0x0180},
  {0x00fa, 0x0180}, {0x00fb, 0x0180}, {0x00fc, 0x01ee}, {0x00fd, 0x01ee},
  {0x00fe, 0x01ee}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::times_bold_widths =
{
  {0x0020, 0x00fa}, {0x0021, 0x014d}, {0x0022, 0x022b}, {0x0023, 0x01f4},
  {0x0024, 0x01f4}, {0x0025, 0x03e8}, {0x0026, 0x0341}, {0x0027, 0x014d},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x01f4}, {0x002b, 0x023a},
  {0x002c, 0x00fa}, {0x002d, 0x014d}, {0x002e, 0x00fa}, {0x002f, 0x0116},
  {0x0030, 0x01f4}, {0x0031, 0x01f4}, {0x0032, 0x01f4}, {0x0033, 0x01f4},
  {0x0034, 0x01f4}, {0x0035, 0x01f4}, {0x0036, 0x01f4}, {0x0037, 0x01f4},
  {0x0038, 0x01f4}, {0x0039, 0x01f4}, {0x003a, 0x014d}, {0x003b, 0x014d},
  {0x003c, 0x023a}, {0x003d, 0x023a}, {0x003e, 0x023a}, {0x003f, 0x01f4},
  {0x0040, 0x03a2}, {0x0041, 0x02d2}, {0x0042, 0x029b}, {0x0043, 0x02d2},
  {0x0044, 0x02d2}, {0x0045, 0x029b}, {0x0046, 0x0263}, {0x0047, 0x030a},
  {0x0048, 0x030a}, {0x0049, 0x0185}, {0x004a, 0x01f4}, {0x004b, 0x030a},
  {0x004c, 0x029b}, {0x004d, 0x03b0}, {0x004e, 0x02d2}, {0x004f, 0x030a},
  {0x0050, 0x0263}, {0x0051, 0x030a}, {0x0052, 0x02d2}, {0x0053, 0x022c},
  {0x0054, 0x029b}, {0x0055, 0x02d2}, {0x0056, 0x02d2}, {0x0057, 0x03e8},
  {0x0058, 0x02d2}, {0x0059, 0x02d2}, {0x005a, 0x029b}, {0x005b, 0x014d},
  {0x005c, 0x0116}, {0x005d, 0x014d}, {0x005e, 0x0245}, {0x005f, 0x01f4},
  {0x0060, 0x014d}, {0x0061, 0x01f4}, {0x0062, 0x022c}, {0x0063, 0x01bc},
  {0x0064, 0x022c}, {0x0065, 0x01bc}, {0x0066, 0x014d}, {0x0067, 0x01f4},
  {0x0068, 0x022c}, {0x0069, 0x0116}, {0x006a, 0x014d}, {0x006b, 0x022c},
  {0x006c, 0x0116}, {0x006d, 0x0341}, {0x006e, 0x022c}, {0x006f, 0x01f4},
  {0x0070, 0x022c}, {0x0071, 0x022c}, {0x0072, 0x01bc}, {0x0073, 0x0185},
  {0x0074, 0x014d}, {0x0075, 0x022c}, {0x0076, 0x01f4}, {0x0077, 0x02d2},
  {0x0078, 0x01f4}, {0x0079, 0x01f4}, {0x007a, 0x01bc}, {0x007b, 0x018a},
  {0x007c, 0x00dc}, {0x007d, 0x018a}, {0x007e, 0x0208}, {0x00a1, 0x014d},
  {0x00a2, 0x01f4}, {0x00a3, 0x01f4}, {0x00a4, 0x00a7}, {0x00a5, 0x01f4},
  {0x00a6, 0x01f4}, {0x00a7, 0x01f4}, {0x00a8, 0x01f4}, {0x00a9, 0x0116},
  {0x00aa, 0x01f4}, {0x00ab, 0x01f4}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x022c}, {0x00af, 0x022c}, {0x00b1, 0x01f4}, {0x00b2, 0x01f4},
  {0x00b3, 0x01f4}, {0x00b4, 0x00fa}, {0x00b6, 0x021c}, {0x00b7, 0x015e},
  {0x00b8, 0x014d}, {0x00b9, 0x01f4}, {0x00ba, 0x01f4}, {0x00bb, 0x01f4},
  {0x00bc, 0x03e8}, {0x00bd, 0x03e8}, {0x00bf, 0x01f4}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x03e8}, {0x00e1, 0x03e8}, {0x00e3, 0x012c}, {0x00e8, 0x029b},
  {0x00e9, 0x030a}, {0x00ea, 0x03e8}, {0x00eb, 0x014a}, {0x00f1, 0x02d2},
  {0x00f5, 0x0116}, {0x00f8, 0x0116}, {0x00f9, 0x01f4}, {0x00fa, 0x02d2},
  {0x00fb, 0x022c}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::times_bold_italic_widths =
{
  {0x0020, 0x00fa}, {0x0021, 0x0185}, {0x0022, 0x022b}, {0x0023, 0x01f4},
  {0x0024, 0x01f4}, {0x0025, 0x0341}, {0x0026, 0x030a}, {0x0027, 0x014d},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x01f4}, {0x002b, 0x023a},
  {0x002c, 0x00fa}, {0x002d, 0x014d}, {0x002e, 0x00fa}, {0x002f, 0x0116},
  {0x0030, 0x01f4}, {0x0031, 0x01f4}, {0x0032, 0x01f4}, {0x0033, 0x01f4},
  {0x0034, 0x01f4}, {0x0035, 0x01f4}, {0x0036, 0x01f4}, {0x0037, 0x01f4},
  {0x0038, 0x01f4}, {0x0039, 0x01f4}, {0x003a, 0x014d}, {0x003b, 0x014d},
  {0x003c, 0x023a}, {0x003d, 0x023a}, {0x003e, 0x023a}, {0x003f, 0x01f4},
  {0x0040, 0x0340}, {0x0041, 0x029b}, {0x0042, 0x029b}, {0x0043, 0x029b},
  {0x0044, 0x02d2}, {0x0045, 0x029b}, {0x0046, 0x029b}, {0x0047, 0x02d2},
  {0x0048, 0x030a}, {0x0049, 0x0185}, {0x004a, 0x01f4}, {0x004b, 0x029b},
  {0x004c, 0x0263}, {0x004d, 0x0379}, {0x004e, 0x02d2}, {0x004f, 0x02d2},
  {0x0050, 0x0263}, {0x0051, 0x02d2}, {0x0052, 0x029b}, {0x0053, 0x022c},
  {0x0054, 0x0263}, {0x0055, 0x02d2}, {0x0056, 0x029b}, {0x0057, 0x0379},
  {0x0058, 0x029b}, {0x0059, 0x0263}, {0x005a, 0x0263}, {0x005b, 0x014d},
  {0x005c, 0x0116}, {0x005d, 0x014d}, {0x005e, 0x023a}, {0x005f, 0x01f4},
  {0x0060, 0x014d}, {0x0061, 0x01f4}, {0x0062, 0x01f4}, {0x0063, 0x01bc},
  {0x0064, 0x01f4}, {0x0065, 0x01bc}, {0x0066, 0x014d}, {0x0067, 0x01f4},
  {0x0068, 0x022c}, {0x0069, 0x0116}, {0x006a, 0x0116}, {0x006b, 0x01f4},
  {0x006c, 0x0116}, {0x006d, 0x030a}, {0x006e, 0x022c}, {0x006f, 0x01f4},
  {0x0070, 0x01f4}, {0x0071, 0x01f4}, {0x0072, 0x0185}, {0x0073, 0x0185},
  {0x0074, 0x0116}, {0x0075, 0x022c}, {0x0076, 0x01bc}, {0x0077, 0x029b},
  {0x0078, 0x01f4}, {0x0079, 0x01bc}, {0x007a, 0x0185}, {0x007b, 0x015c},
  {0x007c, 0x00dc}, {0x007d, 0x015c}, {0x007e, 0x023a}, {0x00a1, 0x0185},
  {0x00a2, 0x01f4}, {0x00a3, 0x01f4}, {0x00a4, 0x00a7}, {0x00a5, 0x01f4},
  {0x00a6, 0x01f4}, {0x00a7, 0x01f4}, {0x00a8, 0x01f4}, {0x00a9, 0x0116},
  {0x00aa, 0x01f4}, {0x00ab, 0x01f4}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x022c}, {0x00af, 0x022c}, {0x00b1, 0x01f4}, {0x00b2, 0x01f4},
  {0x00b3, 0x01f4}, {0x00b4, 0x00fa}, {0x00b6, 0x01f4}, {0x00b7, 0x015e},
  {0x00b8, 0x014d}, {0x00b9, 0x01f4}, {0x00ba, 0x01f4}, {0x00bb, 0x01f4},
  {0x00bc, 0x03e8}, {0x00bd, 0x03e8}, {0x00bf, 0x01f4}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x03e8}, {0x00e1, 0x03b0}, {0x00e3, 0x010a}, {0x00e8, 0x0263},
  {0x00e9, 0x02d2}, {0x00ea, 0x03b0}, {0x00eb, 0x012c}, {0x00f1, 0x02d2},
  {0x00f5, 0x0116}, {0x00f8, 0x0116}, {0x00f9, 0x01f4}, {0x00fa, 0x02d2},
  {0x00fb, 0x01f4}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::times_italic_widths =
{
  {0x0020, 0x00fa}, {0x0021, 0x014d}, {0x0022, 0x01a4}, {0x0023, 0x01f4},
  {0x0024, 0x01f4}, {0x0025, 0x0341}, {0x0026, 0x030a}, {0x0027, 0x014d},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x01f4}, {0x002b, 0x02a3},
  {0x002c, 0x00fa}, {0x002d, 0x014d}, {0x002e, 0x00fa}, {0x002f, 0x0116},
  {0x0030, 0x01f4}, {0x0031, 0x01f4}, {0x0032, 0x01f4}, {0x0033, 0x01f4},
  {0x0034, 0x01f4}, {0x0035, 0x01f4}, {0x0036, 0x01f4}, {0x0037, 0x01f4},
  {0x0038, 0x01f4}, {0x0039, 0x01f4}, {0x003a, 0x014d}, {0x003b, 0x014d},
  {0x003c, 0x02a3}, {0x003d, 0x02a3}, {0x003e, 0x02a3}, {0x003f, 0x01f4},
  {0x0040, 0x0398}, {0x0041, 0x0263}, {0x0042, 0x0263}, {0x0043, 0x029b},
  {0x0044, 0x02d2}, {0x0045, 0x0263}, {0x0046, 0x0263}, {0x0047, 0x02d2},
  {0x0048, 0x02d2}, {0x0049, 0x014d}, {0x004a, 0x01bc}, {0x004b, 0x029b},
  {0x004c, 0x022c}, {0x004d, 0x0341}, {0x004e, 0x029b}, {0x004f, 0x02d2},
  {0x0050, 0x0263}, {0x0051, 0x02d2}, {0x0052, 0x0263}, {0x0053, 0x01f4},
  {0x0054, 0x022c}, {0x0055, 0x02d2}, {0x0056, 0x0263}, {0x0057, 0x0341},
  {0x0058, 0x0263}, {0x0059, 0x022c}, {0x005a, 0x022c}, {0x005b, 0x0185},
  {0x005c, 0x0116}, {0x005d, 0x0185}, {0x005e, 0x01a6}, {0x005f, 0x01f4},
  {0x0060, 0x014d}, {0x0061, 0x01f4}, {0x0062, 0x01f4}, {0x0063, 0x01bc},
  {0x0064, 0x01f4}, {0x0065, 0x01bc}, {0x0066, 0x0116}, {0x0067, 0x01f4},
  {0x0068, 0x01f4}, {0x0069, 0x0116}, {0x006a, 0x0116}, {0x006b, 0x01bc},
  {0x006c, 0x0116}, {0x006d, 0x02d2}, {0x006e, 0x01f4}, {0x006f, 0x01f4},
  {0x0070, 0x01f4}, {0x0071, 0x01f4}, {0x0072, 0x0185}, {0x0073, 0x0185},
  {0x0074, 0x0116}, {0x0075, 0x01f4}, {0x0076, 0x01bc}, {0x0077, 0x029b},
  {0x0078, 0x01bc}, {0x0079, 0x01bc}, {0x007a, 0x0185}, {0x007b, 0x0190},
  {0x007c, 0x0113}, {0x007d, 0x0190}, {0x007e, 0x021d}, {0x00a1, 0x0185},
  {0x00a2, 0x01f4}, {0x00a3, 0x01f4}, {0x00a4, 0x00a7}, {0x00a5, 0x01f4},
  {0x00a6, 0x01f4}, {0x00a7, 0x01f4}, {0x00a8, 0x01f4}, {0x00a9, 0x00d6},
  {0x00aa, 0x022c}, {0x00ab, 0x01f4}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x01f4}, {0x00af, 0x01f4}, {0x00b1, 0x01f4}, {0x00b2, 0x01f4},
  {0x00b3, 0x01f4}, {0x00b4, 0x00fa}, {0x00b6, 0x020b}, {0x00b7, 0x015e},
  {0x00b8, 0x014d}, {0x00b9, 0x022c}, {0x00ba, 0x022c}, {0x00bb, 0x01f4},
  {0x00bc, 0x0379}, {0x00bd, 0x03e8}, {0x00bf, 0x01f4}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x0379}, {0x00e1, 0x0379}, {0x00e3, 0x0114}, {0x00e8, 0x022c},
  {0x00e9, 0x02d2}, {0x00ea, 0x03b0}, {0x00eb, 0x0136}, {0x00f1, 0x029b},
  {0x00f5, 0x0116}, {0x00f8, 0x0116}, {0x00f9, 0x01f4}, {0x00fa, 0x029b},
  {0x00fb, 0x01f4}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::times_roman_widths =
{
  {0x0020, 0x00fa}, {0x0021, 0x014d}, {0x0022, 0x0198}, {0x0023, 0x01f4},
  {0x0024, 0x01f4}, {0x0025, 0x0341}, {0x0026, 0x030a}, {0x0027, 0x014d},
  {0x0028, 0x014d}, {0x0029, 0x014d}, {0x002a, 0x01f4}, {0x002b, 0x0234},
  {0x002c, 0x00fa}, {0x002d, 0x014d}, {0x002e, 0x00fa}, {0x002f, 0x0116},
  {0x0030, 0x01f4}, {0x0031, 0x01f4}, {0x0032, 0x01f4}, {0x0033, 0x01f4},
  {0x0034, 0x01f4}, {0x0035, 0x01f4}, {0x0036, 0x01f4}, {0x0037, 0x01f4},
  {0x0038, 0x01f4}, {0x0039, 0x01f4}, {0x003a, 0x0116}, {0x003b, 0x0116},
  {0x003c, 0x0234}, {0x003d, 0x0234}, {0x003e, 0x0234}, {0x003f, 0x01bc},
  {0x0040, 0x0399}, {0x0041, 0x02d2}, {0x0042, 0x029b}, {0x0043, 0x029b},
  {0x0044, 0x02d2}, {0x0045, 0x0263}, {0x0046, 0x022c}, {0x0047, 0x02d2},
  {0x0048, 0x02d2}, {0x0049, 0x014d}, {0x004a, 0x0185}, {0x004b, 0x02d2},
  {0x004c, 0x0263}, {0x004d, 0x0379}, {0x004e, 0x02d2}, {0x004f, 0x02d2},
  {0x0050, 0x022c}, {0x0051, 0x02d2}, {0x0052, 0x029b}, {0x0053, 0x022c},
  {0x0054, 0x0263}, {0x0055, 0x02d2}, {0x0056, 0x02d2}, {0x0057, 0x03b0},
  {0x0058, 0x02d2}, {0x0059, 0x02d2}, {0x005a, 0x0263}, {0x005b, 0x014d},
  {0x005c, 0x0116}, {0x005d, 0x014d}, {0x005e, 0x01d5}, {0x005f, 0x01f4},
  {0x0060, 0x014d}, {0x0061, 0x01bc}, {0x0062, 0x01f4}, {0x0063, 0x01bc},
  {0x0064, 0x01f4}, {0x0065, 0x01bc}, {0x0066, 0x014d}, {0x0067, 0x01f4},
  {0x0068, 0x01f4}, {0x0069, 0x0116}, {0x006a, 0x0116}, {0x006b, 0x01f4},
  {0x006c, 0x0116}, {0x006d, 0x030a}, {0x006e, 0x01f4}, {0x006f, 0x01f4},
  {0x0070, 0x01f4}, {0x0071, 0x01f4}, {0x0072, 0x014d}, {0x0073, 0x0185},
  {0x0074, 0x0116}, {0x0075, 0x01f4}, {0x0076, 0x01f4}, {0x0077, 0x02d2},
  {0x0078, 0x01f4}, {0x0079, 0x01f4}, {0x007a, 0x01bc}, {0x007b, 0x01e0},
  {0x007c, 0x00c8}, {0x007d, 0x01e0}, {0x007e, 0x021d}, {0x00a1, 0x014d},
  {0x00a2, 0x01f4}, {0x00a3, 0x01f4}, {0x00a4, 0x00a7}, {0x00a5, 0x01f4},
  {0x00a6, 0x01f4}, {0x00a7, 0x01f4}, {0x00a8, 0x01f4}, {0x00a9, 0x00b4},
  {0x00aa, 0x01bc}, {0x00ab, 0x01f4}, {0x00ac, 0x014d}, {0x00ad, 0x014d},
  {0x00ae, 0x022c}, {0x00af, 0x022c}, {0x00b1, 0x01f4}, {0x00b2, 0x01f4},
  {0x00b3, 0x01f4}, {0x00b4, 0x00fa}, {0x00b6, 0x01c5}, {0x00b7, 0x015e},
  {0x00b8, 0x014d}, {0x00b9, 0x01bc}, {0x00ba, 0x01bc}, {0x00bb, 0x01f4},
  {0x00bc, 0x03e8}, {0x00bd, 0x03e8}, {0x00bf, 0x01bc}, {0x00c1, 0x014d},
  {0x00c2, 0x014d}, {0x00c3, 0x014d}, {0x00c4, 0x014d}, {0x00c5, 0x014d},
  {0x00c6, 0x014d}, {0x00c7, 0x014d}, {0x00c8, 0x014d}, {0x00ca, 0x014d},
  {0x00cb, 0x014d}, {0x00cd, 0x014d}, {0x00ce, 0x014d}, {0x00cf, 0x014d},
  {0x00d0, 0x03e8}, {0x00e1, 0x0379}, {0x00e3, 0x0114}, {0x00e8, 0x0263},
  {0x00e9, 0x02d2}, {0x00ea, 0x0379}, {0x00eb, 0x0136}, {0x00f1, 0x029b},
  {0x00f5, 0x0116}, {0x00f8, 0x0116}, {0x00f9, 0x01f4}, {0x00fa, 0x02d2},
  {0x00fb, 0x01f4}
};

//---------------------------------------------------------------------------//

std::unordered_map<Unicode, int> glyphwidths::dingbats_widths =
{
  {0x0020, 0x0116}, {0x0021, 0x03ce}, {0x0022, 0x03c1}, {0x0023, 0x03ce},
  {0x0024, 0x03d4}, {0x0025, 0x02cf}, {0x0026, 0x0315}, {0x0027, 0x0316},
  {0x0028, 0x0317}, {0x0029, 0x02b2}, {0x002a, 0x03c0}, {0x002b, 0x03ab},
  {0x002c, 0x0225}, {0x002d, 0x0357}, {0x002e, 0x038f}, {0x002f, 0x03a5},
  {0x0030, 0x038f}, {0x0031, 0x03b1}, {0x0032, 0x03ce}, {0x0033, 0x02f3},
  {0x0034, 0x034e}, {0x0035, 0x02fa}, {0x0036, 0x02f9}, {0x0037, 0x023b},
  {0x0038, 0x02a5}, {0x0039, 0x02fb}, {0x003a, 0x02f8}, {0x003b, 0x02f7},
  {0x003c, 0x02f2}, {0x003d, 0x01ee}, {0x003e, 0x0228}, {0x003f, 0x0219},
  {0x0040, 0x0241}, {0x0041, 0x02b4}, {0x0042, 0x0312}, {0x0043, 0x0314},
  {0x0044, 0x0314}, {0x0045, 0x0316}, {0x0046, 0x0319}, {0x0047, 0x031a},
  {0x0048, 0x0330}, {0x0049, 0x0337}, {0x004a, 0x0315}, {0x004b, 0x0349},
  {0x004c, 0x0337}, {0x004d, 0x0341}, {0x004e, 0x0330}, {0x004f, 0x033f},
  {0x0050, 0x039b}, {0x0051, 0x02e8}, {0x0052, 0x02d3}, {0x0053, 0x02ed},
  {0x0054, 0x0316}, {0x0055, 0x0318}, {0x0056, 0x02b7}, {0x0057, 0x0308},
  {0x0058, 0x0300}, {0x0059, 0x0318}, {0x005a, 0x02f7}, {0x005b, 0x02c3},
  {0x005c, 0x02c4}, {0x005d, 0x02aa}, {0x005e, 0x02bd}, {0x005f, 0x033a},
  {0x0060, 0x032f}, {0x0061, 0x0315}, {0x0062, 0x0315}, {0x0063, 0x02c3},
  {0x0064, 0x02af}, {0x0065, 0x02b8}, {0x0066, 0x02b1}, {0x0067, 0x0312},
  {0x0068, 0x0313}, {0x0069, 0x02c9}, {0x006a, 0x0317}, {0x006b, 0x0311},
  {0x006c, 0x0317}, {0x006d, 0x0369}, {0x006e, 0x02f9}, {0x006f, 0x02fa},
  {0x0070, 0x02fa}, {0x0071, 0x02f7}, {0x0072, 0x02f7}, {0x0073, 0x037c},
  {0x0074, 0x037c}, {0x0075, 0x0314}, {0x0076, 0x0310}, {0x0077, 0x01b6},
  {0x0078, 0x008a}, {0x0079, 0x0115}, {0x007a, 0x019f}, {0x007b, 0x0188},
  {0x007c, 0x0188}, {0x007d, 0x029c}, {0x007e, 0x029c}, {0x0080, 0x0186},
  {0x0081, 0x0186}, {0x0082, 0x013d}, {0x0083, 0x013d}, {0x0084, 0x0114},
  {0x0085, 0x0114}, {0x0086, 0x01fd}, {0x0087, 0x01fd}, {0x0088, 0x019a},
  {0x0089, 0x019a}, {0x008a, 0x00ea}, {0x008b, 0x00ea}, {0x008c, 0x014e},
  {0x008d, 0x014e}, {0x00a1, 0x02dc}, {0x00a2, 0x0220}, {0x00a3, 0x0220},
  {0x00a4, 0x038e}, {0x00a5, 0x029b}, {0x00a6, 0x02f8}, {0x00a7, 0x02f8},
  {0x00a8, 0x0308}, {0x00a9, 0x0253}, {0x00aa, 0x02b6}, {0x00ab, 0x0272},
  {0x00ac, 0x0314}, {0x00ad, 0x0314}, {0x00ae, 0x0314}, {0x00af, 0x0314},
  {0x00b0, 0x0314}, {0x00b1, 0x0314}, {0x00b2, 0x0314}, {0x00b3, 0x0314},
  {0x00b4, 0x0314}, {0x00b5, 0x0314}, {0x00b6, 0x0314}, {0x00b7, 0x0314},
  {0x00b8, 0x0314}, {0x00b9, 0x0314}, {0x00ba, 0x0314}, {0x00bb, 0x0314},
  {0x00bc, 0x0314}, {0x00bd, 0x0314}, {0x00be, 0x0314}, {0x00bf, 0x0314},
  {0x00c0, 0x0314}, {0x00c1, 0x0314}, {0x00c2, 0x0314}, {0x00c3, 0x0314},
  {0x00c4, 0x0314}, {0x00c5, 0x0314}, {0x00c6, 0x0314}, {0x00c7, 0x0314},
  {0x00c8, 0x0314}, {0x00c9, 0x0314}, {0x00ca, 0x0314}, {0x00cb, 0x0314},
  {0x00cc, 0x0314}, {0x00cd, 0x0314}, {0x00ce, 0x0314}, {0x00cf, 0x0314},
  {0x00d0, 0x0314}, {0x00d1, 0x0314}, {0x00d2, 0x0314}, {0x00d3, 0x0314},
  {0x00d4, 0x037e}, {0x00d5, 0x0346}, {0x00d6, 0x03f8}, {0x00d7, 0x01ca},
  {0x00d8, 0x02ec}, {0x00d9, 0x039c}, {0x00da, 0x02ec}, {0x00db, 0x0396},
  {0x00dc, 0x039f}, {0x00dd, 0x03a0}, {0x00de, 0x03a0}, {0x00df, 0x0342},
  {0x00e0, 0x0369}, {0x00e1, 0x033c}, {0x00e2, 0x039c}, {0x00e3, 0x039c},
  {0x00e4, 0x0395}, {0x00e5, 0x03a2}, {0x00e6, 0x03a3}, {0x00e7, 0x01cf},
  {0x00e8, 0x0373}, {0x00e9, 0x0344}, {0x00ea, 0x0344}, {0x00eb, 0x0363},
  {0x00ec, 0x0363}, {0x00ed, 0x02b8}, {0x00ee, 0x02b8}, {0x00ef, 0x036a},
  {0x00f1, 0x036a}, {0x00f2, 0x02f8}, {0x00f3, 0x03b2}, {0x00f4, 0x0303},
  {0x00f5, 0x0361}, {0x00f6, 0x0303}, {0x00f7, 0x0378}, {0x00f8, 0x03c7},
  {0x00f9, 0x0378}, {0x00fa, 0x033f}, {0x00fb, 0x0369}, {0x00fc, 0x039f},
  {0x00fd, 0x03ca}
};
