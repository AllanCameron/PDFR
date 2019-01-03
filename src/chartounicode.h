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
#define PDFR_CHARTOUNICODE

#include<string>
#include<map>
#include "pdfr.h"

static std::map<RawChar, Unicode> standardEncodingToUnicode =
{
  {0x0041, 0x0041}, {0x00e1, 0x00C6}, {0x0042, 0x0042}, {0x0043, 0x0043},
  {0x0048, 0x0048}, {0x0049, 0x0049}, {0x004a, 0x004A}, {0x004b, 0x004B},
  {0x004c, 0x004C}, {0x00e8, 0x0141}, {0x004d, 0x004D}, {0x004e, 0x004E},
  {0x004f, 0x004F}, {0x00ea, 0x0152}, {0x00e9, 0x00D8}, {0x0050, 0x0050},
  {0x0051, 0x0051}, {0x0052, 0x0052}, {0x0053, 0x0053}, {0x0054, 0x0054},
  {0x0055, 0x0055}, {0x0056, 0x0056}, {0x0057, 0x0057}, {0x0058, 0x0058},
  {0x0059, 0x0059}, {0x005a, 0x005A}, {0x0061, 0x0061}, {0x00c2, 0x00B4},
  {0x00f1, 0x00E6}, {0x0026, 0x0026}, {0x005e, 0x005E}, {0x007e, 0x007E},
  {0x002a, 0x002A}, {0x0040, 0x0040}, {0x0062, 0x0062}, {0x005c, 0x005C},
  {0x007c, 0x007C}, {0x007b, 0x007B}, {0x007d, 0x007D}, {0x005b, 0x005B},
  {0x005d, 0x005D}, {0x00c6, 0x02D8}, {0x00b7, 0x2022}, {0x0063, 0x0063},
  {0x00cf, 0x02C7}, {0x00cb, 0x00B8}, {0x00a2, 0x00A2}, {0x00c3, 0x02C6},
  {0x003a, 0x003A}, {0x002c, 0x002C}, {0x00a8, 0x00A4}, {0x0064, 0x0064},
  {0x00b2, 0x2020}, {0x00b3, 0x2021}, {0x00c8, 0x00A8}, {0x0024, 0x0024},
  {0x00c7, 0x02D9}, {0x00f5, 0x0131}, {0x0065, 0x0065}, {0x0038, 0x0038},
  {0x00bc, 0x2026}, {0x00d0, 0x2014}, {0x00b1, 0x2013}, {0x003d, 0x003D},
  {0x0021, 0x0021}, {0x00a1, 0x00A1}, {0x0066, 0x0066}, {0x00ae, 0xFB01},
  {0x0035, 0x0035}, {0x00af, 0xFB02}, {0x00a6, 0x0192}, {0x0034, 0x0034},
  {0x00a4, 0x2044}, {0x0067, 0x0067}, {0x00fb, 0x00DF}, {0x00c1, 0x0060},
  {0x003e, 0x003E}, {0x00ab, 0x00AB}, {0x00bb, 0x00BB}, {0x00ac, 0x2039},
  {0x00ad, 0x203A}, {0x0068, 0x0068}, {0x00cd, 0x02DD}, {0x002d, 0x002D},
  {0x0069, 0x0069}, {0x006a, 0x006A}, {0x006b, 0x006B}, {0x006c, 0x006C},
  {0x003c, 0x003C}, {0x00f8, 0x0142}, {0x006d, 0x006D}, {0x00c5, 0x00AF},
  {0x006e, 0x006E}, {0x0039, 0x0039}, {0x0023, 0x0023}, {0x006f, 0x006F},
  {0x00fa, 0x0153}, {0x00ce, 0x02DB}, {0x0031, 0x0031}, {0x00e3, 0x00AA},
  {0x00eb, 0x00BA}, {0x00f9, 0x00F8}, {0x0070, 0x0070}, {0x00b6, 0x00B6},
  {0x0028, 0x0028}, {0x0029, 0x0029}, {0x0025, 0x0025}, {0x002e, 0x002E},
  {0x00b4, 0x00B7}, {0x00bd, 0x2030}, {0x002b, 0x002B}, {0x0071, 0x0071},
  {0x003f, 0x003F}, {0x00bf, 0x00BF}, {0x0022, 0x0022}, {0x00b9, 0x201E},
  {0x00aa, 0x201C}, {0x00ba, 0x201D}, {0x0060, 0x2018}, {0x0027, 0x2019},
  {0x00b8, 0x201A}, {0x00a9, 0x0027}, {0x0072, 0x0072}, {0x00ca, 0x02DA},
  {0x0073, 0x0073}, {0x00a7, 0x00A7}, {0x003b, 0x003B}, {0x0037, 0x0037},
  {0x0036, 0x0036}, {0x002f, 0x002F}, {0x0020, 0x0020}, {0x00a3, 0x00A3},
  {0x0074, 0x0074}, {0x0033, 0x0033}, {0x00c4, 0x02DC}, {0x0032, 0x0032},
  {0x0075, 0x0075}, {0x005f, 0x005F}, {0x0076, 0x0076}, {0x0077, 0x0077},
  {0x0078, 0x0078}, {0x0079, 0x0079}, {0x00a5, 0x00A5}, {0x007a, 0x007A},
  {0x0030, 0x0030}
};

