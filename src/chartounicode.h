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

static std::map<uint16_t, uint16_t> standardEncodingToUnicode =
{
  {0x41, 0x0041}, {0xe1, 0x00C6}, {0x42, 0x0042}, {0x43, 0x0043},
  {0x48, 0x0048}, {0x49, 0x0049}, {0x4a, 0x004A}, {0x4b, 0x004B},
  {0x4c, 0x004C}, {0xe8, 0x0141}, {0x4d, 0x004D}, {0x4e, 0x004E},
  {0x4f, 0x004F}, {0xea, 0x0152}, {0xe9, 0x00D8}, {0x50, 0x0050},
  {0x51, 0x0051}, {0x52, 0x0052}, {0x53, 0x0053}, {0x54, 0x0054},
  {0x55, 0x0055}, {0x56, 0x0056}, {0x57, 0x0057}, {0x58, 0x0058},
  {0x59, 0x0059}, {0x5a, 0x005A}, {0x61, 0x0061}, {0xc2, 0x00B4},
  {0xf1, 0x00E6}, {0x26, 0x0026}, {0x5e, 0x005E}, {0x7e, 0x007E},
  {0x2a, 0x002A}, {0x40, 0x0040}, {0x62, 0x0062}, {0x5c, 0x005C},
  {0x7c, 0x007C}, {0x7b, 0x007B}, {0x7d, 0x007D}, {0x5b, 0x005B},
  {0x5d, 0x005D}, {0xc6, 0x02D8}, {0xb7, 0x2022}, {0x63, 0x0063},
  {0xcf, 0x02C7}, {0xcb, 0x00B8}, {0xa2, 0x00A2}, {0xc3, 0x02C6},
  {0x3a, 0x003A}, {0x2c, 0x002C}, {0xa8, 0x00A4}, {0x64, 0x0064},
  {0xb2, 0x2020}, {0xb3, 0x2021}, {0xc8, 0x00A8}, {0x24, 0x0024},
  {0xc7, 0x02D9}, {0xf5, 0x0131}, {0x65, 0x0065}, {0x38, 0x0038},
  {0xbc, 0x2026}, {0xd0, 0x2014}, {0xb1, 0x2013}, {0x3d, 0x003D},
  {0x21, 0x0021}, {0xa1, 0x00A1}, {0x66, 0x0066}, {0xae, 0xFB01},
  {0x35, 0x0035}, {0xaf, 0xFB02}, {0xa6, 0x0192}, {0x34, 0x0034},
  {0xa4, 0x2044}, {0x67, 0x0067}, {0xfb, 0x00DF}, {0xc1, 0x0060},
  {0x3e, 0x003E}, {0xab, 0x00AB}, {0xbb, 0x00BB}, {0xac, 0x2039},
  {0xad, 0x203A}, {0x68, 0x0068}, {0xcd, 0x02DD}, {0x2d, 0x002D},
  {0x69, 0x0069}, {0x6a, 0x006A}, {0x6b, 0x006B}, {0x6c, 0x006C},
  {0x3c, 0x003C}, {0xf8, 0x0142}, {0x6d, 0x006D}, {0xc5, 0x00AF},
  {0x6e, 0x006E}, {0x39, 0x0039}, {0x23, 0x0023}, {0x6f, 0x006F},
  {0xfa, 0x0153}, {0xce, 0x02DB}, {0x31, 0x0031}, {0xe3, 0x00AA},
  {0xeb, 0x00BA}, {0xf9, 0x00F8}, {0x70, 0x0070}, {0xb6, 0x00B6},
  {0x28, 0x0028}, {0x29, 0x0029}, {0x25, 0x0025}, {0x2e, 0x002E},
  {0xb4, 0x00B7}, {0xbd, 0x2030}, {0x2b, 0x002B}, {0x71, 0x0071},
  {0x3f, 0x003F}, {0xbf, 0x00BF}, {0x22, 0x0022}, {0xb9, 0x201E},
  {0xaa, 0x201C}, {0xba, 0x201D}, {0x60, 0x2018}, {0x27, 0x2019},
  {0xb8, 0x201A}, {0xa9, 0x0027}, {0x72, 0x0072}, {0xca, 0x02DA},
  {0x73, 0x0073}, {0xa7, 0x00A7}, {0x3b, 0x003B}, {0x37, 0x0037},
  {0x36, 0x0036}, {0x2f, 0x002F}, {0x20, 0x0020}, {0xa3, 0x00A3},
  {0x74, 0x0074}, {0x33, 0x0033}, {0xc4, 0x02DC}, {0x32, 0x0032},
  {0x75, 0x0075}, {0x5f, 0x005F}, {0x76, 0x0076}, {0x77, 0x0077},
  {0x78, 0x0078}, {0x79, 0x0079}, {0xa5, 0x00A5}, {0x7a, 0x007A},
  {0x30, 0x0030}
};