static std::map<RawChar, Unicode> macRomanEncodingToUnicode =
{
  {0x0080, 0x00C4}, {0x00cb, 0x00C0}, {0x0081, 0x00C5}, {0x00cc, 0x00C3},
  {0x0042, 0x0042}, {0x0043, 0x0043}, {0x0082, 0x00C7}, {0x0044, 0x0044},
  {0x0045, 0x0045}, {0x0083, 0x00C9}, {0x00e6, 0x00CA}, {0x00e8, 0x00CB},
  {0x00e9, 0x00C8}, {0x0046, 0x0046}, {0x0047, 0x0047}, {0x0048, 0x0048},
  {0x0049, 0x0049}, {0x00ea, 0x00CD}, {0x00eb, 0x00CE}, {0x00ec, 0x00CF},
  {0x00ed, 0x00CC}, {0x004a, 0x004A}, {0x004b, 0x004B}, {0x004c, 0x004C},
  {0x004d, 0x004D}, {0x004e, 0x004E}, {0x0084, 0x00D1}, {0x004f, 0x004F},
  {0x00ce, 0x0152}, {0x00ee, 0x00D3}, {0x00ef, 0x00D4}, {0x0085, 0x00D6},
  {0x00f1, 0x00D2}, {0x00af, 0x00D8}, {0x00cd, 0x00D5}, {0x0050, 0x0050},
  {0x0051, 0x0051}, {0x0052, 0x0052}, {0x0053, 0x0053}, {0x0054, 0x0054},
  {0x0055, 0x0055}, {0x00f2, 0x00DA}, {0x00f3, 0x00DB}, {0x0086, 0x00DC},
  {0x00f4, 0x00D9}, {0x0056, 0x0056}, {0x0057, 0x0057}, {0x0058, 0x0058},
  {0x0059, 0x0059}, {0x00d9, 0x0178}, {0x005a, 0x005A}, {0x0061, 0x0061},
  {0x0087, 0x00E1}, {0x0089, 0x00E2}, {0x00ab, 0x00B4}, {0x008a, 0x00E4},
  {0x00be, 0x00E6}, {0x0088, 0x00E0}, {0x0026, 0x0026}, {0x008c, 0x00E5},
  {0x005e, 0x005E}, {0x007e, 0x007E}, {0x002a, 0x002A}, {0x0040, 0x0040},
  {0x008b, 0x00E3}, {0x0062, 0x0062}, {0x005c, 0x005C}, {0x007c, 0x007C},
  {0x007b, 0x007B}, {0x007d, 0x007D}, {0x005b, 0x005B}, {0x005d, 0x005D},
  {0x00f9, 0x02D8}, {0x00a5, 0x2022}, {0x0063, 0x0063}, {0x00ff, 0x02C7},
  {0x008d, 0x00E7}, {0x00fc, 0x00B8}, {0x00a2, 0x00A2}, {0x00f6, 0x02C6},
  {0x003a, 0x003A}, {0x002c, 0x002C}, {0x00a9, 0x00A9}, {0x00db, 0x00A4},
  {0x0064, 0x0064}, {0x00a0, 0x2020}, {0x00e0, 0x2021}, {0x00a1, 0x00B0},
  {0x00ac, 0x00A8}, {0x00d6, 0x00F7}, {0x0024, 0x0024}, {0x00fa, 0x02D9},
  {0x00f5, 0x0131}, {0x0065, 0x0065}, {0x008e, 0x00E9}, {0x0090, 0x00EA},
  {0x0091, 0x00EB}, {0x008f, 0x00E8}, {0x0038, 0x0038}, {0x00c9, 0x2026},
  {0x00d1, 0x2014}, {0x00d0, 0x2013}, {0x003d, 0x003D}, {0x0021, 0x0021},
  {0x00c1, 0x00A1}, {0x0066, 0x0066}, {0x00de, 0xFB01}, {0x0035, 0x0035},
  {0x00df, 0xFB02}, {0x00c4, 0x0192}, {0x0034, 0x0034}, {0x00da, 0x2044},
  {0x0067, 0x0067}, {0x00a7, 0x00DF}, {0x0060, 0x0060}, {0x003e, 0x003E},
  {0x00c7, 0x00AB}, {0x00c8, 0x00BB}, {0x00dc, 0x2039}, {0x00dd, 0x203A},
  {0x0068, 0x0068}, {0x00fd, 0x02DD}, {0x002d, 0x002D}, {0x0069, 0x0069},
  {0x0092, 0x00ED}, {0x0094, 0x00EE}, {0x0095, 0x00EF}, {0x0093, 0x00EC},
  {0x006a, 0x006A}, {0x006b, 0x006B}, {0x006c, 0x006C}, {0x003c, 0x003C},
  {0x00c2, 0x00AC}, {0x006d, 0x006D}, {0x00f8, 0x00AF}, {0x00b5, 0x00B5},
  {0x006e, 0x006E}, {0x0039, 0x0039}, {0x0096, 0x00F1}, {0x0023, 0x0023},
  {0x006f, 0x006F}, {0x0097, 0x00F3}, {0x0099, 0x00F4}, {0x009a, 0x00F6},
  {0x00cf, 0x0153}, {0x00fe, 0x02DB}, {0x0098, 0x00F2}, {0x0031, 0x0031},
  {0x00bb, 0x00AA}, {0x00bc, 0x00BA}, {0x00bf, 0x00F8}, {0x009b, 0x00F5},
  {0x0070, 0x0070}, {0x00a6, 0x00B6}, {0x0028, 0x0028}, {0x0029, 0x0029},
  {0x0025, 0x0025}, {0x002e, 0x002E}, {0x00e1, 0x00B7}, {0x00e4, 0x2030},
  {0x002b, 0x002B}, {0x00b1, 0x00B1}, {0x0071, 0x0071}, {0x003f, 0x003F},
  {0x00c0, 0x00BF}, {0x0022, 0x0022}, {0x00e3, 0x201E}, {0x00d2, 0x201C},
  {0x00d3, 0x201D}, {0x00d4, 0x2018}, {0x00d5, 0x2019}, {0x00e2, 0x201A},
  {0x0027, 0x0027}, {0x0072, 0x0072}, {0x00a8, 0x00AE}, {0x00fb, 0x02DA},
  {0x0073, 0x0073}, {0x00a4, 0x00A7}, {0x003b, 0x003B}, {0x0037, 0x0037},
  {0x0036, 0x0036}, {0x002f, 0x002F}, {0x0020, 0x0020}, {0x00a3, 0x00A3},
  {0x0074, 0x0074}, {0x0033, 0x0033}, {0x00f7, 0x02DC}, {0x00aa, 0x2122},
  {0x0032, 0x0032}, {0x0075, 0x0075}, {0x009c, 0x00FA}, {0x009e, 0x00FB},
  {0x009f, 0x00FC}, {0x009d, 0x00F9}, {0x005f, 0x005F}, {0x0076, 0x0076},
  {0x0077, 0x0077}, {0x0078, 0x0078}, {0x0079, 0x0079}, {0x00d8, 0x00FF},
  {0x00b4, 0x00A5}, {0x007a, 0x007A}, {0x0030, 0x0030}, {0x0041, 0x0041}
};

static std::map<RawChar, Unicode> winAnsiEncodingToUnicode =
{
  {0x00c4, 0x00C4}, {0x00c0, 0x00C0}, {0x00c5, 0x00C5}, {0x00c3, 0x00C3},
  {0x0042, 0x0042}, {0x0043, 0x0043}, {0x00c7, 0x00C7}, {0x0044, 0x0044},
  {0x0045, 0x0045}, {0x00c9, 0x00C9}, {0x00ca, 0x00CA}, {0x00cb, 0x00CB},
  {0x00c8, 0x00C8}, {0x00d0, 0x00D0}, {0x0080, 0x20AC}, {0x0046, 0x0046},
  {0x0047, 0x0047}, {0x0048, 0x0048}, {0x0049, 0x0049}, {0x00cd, 0x00CD},
  {0x00ce, 0x00CE}, {0x00cf, 0x00CF}, {0x00cc, 0x00CC}, {0x004a, 0x004A},
  {0x004b, 0x004B}, {0x004c, 0x004C}, {0x004d, 0x004D}, {0x004e, 0x004E},
  {0x00d1, 0x00D1}, {0x004f, 0x004F}, {0x008c, 0x0152}, {0x00d3, 0x00D3},
  {0x00d4, 0x00D4}, {0x00d6, 0x00D6}, {0x00d2, 0x00D2}, {0x00d8, 0x00D8},
  {0x00d5, 0x00D5}, {0x0050, 0x0050}, {0x0051, 0x0051}, {0x0052, 0x0052},
  {0x0053, 0x0053}, {0x008a, 0x0160}, {0x0054, 0x0054}, {0x00de, 0x00DE},
  {0x0055, 0x0055}, {0x00da, 0x00DA}, {0x00db, 0x00DB}, {0x00dc, 0x00DC},
  {0x00d9, 0x00D9}, {0x0056, 0x0056}, {0x0057, 0x0057}, {0x0058, 0x0058},
  {0x0059, 0x0059}, {0x00dd, 0x00DD}, {0x009f, 0x0178}, {0x005a, 0x005A},
  {0x008e, 0x017D}, {0x0061, 0x0061}, {0x00e1, 0x00E1}, {0x00e2, 0x00E2},
  {0x00b4, 0x00B4}, {0x00e4, 0x00E4}, {0x00e6, 0x00E6}, {0x00e0, 0x00E0},
  {0x0026, 0x0026}, {0x00e5, 0x00E5}, {0x005e, 0x005E}, {0x007e, 0x007E},
  {0x002a, 0x002A}, {0x0040, 0x0040}, {0x00e3, 0x00E3}, {0x0062, 0x0062},
  {0x005c, 0x005C}, {0x007c, 0x007C}, {0x007b, 0x007B}, {0x007d, 0x007D},
  {0x005b, 0x005B}, {0x005d, 0x005D}, {0x00a6, 0x00A6}, {0x0095, 0x2022},
  {0x0063, 0x0063}, {0x00e7, 0x00E7}, {0x00b8, 0x00B8}, {0x00a2, 0x00A2},
  {0x0088, 0x02C6}, {0x003a, 0x003A}, {0x002c, 0x002C}, {0x00a9, 0x00A9},
  {0x00a4, 0x00A4}, {0x0064, 0x0064}, {0x0086, 0x2020}, {0x0087, 0x2021},
  {0x00b0, 0x00B0}, {0x00a8, 0x00A8}, {0x00f7, 0x00F7}, {0x0024, 0x0024},
  {0x0065, 0x0065}, {0x00e9, 0x00E9}, {0x00ea, 0x00EA}, {0x00eb, 0x00EB},
  {0x00e8, 0x00E8}, {0x0038, 0x0038}, {0x0085, 0x2026}, {0x0097, 0x2014},
  {0x0096, 0x2013}, {0x003d, 0x003D}, {0x00f0, 0x00F0}, {0x0021, 0x0021},
  {0x00a1, 0x00A1}, {0x0066, 0x0066}, {0x0035, 0x0035}, {0x0083, 0x0192},
  {0x0034, 0x0034}, {0x0067, 0x0067}, {0x00df, 0x00DF}, {0x0060, 0x0060},
  {0x003e, 0x003E}, {0x00ab, 0x00AB}, {0x00bb, 0x00BB}, {0x008b, 0x2039},
  {0x009b, 0x203A}, {0x0068, 0x0068}, {0x002d, 0x002D}, {0x0069, 0x0069},
  {0x00ed, 0x00ED}, {0x00ee, 0x00EE}, {0x00ef, 0x00EF}, {0x00ec, 0x00EC},
  {0x006a, 0x006A}, {0x006b, 0x006B}, {0x006c, 0x006C}, {0x003c, 0x003C},
  {0x00ac, 0x00AC}, {0x006d, 0x006D}, {0x00af, 0x00AF}, {0x00b5, 0x00B5},
  {0x00d7, 0x00D7}, {0x006e, 0x006E}, {0x0039, 0x0039}, {0x00f1, 0x00F1},
  {0x0023, 0x0023}, {0x006f, 0x006F}, {0x00f3, 0x00F3}, {0x00f4, 0x00F4},
  {0x00f6, 0x00F6}, {0x009c, 0x0153}, {0x00f2, 0x00F2}, {0x0031, 0x0031},
  {0x00bd, 0x00BD}, {0x00bc, 0x00BC}, {0x00b9, 0x00B9}, {0x00aa, 0x00AA},
  {0x00ba, 0x00BA}, {0x00f8, 0x00F8}, {0x00f5, 0x00F5}, {0x0070, 0x0070},
  {0x00b6, 0x00B6}, {0x0028, 0x0028}, {0x0029, 0x0029}, {0x0025, 0x0025},
  {0x002e, 0x002E}, {0x00b7, 0x00B7}, {0x0089, 0x2030}, {0x002b, 0x002B},
  {0x00b1, 0x00B1}, {0x0071, 0x0071}, {0x003f, 0x003F}, {0x00bf, 0x00BF},
  {0x0022, 0x0022}, {0x0084, 0x201E}, {0x0093, 0x201C}, {0x0094, 0x201D},
  {0x0091, 0x2018}, {0x0092, 0x2019}, {0x0082, 0x201A}, {0x0027, 0x0027},
  {0x0072, 0x0072}, {0x00ae, 0x00AE}, {0x0073, 0x0073}, {0x009a, 0x0161},
  {0x00a7, 0x00A7}, {0x003b, 0x003B}, {0x0037, 0x0037}, {0x0036, 0x0036},
  {0x002f, 0x002F}, {0x0020, 0x0020}, {0x00a3, 0x00A3}, {0x0074, 0x0074},
  {0x00fe, 0x00FE}, {0x0033, 0x0033}, {0x00be, 0x00BE}, {0x00b3, 0x00B3},
  {0x0098, 0x02DC}, {0x0099, 0x2122}, {0x0032, 0x0032}, {0x00b2, 0x00B2},
  {0x0075, 0x0075}, {0x00fa, 0x00FA}, {0x00fb, 0x00FB}, {0x00fc, 0x00FC},
  {0x00f9, 0x00F9}, {0x005f, 0x005F}, {0x0076, 0x0076}, {0x0077, 0x0077},
  {0x0078, 0x0078}, {0x0079, 0x0079}, {0x00fd, 0x00FD}, {0x00ff, 0x00FF},
  {0x00a5, 0x00A5}, {0x007a, 0x007A}, {0x009e, 0x017E}, {0x0030, 0x0030},
  {0x0041, 0x0041}
};