static std::map<uint16_t, uint16_t> macRomanEncodingToUnicode =
{
  {0x80, 0x00C4}, {0xcb, 0x00C0}, {0x81, 0x00C5}, {0xcc, 0x00C3},
  {0x42, 0x0042}, {0x43, 0x0043}, {0x82, 0x00C7}, {0x44, 0x0044},
  {0x45, 0x0045}, {0x83, 0x00C9}, {0xe6, 0x00CA}, {0xe8, 0x00CB},
  {0xe9, 0x00C8}, {0x46, 0x0046}, {0x47, 0x0047}, {0x48, 0x0048},
  {0x49, 0x0049}, {0xea, 0x00CD}, {0xeb, 0x00CE}, {0xec, 0x00CF},
  {0xed, 0x00CC}, {0x4a, 0x004A}, {0x4b, 0x004B}, {0x4c, 0x004C},
  {0x4d, 0x004D}, {0x4e, 0x004E}, {0x84, 0x00D1}, {0x4f, 0x004F},
  {0xce, 0x0152}, {0xee, 0x00D3}, {0xef, 0x00D4}, {0x85, 0x00D6},
  {0xf1, 0x00D2}, {0xaf, 0x00D8}, {0xcd, 0x00D5}, {0x50, 0x0050},
  {0x51, 0x0051}, {0x52, 0x0052}, {0x53, 0x0053}, {0x54, 0x0054},
  {0x55, 0x0055}, {0xf2, 0x00DA}, {0xf3, 0x00DB}, {0x86, 0x00DC},
  {0xf4, 0x00D9}, {0x56, 0x0056}, {0x57, 0x0057}, {0x58, 0x0058},
  {0x59, 0x0059}, {0xd9, 0x0178}, {0x5a, 0x005A}, {0x61, 0x0061},
  {0x87, 0x00E1}, {0x89, 0x00E2}, {0xab, 0x00B4}, {0x8a, 0x00E4},
  {0xbe, 0x00E6}, {0x88, 0x00E0}, {0x26, 0x0026}, {0x8c, 0x00E5},
  {0x5e, 0x005E}, {0x7e, 0x007E}, {0x2a, 0x002A}, {0x40, 0x0040},
  {0x8b, 0x00E3}, {0x62, 0x0062}, {0x5c, 0x005C}, {0x7c, 0x007C},
  {0x7b, 0x007B}, {0x7d, 0x007D}, {0x5b, 0x005B}, {0x5d, 0x005D},
  {0xf9, 0x02D8}, {0xa5, 0x2022}, {0x63, 0x0063}, {0xff, 0x02C7},
  {0x8d, 0x00E7}, {0xfc, 0x00B8}, {0xa2, 0x00A2}, {0xf6, 0x02C6},
  {0x3a, 0x003A}, {0x2c, 0x002C}, {0xa9, 0x00A9}, {0xdb, 0x00A4},
  {0x64, 0x0064}, {0xa0, 0x2020}, {0xe0, 0x2021}, {0xa1, 0x00B0},
  {0xac, 0x00A8}, {0xd6, 0x00F7}, {0x24, 0x0024}, {0xfa, 0x02D9},
  {0xf5, 0x0131}, {0x65, 0x0065}, {0x8e, 0x00E9}, {0x90, 0x00EA},
  {0x91, 0x00EB}, {0x8f, 0x00E8}, {0x38, 0x0038}, {0xc9, 0x2026},
  {0xd1, 0x2014}, {0xd0, 0x2013}, {0x3d, 0x003D}, {0x21, 0x0021},
  {0xc1, 0x00A1}, {0x66, 0x0066}, {0xde, 0xFB01}, {0x35, 0x0035},
  {0xdf, 0xFB02}, {0xc4, 0x0192}, {0x34, 0x0034}, {0xda, 0x2044},
  {0x67, 0x0067}, {0xa7, 0x00DF}, {0x60, 0x0060}, {0x3e, 0x003E},
  {0xc7, 0x00AB}, {0xc8, 0x00BB}, {0xdc, 0x2039}, {0xdd, 0x203A},
  {0x68, 0x0068}, {0xfd, 0x02DD}, {0x2d, 0x002D}, {0x69, 0x0069},
  {0x92, 0x00ED}, {0x94, 0x00EE}, {0x95, 0x00EF}, {0x93, 0x00EC},
  {0x6a, 0x006A}, {0x6b, 0x006B}, {0x6c, 0x006C}, {0x3c, 0x003C},
  {0xc2, 0x00AC}, {0x6d, 0x006D}, {0xf8, 0x00AF}, {0xb5, 0x00B5},
  {0x6e, 0x006E}, {0x39, 0x0039}, {0x96, 0x00F1}, {0x23, 0x0023},
  {0x6f, 0x006F}, {0x97, 0x00F3}, {0x99, 0x00F4}, {0x9a, 0x00F6},
  {0xcf, 0x0153}, {0xfe, 0x02DB}, {0x98, 0x00F2}, {0x31, 0x0031},
  {0xbb, 0x00AA}, {0xbc, 0x00BA}, {0xbf, 0x00F8}, {0x9b, 0x00F5},
  {0x70, 0x0070}, {0xa6, 0x00B6}, {0x28, 0x0028}, {0x29, 0x0029},
  {0x25, 0x0025}, {0x2e, 0x002E}, {0xe1, 0x00B7}, {0xe4, 0x2030},
  {0x2b, 0x002B}, {0xb1, 0x00B1}, {0x71, 0x0071}, {0x3f, 0x003F},
  {0xc0, 0x00BF}, {0x22, 0x0022}, {0xe3, 0x201E}, {0xd2, 0x201C},
  {0xd3, 0x201D}, {0xd4, 0x2018}, {0xd5, 0x2019}, {0xe2, 0x201A},
  {0x27, 0x0027}, {0x72, 0x0072}, {0xa8, 0x00AE}, {0xfb, 0x02DA},
  {0x73, 0x0073}, {0xa4, 0x00A7}, {0x3b, 0x003B}, {0x37, 0x0037},
  {0x36, 0x0036}, {0x2f, 0x002F}, {0x20, 0x0020}, {0xa3, 0x00A3},
  {0x74, 0x0074}, {0x33, 0x0033}, {0xf7, 0x02DC}, {0xaa, 0x2122},
  {0x32, 0x0032}, {0x75, 0x0075}, {0x9c, 0x00FA}, {0x9e, 0x00FB},
  {0x9f, 0x00FC}, {0x9d, 0x00F9}, {0x5f, 0x005F}, {0x76, 0x0076},
  {0x77, 0x0077}, {0x78, 0x0078}, {0x79, 0x0079}, {0xd8, 0x00FF},
  {0xb4, 0x00A5}, {0x7a, 0x007A}, {0x30, 0x0030}, {0x41, 0x0041}
};