static std::map<RawChar, Unicode> pdfDocEncodingToUnicode =
{
  {0x00c4, 0x00C4}, {0x00c0, 0x00C0}, {0x00c5, 0x00C5}, {0x00c3, 0x00C3},
  {0x0042, 0x0042}, {0x0043, 0x0043}, {0x00c7, 0x00C7}, {0x0044, 0x0044},
  {0x0045, 0x0045}, {0x00c9, 0x00C9}, {0x00ca, 0x00CA}, {0x00cb, 0x00CB},
  {0x00c8, 0x00C8}, {0x00d0, 0x00D0}, {0x00a0, 0x20AC}, {0x0046, 0x0046},
  {0x0047, 0x0047}, {0x0048, 0x0048}, {0x0049, 0x0049}, {0x00cd, 0x00CD},
  {0x00ce, 0x00CE}, {0x00cf, 0x00CF}, {0x00cc, 0x00CC}, {0x004a, 0x004A},
  {0x004b, 0x004B}, {0x004c, 0x004C}, {0x0095, 0x0141}, {0x004d, 0x004D},
  {0x004e, 0x004E}, {0x00d1, 0x00D1}, {0x004f, 0x004F}, {0x0096, 0x0152},
  {0x00d3, 0x00D3}, {0x00d4, 0x00D4}, {0x00d6, 0x00D6}, {0x00d2, 0x00D2},
  {0x00d8, 0x00D8}, {0x00d5, 0x00D5}, {0x0050, 0x0050}, {0x0051, 0x0051},
  {0x0052, 0x0052}, {0x0053, 0x0053}, {0x0097, 0x0160}, {0x0054, 0x0054},
  {0x00de, 0x00DE}, {0x0055, 0x0055}, {0x00da, 0x00DA}, {0x00db, 0x00DB},
  {0x00dc, 0x00DC}, {0x00d9, 0x00D9}, {0x0056, 0x0056}, {0x0057, 0x0057},
  {0x0058, 0x0058}, {0x0059, 0x0059}, {0x00dd, 0x00DD}, {0x0098, 0x0178},
  {0x005a, 0x005A}, {0x0099, 0x017D}, {0x0061, 0x0061}, {0x00e1, 0x00E1},
  {0x00e2, 0x00E2}, {0x00b4, 0x00B4}, {0x00e4, 0x00E4}, {0x00e6, 0x00E6},
  {0x00e0, 0x00E0}, {0x0026, 0x0026}, {0x00e5, 0x00E5}, {0x005e, 0x005E},
  {0x007e, 0x007E}, {0x002a, 0x002A}, {0x0040, 0x0040}, {0x00e3, 0x00E3},
  {0x0062, 0x0062}, {0x005c, 0x005C}, {0x007c, 0x007C}, {0x007b, 0x007B},
  {0x007d, 0x007D}, {0x005b, 0x005B}, {0x005d, 0x005D}, {0x0018, 0x02D8},
  {0x00a6, 0x00A6}, {0x0080, 0x2022}, {0x0063, 0x0063}, {0x0019, 0x02C7},
  {0x00e7, 0x00E7}, {0x00b8, 0x00B8}, {0x00a2, 0x00A2}, {0x001a, 0x02C6},
  {0x003a, 0x003A}, {0x002c, 0x002C}, {0x00a9, 0x00A9}, {0x00a4, 0x00A4},
  {0x0064, 0x0064}, {0x0081, 0x2020}, {0x0082, 0x2021}, {0x00b0, 0x00B0},
  {0x00a8, 0x00A8}, {0x00f7, 0x00F7}, {0x0024, 0x0024}, {0x001b, 0x02D9},
  {0x009a, 0x0131}, {0x0065, 0x0065}, {0x00e9, 0x00E9}, {0x00ea, 0x00EA},
  {0x00eb, 0x00EB}, {0x00e8, 0x00E8}, {0x0038, 0x0038}, {0x0083, 0x2026},
  {0x0084, 0x2014}, {0x0085, 0x2013}, {0x003d, 0x003D}, {0x00f0, 0x00F0},
  {0x0021, 0x0021}, {0x00a1, 0x00A1}, {0x0066, 0x0066}, {0x0093, 0xFB01},
  {0x0035, 0x0035}, {0x0094, 0xFB02}, {0x0086, 0x0192}, {0x0034, 0x0034},
  {0x0087, 0x2044}, {0x0067, 0x0067}, {0x00df, 0x00DF}, {0x0060, 0x0060},
  {0x003e, 0x003E}, {0x00ab, 0x00AB}, {0x00bb, 0x00BB}, {0x0088, 0x2039},
  {0x0089, 0x203A}, {0x0068, 0x0068}, {0x001c, 0x02DD}, {0x002d, 0x002D},
  {0x0069, 0x0069}, {0x00ed, 0x00ED}, {0x00ee, 0x00EE}, {0x00ef, 0x00EF},
  {0x00ec, 0x00EC}, {0x006a, 0x006A}, {0x006b, 0x006B}, {0x006c, 0x006C},
  {0x003c, 0x003C}, {0x00ac, 0x00AC}, {0x009b, 0x0142}, {0x006d, 0x006D},
  {0x00af, 0x00AF}, {0x008a, 0x2212}, {0x00b5, 0x00B5}, {0x00d7, 0x00D7},
  {0x006e, 0x006E}, {0x0039, 0x0039}, {0x00f1, 0x00F1}, {0x0023, 0x0023},
  {0x006f, 0x006F}, {0x00f3, 0x00F3}, {0x00f4, 0x00F4}, {0x00f6, 0x00F6},
  {0x009c, 0x0153}, {0x001d, 0x02DB}, {0x00f2, 0x00F2}, {0x0031, 0x0031},
  {0x00bd, 0x00BD}, {0x00bc, 0x00BC}, {0x00b9, 0x00B9}, {0x00aa, 0x00AA},
  {0x00ba, 0x00BA}, {0x00f8, 0x00F8}, {0x00f5, 0x00F5}, {0x0070, 0x0070},
  {0x00b6, 0x00B6}, {0x0028, 0x0028}, {0x0029, 0x0029}, {0x0025, 0x0025},
  {0x002e, 0x002E}, {0x00b7, 0x00B7}, {0x008b, 0x2030}, {0x002b, 0x002B},
  {0x00b1, 0x00B1}, {0x0071, 0x0071}, {0x003f, 0x003F}, {0x00bf, 0x00BF},
  {0x0022, 0x0022}, {0x008c, 0x201E}, {0x008d, 0x201C}, {0x008e, 0x201D},
  {0x008f, 0x2018}, {0x0090, 0x2019}, {0x0091, 0x201A}, {0x0027, 0x0027},
  {0x0072, 0x0072}, {0x00ae, 0x00AE}, {0x001e, 0x02DA}, {0x0073, 0x0073},
  {0x009d, 0x0161}, {0x00a7, 0x00A7}, {0x003b, 0x003B}, {0x0037, 0x0037},
  {0x0036, 0x0036}, {0x002f, 0x002F}, {0x0020, 0x0020}, {0x00a3, 0x00A3},
  {0x0074, 0x0074}, {0x00fe, 0x00FE}, {0x0033, 0x0033}, {0x00be, 0x00BE},
  {0x00b3, 0x00B3}, {0x001f, 0x02DC}, {0x0092, 0x2122}, {0x0032, 0x0032},
  {0x00b2, 0x00B2}, {0x0075, 0x0075}, {0x00fa, 0x00FA}, {0x00fb, 0x00FB},
  {0x00fc, 0x00FC}, {0x00f9, 0x00F9}, {0x005f, 0x005F}, {0x0076, 0x0076},
  {0x0077, 0x0077}, {0x0078, 0x0078}, {0x0079, 0x0079}, {0x00fd, 0x00FD},
  {0x00ff, 0x00FF}, {0x00a5, 0x00A5}, {0x007a, 0x007A}, {0x009e, 0x017E},
  {0x0030, 0x0030}, {0x0041, 0x0041}
};

#endif