static std::map<uint16_t, uint16_t> winAnsiEncodingToUnicode =
{
  {0xc4, 0x00C4}, {0xc0, 0x00C0}, {0xc5, 0x00C5}, {0xc3, 0x00C3},
  {0x42, 0x0042}, {0x43, 0x0043}, {0xc7, 0x00C7}, {0x44, 0x0044},
  {0x45, 0x0045}, {0xc9, 0x00C9}, {0xca, 0x00CA}, {0xcb, 0x00CB},
  {0xc8, 0x00C8}, {0xd0, 0x00D0}, {0x80, 0x20AC}, {0x46, 0x0046},
  {0x47, 0x0047}, {0x48, 0x0048}, {0x49, 0x0049}, {0xcd, 0x00CD},
  {0xce, 0x00CE}, {0xcf, 0x00CF}, {0xcc, 0x00CC}, {0x4a, 0x004A},
  {0x4b, 0x004B}, {0x4c, 0x004C}, {0x4d, 0x004D}, {0x4e, 0x004E},
  {0xd1, 0x00D1}, {0x4f, 0x004F}, {0x8c, 0x0152}, {0xd3, 0x00D3},
  {0xd4, 0x00D4}, {0xd6, 0x00D6}, {0xd2, 0x00D2}, {0xd8, 0x00D8},
  {0xd5, 0x00D5}, {0x50, 0x0050}, {0x51, 0x0051}, {0x52, 0x0052},
  {0x53, 0x0053}, {0x8a, 0x0160}, {0x54, 0x0054}, {0xde, 0x00DE},
  {0x55, 0x0055}, {0xda, 0x00DA}, {0xdb, 0x00DB}, {0xdc, 0x00DC},
  {0xd9, 0x00D9}, {0x56, 0x0056}, {0x57, 0x0057}, {0x58, 0x0058},
  {0x59, 0x0059}, {0xdd, 0x00DD}, {0x9f, 0x0178}, {0x5a, 0x005A},
  {0x8e, 0x017D}, {0x61, 0x0061}, {0xe1, 0x00E1}, {0xe2, 0x00E2},
  {0xb4, 0x00B4}, {0xe4, 0x00E4}, {0xe6, 0x00E6}, {0xe0, 0x00E0},
  {0x26, 0x0026}, {0xe5, 0x00E5}, {0x5e, 0x005E}, {0x7e, 0x007E},
  {0x2a, 0x002A}, {0x40, 0x0040}, {0xe3, 0x00E3}, {0x62, 0x0062},
  {0x5c, 0x005C}, {0x7c, 0x007C}, {0x7b, 0x007B}, {0x7d, 0x007D},
  {0x5b, 0x005B}, {0x5d, 0x005D}, {0xa6, 0x00A6}, {0x95, 0x2022},
  {0x63, 0x0063}, {0xe7, 0x00E7}, {0xb8, 0x00B8}, {0xa2, 0x00A2},
  {0x88, 0x02C6}, {0x3a, 0x003A}, {0x2c, 0x002C}, {0xa9, 0x00A9},
  {0xa4, 0x00A4}, {0x64, 0x0064}, {0x86, 0x2020}, {0x87, 0x2021},
  {0xb0, 0x00B0}, {0xa8, 0x00A8}, {0xf7, 0x00F7}, {0x24, 0x0024},
  {0x65, 0x0065}, {0xe9, 0x00E9}, {0xea, 0x00EA}, {0xeb, 0x00EB},
  {0xe8, 0x00E8}, {0x38, 0x0038}, {0x85, 0x2026}, {0x97, 0x2014},
  {0x96, 0x2013}, {0x3d, 0x003D}, {0xf0, 0x00F0}, {0x21, 0x0021},
  {0xa1, 0x00A1}, {0x66, 0x0066}, {0x35, 0x0035}, {0x83, 0x0192},
  {0x34, 0x0034}, {0x67, 0x0067}, {0xdf, 0x00DF}, {0x60, 0x0060},
  {0x3e, 0x003E}, {0xab, 0x00AB}, {0xbb, 0x00BB}, {0x8b, 0x2039},
  {0x9b, 0x203A}, {0x68, 0x0068}, {0x2d, 0x002D}, {0x69, 0x0069},
  {0xed, 0x00ED}, {0xee, 0x00EE}, {0xef, 0x00EF}, {0xec, 0x00EC},
  {0x6a, 0x006A}, {0x6b, 0x006B}, {0x6c, 0x006C}, {0x3c, 0x003C},
  {0xac, 0x00AC}, {0x6d, 0x006D}, {0xaf, 0x00AF}, {0xb5, 0x00B5},
  {0xd7, 0x00D7}, {0x6e, 0x006E}, {0x39, 0x0039}, {0xf1, 0x00F1},
  {0x23, 0x0023}, {0x6f, 0x006F}, {0xf3, 0x00F3}, {0xf4, 0x00F4},
  {0xf6, 0x00F6}, {0x9c, 0x0153}, {0xf2, 0x00F2}, {0x31, 0x0031},
  {0xbd, 0x00BD}, {0xbc, 0x00BC}, {0xb9, 0x00B9}, {0xaa, 0x00AA},
  {0xba, 0x00BA}, {0xf8, 0x00F8}, {0xf5, 0x00F5}, {0x70, 0x0070},
  {0xb6, 0x00B6}, {0x28, 0x0028}, {0x29, 0x0029}, {0x25, 0x0025},
  {0x2e, 0x002E}, {0xb7, 0x00B7}, {0x89, 0x2030}, {0x2b, 0x002B},
  {0xb1, 0x00B1}, {0x71, 0x0071}, {0x3f, 0x003F}, {0xbf, 0x00BF},
  {0x22, 0x0022}, {0x84, 0x201E}, {0x93, 0x201C}, {0x94, 0x201D},
  {0x91, 0x2018}, {0x92, 0x2019}, {0x82, 0x201A}, {0x27, 0x0027},
  {0x72, 0x0072}, {0xae, 0x00AE}, {0x73, 0x0073}, {0x9a, 0x0161},
  {0xa7, 0x00A7}, {0x3b, 0x003B}, {0x37, 0x0037}, {0x36, 0x0036},
  {0x2f, 0x002F}, {0x20, 0x0020}, {0xa3, 0x00A3}, {0x74, 0x0074},
  {0xfe, 0x00FE}, {0x33, 0x0033}, {0xbe, 0x00BE}, {0xb3, 0x00B3},
  {0x98, 0x02DC}, {0x99, 0x2122}, {0x32, 0x0032}, {0xb2, 0x00B2},
  {0x75, 0x0075}, {0xfa, 0x00FA}, {0xfb, 0x00FB}, {0xfc, 0x00FC},
  {0xf9, 0x00F9}, {0x5f, 0x005F}, {0x76, 0x0076}, {0x77, 0x0077},
  {0x78, 0x0078}, {0x79, 0x0079}, {0xfd, 0x00FD}, {0xff, 0x00FF},
  {0xa5, 0x00A5}, {0x7a, 0x007A}, {0x9e, 0x017E}, {0x30, 0x0030},
  {0x41, 0x0041}
};

static std::map<uint16_t, uint16_t> pdfDocEncodingToUnicode =
{
  {0xc4, 0x00C4}, {0xc0, 0x00C0}, {0xc5, 0x00C5}, {0xc3, 0x00C3},
  {0x42, 0x0042}, {0x43, 0x0043}, {0xc7, 0x00C7}, {0x44, 0x0044},
  {0x45, 0x0045}, {0xc9, 0x00C9}, {0xca, 0x00CA}, {0xcb, 0x00CB},
  {0xc8, 0x00C8}, {0xd0, 0x00D0}, {0xa0, 0x20AC}, {0x46, 0x0046},
  {0x47, 0x0047}, {0x48, 0x0048}, {0x49, 0x0049}, {0xcd, 0x00CD},
  {0xce, 0x00CE}, {0xcf, 0x00CF}, {0xcc, 0x00CC}, {0x4a, 0x004A},
  {0x4b, 0x004B}, {0x4c, 0x004C}, {0x95, 0x0141}, {0x4d, 0x004D},
  {0x4e, 0x004E}, {0xd1, 0x00D1}, {0x4f, 0x004F}, {0x96, 0x0152},
  {0xd3, 0x00D3}, {0xd4, 0x00D4}, {0xd6, 0x00D6}, {0xd2, 0x00D2},
  {0xd8, 0x00D8}, {0xd5, 0x00D5}, {0x50, 0x0050}, {0x51, 0x0051},
  {0x52, 0x0052}, {0x53, 0x0053}, {0x97, 0x0160}, {0x54, 0x0054},
  {0xde, 0x00DE}, {0x55, 0x0055}, {0xda, 0x00DA}, {0xdb, 0x00DB},
  {0xdc, 0x00DC}, {0xd9, 0x00D9}, {0x56, 0x0056}, {0x57, 0x0057},
  {0x58, 0x0058}, {0x59, 0x0059}, {0xdd, 0x00DD}, {0x98, 0x0178},
  {0x5a, 0x005A}, {0x99, 0x017D}, {0x61, 0x0061}, {0xe1, 0x00E1},
  {0xe2, 0x00E2}, {0xb4, 0x00B4}, {0xe4, 0x00E4}, {0xe6, 0x00E6},
  {0xe0, 0x00E0}, {0x26, 0x0026}, {0xe5, 0x00E5}, {0x5e, 0x005E},
  {0x7e, 0x007E}, {0x2a, 0x002A}, {0x40, 0x0040}, {0xe3, 0x00E3},
  {0x62, 0x0062}, {0x5c, 0x005C}, {0x7c, 0x007C}, {0x7b, 0x007B},
  {0x7d, 0x007D}, {0x5b, 0x005B}, {0x5d, 0x005D}, {0x18, 0x02D8},
  {0xa6, 0x00A6}, {0x80, 0x2022}, {0x63, 0x0063}, {0x19, 0x02C7},
  {0xe7, 0x00E7}, {0xb8, 0x00B8}, {0xa2, 0x00A2}, {0x1a, 0x02C6},
  {0x3a, 0x003A}, {0x2c, 0x002C}, {0xa9, 0x00A9}, {0xa4, 0x00A4},
  {0x64, 0x0064}, {0x81, 0x2020}, {0x82, 0x2021}, {0xb0, 0x00B0},
  {0xa8, 0x00A8}, {0xf7, 0x00F7}, {0x24, 0x0024}, {0x1b, 0x02D9},
  {0x9a, 0x0131}, {0x65, 0x0065}, {0xe9, 0x00E9}, {0xea, 0x00EA},
  {0xeb, 0x00EB}, {0xe8, 0x00E8}, {0x38, 0x0038}, {0x83, 0x2026},
  {0x84, 0x2014}, {0x85, 0x2013}, {0x3d, 0x003D}, {0xf0, 0x00F0},
  {0x21, 0x0021}, {0xa1, 0x00A1}, {0x66, 0x0066}, {0x93, 0xFB01},
  {0x35, 0x0035}, {0x94, 0xFB02}, {0x86, 0x0192}, {0x34, 0x0034},
  {0x87, 0x2044}, {0x67, 0x0067}, {0xdf, 0x00DF}, {0x60, 0x0060},
  {0x3e, 0x003E}, {0xab, 0x00AB}, {0xbb, 0x00BB}, {0x88, 0x2039},
  {0x89, 0x203A}, {0x68, 0x0068}, {0x1c, 0x02DD}, {0x2d, 0x002D},
  {0x69, 0x0069}, {0xed, 0x00ED}, {0xee, 0x00EE}, {0xef, 0x00EF},
  {0xec, 0x00EC}, {0x6a, 0x006A}, {0x6b, 0x006B}, {0x6c, 0x006C},
  {0x3c, 0x003C}, {0xac, 0x00AC}, {0x9b, 0x0142}, {0x6d, 0x006D},
  {0xaf, 0x00AF}, {0x8a, 0x2212}, {0xb5, 0x00B5}, {0xd7, 0x00D7},
  {0x6e, 0x006E}, {0x39, 0x0039}, {0xf1, 0x00F1}, {0x23, 0x0023},
  {0x6f, 0x006F}, {0xf3, 0x00F3}, {0xf4, 0x00F4}, {0xf6, 0x00F6},
  {0x9c, 0x0153}, {0x1d, 0x02DB}, {0xf2, 0x00F2}, {0x31, 0x0031},
  {0xbd, 0x00BD}, {0xbc, 0x00BC}, {0xb9, 0x00B9}, {0xaa, 0x00AA},
  {0xba, 0x00BA}, {0xf8, 0x00F8}, {0xf5, 0x00F5}, {0x70, 0x0070},
  {0xb6, 0x00B6}, {0x28, 0x0028}, {0x29, 0x0029}, {0x25, 0x0025},
  {0x2e, 0x002E}, {0xb7, 0x00B7}, {0x8b, 0x2030}, {0x2b, 0x002B},
  {0xb1, 0x00B1}, {0x71, 0x0071}, {0x3f, 0x003F}, {0xbf, 0x00BF},
  {0x22, 0x0022}, {0x8c, 0x201E}, {0x8d, 0x201C}, {0x8e, 0x201D},
  {0x8f, 0x2018}, {0x90, 0x2019}, {0x91, 0x201A}, {0x27, 0x0027},
  {0x72, 0x0072}, {0xae, 0x00AE}, {0x1e, 0x02DA}, {0x73, 0x0073},
  {0x9d, 0x0161}, {0xa7, 0x00A7}, {0x3b, 0x003B}, {0x37, 0x0037},
  {0x36, 0x0036}, {0x2f, 0x002F}, {0x20, 0x0020}, {0xa3, 0x00A3},
  {0x74, 0x0074}, {0xfe, 0x00FE}, {0x33, 0x0033}, {0xbe, 0x00BE},
  {0xb3, 0x00B3}, {0x1f, 0x02DC}, {0x92, 0x2122}, {0x32, 0x0032},
  {0xb2, 0x00B2}, {0x75, 0x0075}, {0xfa, 0x00FA}, {0xfb, 0x00FB},
  {0xfc, 0x00FC}, {0xf9, 0x00F9}, {0x5f, 0x005F}, {0x76, 0x0076},
  {0x77, 0x0077}, {0x78, 0x0078}, {0x79, 0x0079}, {0xfd, 0x00FD},
  {0xff, 0x00FF}, {0xa5, 0x00A5}, {0x7a, 0x007A}, {0x9e, 0x017E},
  {0x30, 0x0030}, {0x41, 0x0041}
};

#endif