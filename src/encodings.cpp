//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR encodings implementation file                                       //
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
 * We therefore need to know the encoding used if we want to recover the
 * corglyphs from the string. We do this by reading the encoding entry of the
 * fonts dictionary. This allows us to convert directly to a pdf-standard
 * name ("/glyphname") for each character. This can be converted as needed
 * for output on the system running the software.
 *
 * We need to start with a base encoding, if specified in the font dictionary.
 * Sometimes, none is specified in which case we use standard encoding.
 * Sometimes, some or all glyph names and their byte values are given; these
 * supercede the base encoding). Sometimes the encoding is given as
 * /Identity-H which means the encoding is specified in a CMAP.
 *
 * Since this library aims to extract usable text rather than beautiful layout,
 * some glyphs need to be converted to pairs of lower-byte glyphs to make
 * text extraction more useful, particularly the ligatures.
 */


#include "pdfr.h"
#include "document.h"
#include "stringfunctions.h"
#include "streams.h"
#include "encodings.h"
#include "ucm.h"

EncMap getBaseEncode(const std::string& encoding)
{
  EncMap res;
  if(encoding != "/MacRomanEncoding" &&
     encoding != "/WinAnsiEncoding" &&
     encoding != "/PDFDocEncoding")
  {
    res[0x41] = "/A";
    res[0xe1] = "/AE";
    res[0x42] = "/B";
    res[0x43] = "/C";
    res[0x44] = "/D";
    res[0x45] = "/E";
    res[0x46] = "/F";
    res[0x47] = "/G";
    res[0x48] = "/H";
    res[0x49] = "/I";
    res[0x4a] = "/J";
    res[0x4b] = "/K";
    res[0x4c] = "/L";
    res[0xe8] = "/Lslash";
    res[0x4d] = "/M";
    res[0x4e] = "/N";
    res[0x4f] = "/O";
    res[0xea] = "/OE";
    res[0xe9] = "/Oslash";
    res[0x50] = "/P";
    res[0x51] = "/Q";
    res[0x52] = "/R";
    res[0x53] = "/S";
    res[0x54] = "/T";
    res[0x55] = "/U";
    res[0x56] = "/V";
    res[0x57] = "/W";
    res[0x58] = "/X";
    res[0x59] = "/Y";
    res[0x5a] = "/Z";
    res[0x61] = "/a";
    res[0xc2] = "/acute";
    res[0xf1] = "/ae";
    res[0x26] = "/ampersand";
    res[0x5e] = "/asciicircum";
    res[0x7e] = "/asciitilde";
    res[0x2a] = "/asterisk";
    res[0x40] = "/at";
    res[0x62] = "/b";
    res[0x5c] = "/backslash";
    res[0x7c] = "/bar";
    res[0x7b] = "/braceleft";
    res[0x7d] = "/braceright";
    res[0x5b] = "/bracketleft";
    res[0x5d] = "/bracketright";
    res[0xc6] = "/breve";
    res[0xb7] = "/bullet";
    res[0x63] = "/c";
    res[0xcf] = "/caron";
    res[0xcb] = "/cedilla";
    res[0xa2] = "/cent";
    res[0xc3] = "/circumflex";
    res[0x3a] = "/colon";
    res[0x2c] = "/comma";
    res[0xa8] = "/currency";
    res[0x64] = "/d";
    res[0xb2] = "/dagger";
    res[0xb3] = "/daggerdbl";
    res[0xc8] = "/dieresis";
    res[0x24] = "/dollar";
    res[0xc7] = "/dotaccent";
    res[0xf5] = "/dotlessi";
    res[0x65] = "/e";
    res[0x38] = "/eight";
    res[0xbc] = "/ellipsis";
    res[0xd0] = "/emdash";
    res[0xb1] = "/endash";
    res[0x3d] = "/equal";
    res[0x21] = "/exclam";
    res[0xa1] = "/exclamdown";
    res[0x66] = "/f";
    res[0xae] = "/fi";
    res[0x35] = "/five";
    res[0xaf] = "/fl";
    res[0xa6] = "/florin";
    res[0x34] = "/four";
    res[0xa4] = "/fraction";
    res[0x67] = "/g";
    res[0xfb] = "/germandbls";
    res[0xc1] = "/grave";
    res[0x3e] = "/greater";
    res[0xab] = "/guillemotleft";
    res[0xbb] = "/guillemotright";
    res[0xac] = "/guilsinglleft";
    res[0xad] = "/guilsinglright";
    res[0x68] = "/h";
    res[0xcd] = "/hungarumlaut";
    res[0x2d] = "/hyphen";
    res[0x69] = "/i";
    res[0x6a] = "/j";
    res[0x6b] = "/k";
    res[0x6c] = "/l";
    res[0x3c] = "/less";
    res[0xf8] = "/lslash";
    res[0x6d] = "/m";
    res[0xc5] = "/macron";
    res[0x6e] = "/n";
    res[0x39] = "/nine";
    res[0x23] = "/numbersign";
    res[0x6f] = "/o";
    res[0xfa] = "/oe";
    res[0xce] = "/ogonek";
    res[0x31] = "/one";
    res[0xe3] = "/ordfeminine";
    res[0xeb] = "/ordmasculine";
    res[0xf9] = "/oslash";
    res[0x70] = "/p";
    res[0xb6] = "/paragraph";
    res[0x28] = "/parenleft";
    res[0x29] = "/parenright";
    res[0x25] = "/percent";
    res[0x2e] = "/period";
    res[0xb4] = "/periodcentered";
    res[0xbd] = "/perthousand";
    res[0x2b] = "/plus";
    res[0x71] = "/q";
    res[0x3f] = "/question";
    res[0xbf] = "/questiondown";
    res[0x22] = "/quotedbl";
    res[0xb9] = "/quotedblbase";
    res[0xaa] = "/quotedblleft";
    res[0xba] = "/quotedblright";
    res[0x60] = "/quoteleft";
    res[0x27] = "/quoteright";
    res[0xb8] = "/quotesinglbase";
    res[0xa9] = "/quotesingle";
    res[0x72] = "/r";
    res[0xca] = "/ring";
    res[0x73] = "/s";
    res[0xa7] = "/section";
    res[0x3b] = "/semicolon";
    res[0x37] = "/seven";
    res[0x36] = "/six";
    res[0x2f] = "/slash";
    res[0x20] = "/space";
    res[0xa3] = "/sterling";
    res[0x74] = "/t";
    res[0x33] = "/three";
    res[0xc4] = "/tilde";
    res[0x32] = "/two";
    res[0x75] = "/u";
    res[0x5f] = "/underscore";
    res[0x76] = "/v";
    res[0x77] = "/w";
    res[0x78] = "/x";
    res[0x79] = "/y";
    res[0xa5] = "/yen";
    res[0x7a] = "/z";
    res[0x30] = "/zero";
  }

  if(encoding == "/MacRomanEncoding")
  {
    res[0x41] = "/A";
    res[0xae] = "/AE";
    res[0xe7] = "/Aacute";
    res[0xe5] = "/Acircumflex";
    res[0x80] = "/Adieresis";
    res[0xcb] = "/Agrave";
    res[0x81] = "/Aring";
    res[0xcc] = "/Atilde";
    res[0x42] = "/B";
    res[0x43] = "/C";
    res[0x82] = "/Ccedilla";
    res[0x44] = "/D";
    res[0x45] = "/E";
    res[0x83] = "/Eacute";
    res[0xe6] = "/Ecircumflex";
    res[0xe8] = "/Edieresis";
    res[0xe9] = "/Egrave";
    res[0x46] = "/F";
    res[0x47] = "/G";
    res[0x48] = "/H";
    res[0x49] = "/I";
    res[0xea] = "/Iacute";
    res[0xeb] = "/Icircumflex";
    res[0xec] = "/Idieresis";
    res[0xed] = "/Igrave";
    res[0x4a] = "/J";
    res[0x4b] = "/K";
    res[0x4c] = "/L";
    res[0x4d] = "/M";
    res[0x4e] = "/N";
    res[0x84] = "/Ntilde";
    res[0x4f] = "/O";
    res[0xce] = "/OE";
    res[0xee] = "/Oacute";
    res[0xef] = "/Ocircumflex";
    res[0x85] = "/Odieresis";
    res[0xf1] = "/Ograve";
    res[0xaf] = "/Oslash";
    res[0xcd] = "/Otilde";
    res[0x50] = "/P";
    res[0x51] = "/Q";
    res[0x52] = "/R";
    res[0x53] = "/S";
    res[0x54] = "/T";
    res[0x55] = "/U";
    res[0xf2] = "/Uacute";
    res[0xf3] = "/Ucircumflex";
    res[0x86] = "/Udieresis";
    res[0xf4] = "/Ugrave";
    res[0x56] = "/V";
    res[0x57] = "/W";
    res[0x58] = "/X";
    res[0x59] = "/Y";
    res[0xd9] = "/Ydieresis";
    res[0x5a] = "/Z";
    res[0x61] = "/a";
    res[0x87] = "/aacute";
    res[0x89] = "/acircumflex";
    res[0xab] = "/acute";
    res[0x8a] = "/adieresis";
    res[0xbe] = "/ae";
    res[0x88] = "/agrave";
    res[0x26] = "/ampersand";
    res[0x8c] = "/aring";
    res[0x5e] = "/asciicircum";
    res[0x7e] = "/asciitilde";
    res[0x2a] = "/asterisk";
    res[0x40] = "/at";
    res[0x8b] = "/atilde";
    res[0x62] = "/b";
    res[0x5c] = "/backslash";
    res[0x7c] = "/bar";
    res[0x7b] = "/braceleft";
    res[0x7d] = "/braceright";
    res[0x5b] = "/bracketleft";
    res[0x5d] = "/bracketright";
    res[0xf9] = "/breve";
    res[0xa5] = "/bullet";
    res[0x63] = "/c";
    res[0xff] = "/caron";
    res[0x8d] = "/ccedilla";
    res[0xfc] = "/cedilla";
    res[0xa2] = "/cent";
    res[0xf6] = "/circumflex";
    res[0x3a] = "/colon";
    res[0x2c] = "/comma";
    res[0xa9] = "/copyright";
    res[0xdb] = "/currency";
    res[0x64] = "/d";
    res[0xa0] = "/dagger";
    res[0xe0] = "/daggerdbl";
    res[0xa1] = "/degree";
    res[0xac] = "/dieresis";
    res[0xd6] = "/divide";
    res[0x24] = "/dollar";
    res[0xfa] = "/dotaccent";
    res[0xf5] = "/dotlessi";
    res[0x65] = "/e";
    res[0x8e] = "/eacute";
    res[0x90] = "/ecircumflex";
    res[0x91] = "/edieresis";
    res[0x8f] = "/egrave";
    res[0x38] = "/eight";
    res[0xc9] = "/ellipsis";
    res[0xd1] = "/emdash";
    res[0xd0] = "/endash";
    res[0x3d] = "/equal";
    res[0x21] = "/exclam";
    res[0xc1] = "/exclamdown";
    res[0x66] = "/f";
    res[0xde] = "/fi";
    res[0x35] = "/five";
    res[0xdf] = "/fl";
    res[0xc4] = "/florin";
    res[0x34] = "/four";
    res[0xda] = "/fraction";
    res[0x67] = "/g";
    res[0xa7] = "/germandbls";
    res[0x60] = "/grave";
    res[0x3e] = "/greater";
    res[0xc7] = "/guillemotleft";
    res[0xc8] = "/guillemotright";
    res[0xdc] = "/guilsinglleft";
    res[0xdd] = "/guilsinglright";
    res[0x68] = "/h";
    res[0xfd] = "/hungarumlaut";
    res[0x2d] = "/hyphen";
    res[0x69] = "/i";
    res[0x92] = "/iacute";
    res[0x94] = "/icircumflex";
    res[0x95] = "/idieresis";
    res[0x93] = "/igrave";
    res[0x6a] = "/j";
    res[0x6b] = "/k";
    res[0x6c] = "/l";
    res[0x3c] = "/less";
    res[0xc2] = "/logicalnot";
    res[0x6d] = "/m";
    res[0xf8] = "/macron";
    res[0xb5] = "/mu";
    res[0x6e] = "/n";
    res[0x39] = "/nine";
    res[0x96] = "/ntilde";
    res[0x23] = "/numbersign";
    res[0x6f] = "/o";
    res[0x97] = "/oacute";
    res[0x99] = "/ocircumflex";
    res[0x9a] = "/odieresis";
    res[0xcf] = "/oe";
    res[0xfe] = "/ogonek";
    res[0x98] = "/ograve";
    res[0x31] = "/one";
    res[0xbb] = "/ordfeminine";
    res[0xbc] = "/ordmasculine";
    res[0xbf] = "/oslash";
    res[0x9b] = "/otilde";
    res[0x70] = "/p";
    res[0xa6] = "/paragraph";
    res[0x28] = "/parenleft";
    res[0x29] = "/parenright";
    res[0x25] = "/percent";
    res[0x2e] = "/period";
    res[0xe1] = "/periodcentered";
    res[0xe4] = "/perthousand";
    res[0x2b] = "/plus";
    res[0xb1] = "/plusminus";
    res[0x71] = "/q";
    res[0x3f] = "/question";
    res[0xc0] = "/questiondown";
    res[0x22] = "/quotedbl";
    res[0xe3] = "/quotedblbase";
    res[0xd2] = "/quotedblleft";
    res[0xd3] = "/quotedblright";
    res[0xd4] = "/quoteleft";
    res[0xd5] = "/quoteright";
    res[0xe2] = "/quotesinglbase";
    res[0x27] = "/quotesingle";
    res[0x72] = "/r";
    res[0xa8] = "/registered";
    res[0xfb] = "/ring";
    res[0x73] = "/s";
    res[0xa4] = "/section";
    res[0x3b] = "/semicolon";
    res[0x37] = "/seven";
    res[0x36] = "/six";
    res[0x2f] = "/slash";
    res[0x20] = "/space";
    res[0xa3] = "/sterling";
    res[0x74] = "/t";
    res[0x33] = "/three";
    res[0xf7] = "/tilde";
    res[0xaa] = "/trademark";
    res[0x32] = "/two";
    res[0x75] = "/u";
    res[0x9c] = "/uacute";
    res[0x9e] = "/ucircumflex";
    res[0x9f] = "/udieresis";
    res[0x9d] = "/ugrave";
    res[0x5f] = "/underscore";
    res[0x76] = "/v";
    res[0x77] = "/w";
    res[0x78] = "/x";
    res[0x79] = "/y";
    res[0xd8] = "/ydieresis";
    res[0xb4] = "/yen";
    res[0x7a] = "/z";
    res[0x30] = "/zero";
  }

  if(encoding == "/WinAnsiEncoding")
  {
    res[0x41] = "/A";
    res[0xc6] = "/AE";
    res[0xc1] = "/Aacute";
    res[0xc2] = "/Acircumflex";
    res[0xc4] = "/Adieresis";
    res[0xc0] = "/Agrave";
    res[0xc5] = "/Aring";
    res[0xc3] = "/Atilde";
    res[0x42] = "/B";
    res[0x43] = "/C";
    res[0xc7] = "/Ccedilla";
    res[0x44] = "/D";
    res[0x45] = "/E";
    res[0xc9] = "/Eacute";
    res[0xca] = "/Ecircumflex";
    res[0xcb] = "/Edieresis";
    res[0xc8] = "/Egrave";
    res[0xd0] = "/Eth";
    res[0x80] = "/Euro";
    res[0x46] = "/F";
    res[0x47] = "/G";
    res[0x48] = "/H";
    res[0x49] = "/I";
    res[0xcd] = "/Iacute";
    res[0xce] = "/Icircumflex";
    res[0xcf] = "/Idieresis";
    res[0xcc] = "/Igrave";
    res[0x4a] = "/J";
    res[0x4b] = "/K";
    res[0x4c] = "/L";
    res[0x4d] = "/M";
    res[0x4e] = "/N";
    res[0xd1] = "/Ntilde";
    res[0x4f] = "/O";
    res[0x8c] = "/OE";
    res[0xd3] = "/Oacute";
    res[0xd4] = "/Ocircumflex";
    res[0xd6] = "/Odieresis";
    res[0xd2] = "/Ograve";
    res[0xd8] = "/Oslash";
    res[0xd5] = "/Otilde";
    res[0x50] = "/P";
    res[0x51] = "/Q";
    res[0x52] = "/R";
    res[0x53] = "/S";
    res[0x8a] = "/Scaron";
    res[0x54] = "/T";
    res[0xde] = "/Thorn";
    res[0x55] = "/U";
    res[0xda] = "/Uacute";
    res[0xdb] = "/Ucircumflex";
    res[0xdc] = "/Udieresis";
    res[0xd9] = "/Ugrave";
    res[0x56] = "/V";
    res[0x57] = "/W";
    res[0x58] = "/X";
    res[0x59] = "/Y";
    res[0xdd] = "/Yacute";
    res[0x9f] = "/Ydieresis";
    res[0x5a] = "/Z";
    res[0x8e] = "/Zcaron";
    res[0x61] = "/a";
    res[0xe1] = "/aacute";
    res[0xe2] = "/acircumflex";
    res[0xb4] = "/acute";
    res[0xe4] = "/adieresis";
    res[0xe6] = "/ae";
    res[0xe0] = "/agrave";
    res[0x26] = "/ampersand";
    res[0xe5] = "/aring";
    res[0x5e] = "/asciicircum";
    res[0x7e] = "/asciitilde";
    res[0x2a] = "/asterisk";
    res[0x40] = "/at";
    res[0xe3] = "/atilde";
    res[0x62] = "/b";
    res[0x5c] = "/backslash";
    res[0x7c] = "/bar";
    res[0x7b] = "/braceleft";
    res[0x7d] = "/braceright";
    res[0x5b] = "/bracketleft";
    res[0x5d] = "/bracketright";
    res[0xa6] = "/brokenbar";
    res[0x95] = "/bullet";
    res[0x63] = "/c";
    res[0xe7] = "/ccedilla";
    res[0xb8] = "/cedilla";
    res[0xa2] = "/cent";
    res[0x88] = "/circumflex";
    res[0x3a] = "/colon";
    res[0x2c] = "/comma";
    res[0xa9] = "/copyright";
    res[0xa4] = "/currency";
    res[0x64] = "/d";
    res[0x86] = "/dagger";
    res[0x87] = "/daggerdbl";
    res[0xb0] = "/degree";
    res[0xa8] = "/dieresis";
    res[0xf7] = "/divide";
    res[0x24] = "/dollar";
    res[0x65] = "/e";
    res[0xe9] = "/eacute";
    res[0xea] = "/ecircumflex";
    res[0xeb] = "/edieresis";
    res[0xe8] = "/egrave";
    res[0x38] = "/eight";
    res[0x85] = "/ellipsis";
    res[0x97] = "/emdash";
    res[0x96] = "/endash";
    res[0x3d] = "/equal";
    res[0xf0] = "/eth";
    res[0x21] = "/exclam";
    res[0xa1] = "/exclamdown";
    res[0x66] = "/f";
    res[0x35] = "/five";
    res[0x83] = "/florin";
    res[0x34] = "/four";
    res[0x67] = "/g";
    res[0xdf] = "/germandbls";
    res[0x60] = "/grave";
    res[0x3e] = "/greater";
    res[0xab] = "/guillemotleft";
    res[0xbb] = "/guillemotright";
    res[0x8b] = "/guilsinglleft";
    res[0x9b] = "/guilsinglright";
    res[0x68] = "/h";
    res[0x2d] = "/hyphen";
    res[0x69] = "/i";
    res[0xed] = "/iacute";
    res[0xee] = "/icircumflex";
    res[0xef] = "/idieresis";
    res[0xec] = "/igrave";
    res[0x6a] = "/j";
    res[0x6b] = "/k";
    res[0x6c] = "/l";
    res[0x3c] = "/less";
    res[0xac] = "/logicalnot";
    res[0x6d] = "/m";
    res[0xaf] = "/macron";
    res[0xb5] = "/mu";
    res[0xd7] = "/multiply";
    res[0x6e] = "/n";
    res[0x39] = "/nine";
    res[0xf1] = "/ntilde";
    res[0x23] = "/numbersign";
    res[0x6f] = "/o";
    res[0xf3] = "/oacute";
    res[0xf4] = "/ocircumflex";
    res[0xf6] = "/odieresis";
    res[0x9c] = "/oe";
    res[0xf2] = "/ograve";
    res[0x31] = "/one";
    res[0xbd] = "/onehalf";
    res[0xbc] = "/onequarter";
    res[0xb9] = "/onesuperior";
    res[0xaa] = "/ordfeminine";
    res[0xba] = "/ordmasculine";
    res[0xf8] = "/oslash";
    res[0xf5] = "/otilde";
    res[0x70] = "/p";
    res[0xb6] = "/paragraph";
    res[0x28] = "/parenleft";
    res[0x29] = "/parenright";
    res[0x25] = "/percent";
    res[0x2e] = "/period";
    res[0xb7] = "/periodcentered";
    res[0x89] = "/perthousand";
    res[0x2b] = "/plus";
    res[0xb1] = "/plusminus";
    res[0x71] = "/q";
    res[0x3f] = "/question";
    res[0xbf] = "/questiondown";
    res[0x22] = "/quotedbl";
    res[0x84] = "/quotedblbase";
    res[0x93] = "/quotedblleft";
    res[0x94] = "/quotedblright";
    res[0x91] = "/quoteleft";
    res[0x92] = "/quoteright";
    res[0x82] = "/quotesinglbase";
    res[0x27] = "/quotesingle";
    res[0x72] = "/r";
    res[0xae] = "/registered";
    res[0x73] = "/s";
    res[0x9a] = "/scaron";
    res[0xa7] = "/section";
    res[0x3b] = "/semicolon";
    res[0x37] = "/seven";
    res[0x36] = "/six";
    res[0x2f] = "/slash";
    res[0x20] = "/space";
    res[0xa3] = "/sterling";
    res[0x74] = "/t";
    res[0xfe] = "/thorn";
    res[0x33] = "/three";
    res[0xbe] = "/threequarters";
    res[0xb3] = "/threesuperior";
    res[0x98] = "/tilde";
    res[0x99] = "/trademark";
    res[0x32] = "/two";
    res[0xb2] = "/twosuperior";
    res[0x75] = "/u";
    res[0xfa] = "/uacute";
    res[0xfb] = "/ucircumflex";
    res[0xfc] = "/udieresis";
    res[0xf9] = "/ugrave";
    res[0x5f] = "/underscore";
    res[0x76] = "/v";
    res[0x77] = "/w";
    res[0x78] = "/x";
    res[0x79] = "/y";
    res[0xfd] = "/yacute";
    res[0xff] = "/ydieresis";
    res[0xa5] = "/yen";
    res[0x7a] = "/z";
    res[0x9e] = "/zcaron";
    res[0x30] = "/zero";
  }

  if(encoding == "/PDFDocEncoding")
  {
    res[0x41] = "/A";
    res[0xc6] = "/AE";
    res[0xc1] = "/Aacute";
    res[0xc2] = "/Acircumflex";
    res[0xc4] = "/Adieresis";
    res[0xc0] = "/Agrave";
    res[0xc5] = "/Aring";
    res[0xc3] = "/Atilde";
    res[0x42] = "/B";
    res[0x43] = "/C";
    res[0xc7] = "/Ccedilla";
    res[0x44] = "/D";
    res[0x45] = "/E";
    res[0xc9] = "/Eacute";
    res[0xca] = "/Ecircumflex";
    res[0xcb] = "/Edieresis";
    res[0xc8] = "/Egrave";
    res[0xd0] = "/Eth";
    res[0xa0] = "/Euro";
    res[0x46] = "/F";
    res[0x47] = "/G";
    res[0x48] = "/H";
    res[0x49] = "/I";
    res[0xcd] = "/Iacute";
    res[0xce] = "/Icircumflex";
    res[0xcf] = "/Idieresis";
    res[0xcc] = "/Igrave";
    res[0x4a] = "/J";
    res[0x4b] = "/K";
    res[0x4c] = "/L";
    res[0x95] = "/Lslash";
    res[0x4d] = "/M";
    res[0x4e] = "/N";
    res[0xd1] = "/Ntilde";
    res[0x4f] = "/O";
    res[0x96] = "/OE";
    res[0xd3] = "/Oacute";
    res[0xd4] = "/Ocircumflex";
    res[0xd6] = "/Odieresis";
    res[0xd2] = "/Ograve";
    res[0xd8] = "/Oslash";
    res[0xd5] = "/Otilde";
    res[0x50] = "/P";
    res[0x51] = "/Q";
    res[0x52] = "/R";
    res[0x53] = "/S";
    res[0x97] = "/Scaron";
    res[0x54] = "/T";
    res[0xde] = "/Thorn";
    res[0x55] = "/U";
    res[0xda] = "/Uacute";
    res[0xdb] = "/Ucircumflex";
    res[0xdc] = "/Udieresis";
    res[0xd9] = "/Ugrave";
    res[0x56] = "/V";
    res[0x57] = "/W";
    res[0x58] = "/X";
    res[0x59] = "/Y";
    res[0xdd] = "/Yacute";
    res[0x98] = "/Ydieresis";
    res[0x5a] = "/Z";
    res[0x99] = "/Zcaron";
    res[0x61] = "/a";
    res[0xe1] = "/aacute";
    res[0xe2] = "/acircumflex";
    res[0xb4] = "/acute";
    res[0xe4] = "/adieresis";
    res[0xe6] = "/ae";
    res[0xe0] = "/agrave";
    res[0x26] = "/ampersand";
    res[0xe5] = "/aring";
    res[0x5e] = "/asciicircum";
    res[0x7e] = "/asciitilde";
    res[0x2a] = "/asterisk";
    res[0x40] = "/at";
    res[0xe3] = "/atilde";
    res[0x62] = "/b";
    res[0x5c] = "/backslash";
    res[0x7c] = "/bar";
    res[0x7b] = "/braceleft";
    res[0x7d] = "/braceright";
    res[0x5b] = "/bracketleft";
    res[0x5d] = "/bracketright";
    res[0x18] = "/breve";
    res[0xa6] = "/brokenbar";
    res[0x80] = "/bullet";
    res[0x63] = "/c";
    res[0x19] = "/caron";
    res[0xe7] = "/ccedilla";
    res[0xb8] = "/cedilla";
    res[0xa2] = "/cent";
    res[0x1a] = "/circumflex";
    res[0x3a] = "/colon";
    res[0x2c] = "/comma";
    res[0xa9] = "/copyright";
    res[0xa4] = "/currency";
    res[0x64] = "/d";
    res[0x81] = "/dagger";
    res[0x82] = "/daggerdbl";
    res[0xb0] = "/degree";
    res[0xa8] = "/dieresis";
    res[0xf7] = "/divide";
    res[0x24] = "/dollar";
    res[0x1b] = "/dotaccent";
    res[0x9a] = "/dotlessi";
    res[0x65] = "/e";
    res[0xe9] = "/eacute";
    res[0xea] = "/ecircumflex";
    res[0xeb] = "/edieresis";
    res[0xe8] = "/egrave";
    res[0x38] = "/eight";
    res[0x83] = "/ellipsis";
    res[0x84] = "/emdash";
    res[0x85] = "/endash";
    res[0x3d] = "/equal";
    res[0xf0] = "/eth";
    res[0x21] = "/exclam";
    res[0xa1] = "/exclamdown";
    res[0x66] = "/f";
    res[0x93] = "/fi";
    res[0x35] = "/five";
    res[0x94] = "/fl";
    res[0x86] = "/florin";
    res[0x34] = "/four";
    res[0x87] = "/fraction";
    res[0x67] = "/g";
    res[0xdf] = "/germandbls";
    res[0x60] = "/grave";
    res[0x3e] = "/greater";
    res[0xab] = "/guillemotleft";
    res[0xbb] = "/guillemotright";
    res[0x88] = "/guilsinglleft";
    res[0x89] = "/guilsinglright";
    res[0x68] = "/h";
    res[0x1c] = "/hungarumlaut";
    res[0x2d] = "/hyphen";
    res[0x69] = "/i";
    res[0xed] = "/iacute";
    res[0xee] = "/icircumflex";
    res[0xef] = "/idieresis";
    res[0xec] = "/igrave";
    res[0x6a] = "/j";
    res[0x6b] = "/k";
    res[0x6c] = "/l";
    res[0x3c] = "/less";
    res[0xac] = "/logicalnot";
    res[0x9b] = "/lslash";
    res[0x6d] = "/m";
    res[0xaf] = "/macron";
    res[0x8a] = "/minus";
    res[0xb5] = "/mu";
    res[0xd7] = "/multiply";
    res[0x6e] = "/n";
    res[0x39] = "/nine";
    res[0xf1] = "/ntilde";
    res[0x23] = "/numbersign";
    res[0x6f] = "/o";
    res[0xf3] = "/oacute";
    res[0xf4] = "/ocircumflex";
    res[0xf6] = "/odieresis";
    res[0x9c] = "/oe";
    res[0x1d] = "/ogonek";
    res[0xf2] = "/ograve";
    res[0x31] = "/one";
    res[0xbd] = "/onehalf";
    res[0xbc] = "/onequarter";
    res[0xb9] = "/onesuperior";
    res[0xaa] = "/ordfeminine";
    res[0xba] = "/ordmasculine";
    res[0xf8] = "/oslash";
    res[0xf5] = "/otilde";
    res[0x70] = "/p";
    res[0xb6] = "/paragraph";
    res[0x28] = "/parenleft";
    res[0x29] = "/parenright";
    res[0x25] = "/percent";
    res[0x2e] = "/period";
    res[0xb7] = "/periodcentered";
    res[0x8b] = "/perthousand";
    res[0x2b] = "/plus";
    res[0xb1] = "/plusminus";
    res[0x71] = "/q";
    res[0x3f] = "/question";
    res[0xbf] = "/questiondown";
    res[0x22] = "/quotedbl";
    res[0x8c] = "/quotedblbase";
    res[0x8d] = "/quotedblleft";
    res[0x8e] = "/quotedblright";
    res[0x8f] = "/quoteleft";
    res[0x90] = "/quoteright";
    res[0x91] = "/quotesinglbase";
    res[0x27] = "/quotesingle";
    res[0x72] = "/r";
    res[0xae] = "/registered";
    res[0x1e] = "/ring";
    res[0x73] = "/s";
    res[0x9d] = "/scaron";
    res[0xa7] = "/section";
    res[0x3b] = "/semicolon";
    res[0x37] = "/seven";
    res[0x36] = "/six";
    res[0x2f] = "/slash";
    res[0x20] = "/space";
    res[0xa3] = "/sterling";
    res[0x74] = "/t";
    res[0xfe] = "/thorn";
    res[0x33] = "/three";
    res[0xbe] = "/threequarters";
    res[0xb3] = "/threesuperior";
    res[0x1f] = "/tilde";
    res[0x92] = "/trademark";
    res[0x32] = "/two";
    res[0xb2] = "/twosuperior";
    res[0x75] = "/u";
    res[0xfa] = "/uacute";
    res[0xfb] = "/ucircumflex";
    res[0xfc] = "/udieresis";
    res[0xf9] = "/ugrave";
    res[0x5f] = "/underscore";
    res[0x76] = "/v";
    res[0x77] = "/w";
    res[0x78] = "/x";
    res[0x79] = "/y";
    res[0xfd] = "/yacute";
    res[0xff] = "/ydieresis";
    res[0xa5] = "/yen";
    res[0x7a] = "/z";
    res[0x9e] = "/zcaron";
    res[0x30] = "/zero";
  }
  return res;
}

/*--------------------------------------------------------------------------*/

std::string
parseUnicode(std::string s, std::map<std::string, std::string>& UM)
{
  std::vector<std::string> hstrings;
  for(auto i : s) {int a = i; hstrings.push_back(intToHexstring(a));}
  for(auto &i : hstrings) if(UM.find(i) != UM.end()) i = UM[i];
  std::string res;
  for(auto i : hstrings) res += namesToChar(i, "/WinAnsiEncoding");
  return res;
}

/*--------------------------------------------------------------------------*/

std::string defaultUnicode(document& d, std::string s)
{
  std::vector<std::string> hstrings;
  std::map<std::string, std::string> &UM = UCM;
  for(auto i : s) hstrings.push_back(intToHexstring((int) i));
  for(auto &i : hstrings) if(UM.find(i) != UM.end()) i = UM[i];
  std::string res;
  for(auto i : hstrings) res += namesToChar(i, "/WinAnsiEncoding");
  return res;
}

/*--------------------------------------------------------------------------*/

std::vector<std::string> baseEncoding(const std::string& enc)
{
  if(enc == "/WinAnsiEncoding")
  {
    std::vector<std::string> winAnsi =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space",
      "/exclam", "/quotedbl", "/numbersign", "/dollar", "/percent",
      "/ampersand", "/quotesingle", "/parenleft", "/parenright", "/asterisk",
      "/plus", "/comma", "/hyphen", "/period", "/slash", "/zero", "/one",
      "/two", "/three", "/four", "/five", "/six", "/seven", "/eight", "/nine",
      "/colon", "/semicolon", "/less", "/equal", "/greater", "/question", "/at",
      "/A", "/B", "/C", "/D", "/E", "/F", "/G", "/H", "/I", "/J", "/K", "/L",
      "/M", "/N", "/O", "/P", "/Q", "/R", "/S", "/T", "/U", "/V", "/W", "/X",
      "/Y", "/Z", "/bracketleft", "/backslash", "/bracketright", "/asciicircum",
      "/underscore", "/grave", "/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h",
      "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p", "/q", "/r", "/s", "/t",
      "/u", "/v", "/w", "/x", "/y", "/z", "/braceleft", "/bar", "/braceright",
      "/asciitilde", "/bullet", "/Euro", "/bullet", "/quotesinglbase",
      "/florin", "/quotedblbase", "/ellipsis", "/dagger", "/daggerdbl",
      "/circumflex", "/perthousand", "/Scaron", "/guilsinglleft", "/OE",
      "/bullet", "/Zcaron", "/bullet", "/bullet", "/quoteleft", "/quoteright",
      "/quotedblleft", "/quotedblright", "/bullet", "/endash", "/emdash",
      "/tilde", "/trademark", "/scaron", "/guilsinglright", "/oe", "/bullet",
      "/zcaron", "/Ydieresis", "/space", "/exclamdown", "/cent", "/sterling",
      "/currency", "/yen", "/brokenbar", "/section", "/dieresis", "/copyright",
      "/ordfeminine", "/guillemotleft", "/logicalnot", "/hyphen", "/registered",
      "/macron", "/degree", "/plusminus", "/twosuperior", "/threesuperior",
      "/acute", "/mu", "/paragraph", "/periodcentered", "/cedilla",
      "/onesuperior", "/ordmasculine", "/guillemotright", "/onequarter",
      "/onehalf", "/threequarters", "/questiondown", "/Agrave", "/Aacute",
      "/Acircumflex", "/Atilde", "/Adieresis", "/Aring", "/AE", "/Ccedilla",
      "/Egrave", "/Eacute", "/Ecircumflex", "/Edieresis", "/Igrave", "/Iacute",
      "/Icircumflex", "/Idieresis", "/Eth", "/Ntilde", "/Ograve", "/Oacute",
      "/Ocircumflex", "/Otilde", "/Odieresis", "/multiply", "/Oslash",
      "/Ugrave", "/Uacute", "/Ucircumflex", "/Udieresis", "/Yacute", "/Thorn",
      "/germandbls", "/agrave", "/aacute", "/acircumflex", "/atilde",
      "/adieresis", "/aring", "/ae", "/ccedilla", "/egrave", "/eacute",
      "/ecircumflex", "/edieresis", "/igrave", "/iacute", "/icircumflex",
      "/idieresis", "/eth", "/ntilde", "/ograve", "/oacute", "/ocircumflex",
      "/otilde", "/odieresis", "/divide", "/oslash", "/ugrave", "/uacute",
      "/ucircumflex", "/udieresis", "/yacute", "/thorn", "/ydieresis"
      };
    return winAnsi;
  }

  if(enc == "/MacRomanEncoding")
  {
    std::vector<std::string> macRoman =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space",
      "/exclam", "/quotedbl", "/numbersign", "/dollar", "/percent",
      "/ampersand", "/quotesingle", "/parenleft", "/parenright", "/asterisk",
      "/plus", "/comma", "/hyphen", "/period", "/slash", "/zero", "/one",
      "/two", "/three", "/four", "/five", "/six", "/seven", "/eight", "/nine",
      "/colon", "/semicolon", "/less", "/equal", "/greater", "/question", "/at",
      "/A", "/B", "/C", "/D", "/E", "/F", "/G", "/H", "/I", "/J", "/K", "/L",
      "/M", "/N", "/O", "/P", "/Q", "/R", "/S", "/T", "/U", "/V", "/W", "/X",
      "/Y", "/Z", "/bracketleft", "/backslash", "/bracketright", "/asciicircum",
      "/underscore", "/grave", "/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h",
      "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p", "/q", "/r", "/s", "/t",
      "/u", "/v", "/w", "/x", "/y", "/z", "/braceleft", "/bar", "/braceright",
      "/asciitilde", "", "/Adieresis", "/Aring", "/Ccedilla", "/Eacute",
      "/Ntilde", "/Odieresis", "/Udieresis", "/aacute", "/agrave",
      "/acircumflex", "/adieresis", "/atilde", "/aring", "/ccedilla", "/eacute",
      "/egrave", "/ecircumflex", "/edieresis", "/iacute", "/igrave",
      "/icircumflex", "/idieresis", "/ntilde", "/oacute", "/ograve",
      "/ocircumflex", "/odieresis", "/otilde", "/uacute", "/ugrave",
      "/ucircumflex", "/udieresis", "/dagger", "/degree", "/cent", "/sterling",
      "/section", "/bullet", "/paragraph", "/germandbls", "/registered",
      "/copyright", "/trademark", "/acute", "/dieresis", "/notequal", "/AE",
      "/Oslash", "/infinity", "/plusminus", "/lessequal", "/greaterequal",
      "/yen", "/mu", "/partialdiff", "/summation", "/product", "/pi",
      "/integral", "/ordfeminine", "/ordmasculine", "/Omega", "/ae", "/oslash",
      "/questiondown", "/exclamdown", "/logicalnot", "/radical", "/florin",
      "/approxequal", "/Delta", "/guillemotleft", "/guillemotright",
      "/ellipsis", "/space", "/Agrave", "/Atilde", "/Otilde", "/OE", "/oe",
      "/endash", "/emdash", "/quotedblleft", "/quotedblright", "/quoteleft",
      "/quoteright", "/divide", "/lozenge", "/ydieresis", "/Ydieresis",
      "/fraction", "/currency", "/guilsinglleft", "/guilsinglright", "/fi",
      "/fl", "/daggerdbl", "/periodcentered", "/quotesinglbase",
      "/quotedblbase", "/perthousand", "/Acircumflex", "/Ecircumflex",
      "/Aacute", "/Edieresis", "/Egrave", "/Iacute", "/Icircumflex",
      "/Idieresis", "/Igrave", "/Oacute", "/Ocircumflex", "/apple", "/Ograve",
      "/Uacute", "/Ucircumflex", "/Ugrave", "/dotlessi", "/circumflex",
      "/tilde", "/macron", "/breve", "/dotaccent", "/ring", "/cedilla",
      "/hungarumlaut", "/ogonek", "/caron"
      };
    return macRoman;
  }
  if(enc == "/MacExpertEncoding")
  {
    std::vector<std::string> expert =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space",
      "/exclamsmall", "/Hungarumlautsmall", "", "/dollaroldstyle",
      "/dollarsuperior", "/ampersandsmall", "/Acutesmall", "/parenleftsuperior",
      "/parenrightsuperior", "/twodotenleader", "/onedotenleader", "/comma",
      "/hyphen", "/period", "/fraction", "/zerooldstyle", "/oneoldstyle",
      "/twooldstyle", "/threeoldstyle", "/fouroldstyle", "/fiveoldstyle",
      "/sixoldstyle", "/sevenoldstyle", "/eightoldstyle", "/nineoldstyle",
      "/colon", "/semicolon", "/commasuperior", "/threequartersemdash",
      "/periodsuperior", "/questionsmall", "", "/asuperior",  "/bsuperior",
      "/centsuperior", "/dsuperior", "/esuperior", "", "", "", "/isuperior", "",
      "", "/lsuperior", "/msuperior", "/nsuperior", "/osuperior", "", "",
      "/rsuperior", "/ssuperior", "/tsuperior", "", "/ff", "/fi", "/fl", "/ffi",
      "/ffl", "/parenleftinferior", "", "/parenrightinferior",
      "/Circumflexsmall", "/hyphensuperior", "/Gravesmall", "/Asmall",
      "/Bsmall", "/Csmall", "/Dsmall", "/Esmall", "/Fsmall", "/Gsmall",
      "/Hsmall", "/Ismall", "/Jsmall", "/Ksmall", "/Lsmall", "/Msmall",
      "/Nsmall", "/Osmall", "/Psmall", "/Qsmall", "/Rsmall", "/Ssmall",
      "/Tsmall", "/Usmall", "/Vsmall", "/Wsmall", "/Xsmall", "/Ysmall",
      "/Zsmall", "/colonmonetary", "/onefitted", "/rupiah", "/Tildesmall",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "/exclamdownsmall", "/centoldstyle", "/Lslashsmall", "", "",
      "/Scaronsmall", "/Zcaronsmall", "/Dieresissmall", "/Brevesmall",
      "/Caronsmall", "", "/Dotaccentsmall", "", "", "/Macronsmall",
      "", "", "/figuredash", "/hypheninferior", "", "", "/Ogoneksmall",
      "/Ringsmall", "/Cedillasmall", "", "", "", "/onequarter", "/onehalf",
      "/threequarters", "/questiondownsmall", "/oneeighth", "/threeeighths",
      "/fiveeighths", "/seveneighths", "/onethird", "/twothirds", "", "",
      "/zerosuperior", "/onesuperior", "/twosuperior", "/threesuperior",
      "/foursuperior", "/fivesuperior", "/sixsuperior", "/sevensuperior",
      "/eightsuperior", "/ninesuperior", "/zeroinferior", "/oneinferior",
      "/twoinferior", "/threeinferior", "/fourinferior", "/fiveinferior",
      "/sixinferior", "/seveninferior", "/eightinferior", "/nineinferior",
      "/centinferior", "/dollarinferior", "/periodinferior", "/commainferior",
      "/Agravesmall", "/Aacutesmall", "/Acircumflexsmall", "/Atildesmall",
      "/Adieresissmall", "/Aringsmall", "/AEsmall", "/Ccedillasmall",
      "/Egravesmall", "/Eacutesmall", "/Ecircumflexsmall", "/Edieresissmall",
      "/Igravesmall", "/Iacutesmall", "/Icircumflexsmall", "/Idieresissmall",
      "/Ethsmall", "/Ntildesmall", "/Ogravesmall", "/Oacutesmall",
      "/Ocircumflexsmall", "/Otildesmall", "/Odieresissmall", "/OEsmall",
      "/Oslashsmall", "/Ugravesmall", "/Uacutesmall", "/Ucircumflexsmall",
      "/Udieresissmall", "/Yacutesmall", "/Thornsmall", "/Ydieresissmall"
      };
    return expert;
  }
  if(enc == "/symbolEncoding")
  {
    std::vector<std::string> symbol =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space",
      "/exclam", "/universal", "/numbersign", "/existential", "/percent",
      "/ampersand", "/suchthat", "/parenleft", "/parenright", "/asteriskmath",
      "/plus", "/comma", "/minus", "/period", "/slash", "/zero", "/one", "/two",
      "/three", "/four", "/five", "/six", "/seven", "/eight", "/nine", "/colon",
      "/semicolon", "/less", "/equal", "/greater", "/question", "/congruent",
      "/Alpha", "/Beta", "/Chi", "/Delta", "/Epsilon", "/Phi", "/Gamma", "/Eta",
      "/Iota", "/theta1", "/Kappa", "/Lambda", "/Mu", "/Nu", "/Omicron", "/Pi",
      "/Theta", "/Rho", "/Sigma", "/Tau", "/Upsilon", "/sigma1", "/Omega",
      "/Xi", "/Psi", "/Zeta", "/bracketleft", "/therefore", "/bracketright",
      "/perpendicular", "/underscore", "/radicalex", "/alpha", "/beta", "/chi",
      "/delta", "/epsilon", "/phi", "/gamma", "/eta", "/iota", "/phi1",
      "/kappa", "/lambda", "/mu", "/nu", "/omicron", "/pi", "/theta", "/rho",
      "/sigma", "/tau", "/upsilon", "/omega1", "/omega", "/xi", "/psi", "/zeta",
      "/braceleft", "/bar", "/braceright", "/similar", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "/Upsilon1", "/minute",
      "/lessequal", "/fraction", "/infinity", "/florin", "/club", "/diamond",
      "/heart", "/spade", "/arrowboth", "/arrowleft", "/arrowup", "/arrowright",
      "/arrowdown", "/degree", "/plusminus", "/second", "/greaterequal",
      "/multiply", "/proportional", "/partialdiff", "/bullet", "/divide",
      "/notequal", "/equivalence", "/approxequal", "/ellipsis", "/arrowvertex",
      "/arrowhorizex", "/carriagereturn", "/aleph", "/Ifraktur", "/Rfraktur",
      "/weierstrass", "/circlemultiply", "/circleplus", "/emptyset",
      "/intersection", "/union", "/propersuperset", "/reflexsuperset",
      "/notsubset", "/propersubset", "/reflexsubset", "/element", "/notelement",
      "/angle", "/gradient", "/registerserif", "/copyrightserif",
      "/trademarkserif", "/product", "/radical", "/dotmath", "/logicalnot",
      "/logicaland", "/logicalor", "/arrowdblboth", "/arrowdblleft",
      "/arrowdblup", "/arrowdblright", "/arrowdbldown", "/lozenge",
      "/angleleft", "/registersans", "/copyrightsans", "/trademarksans",
      "/summation", "/parenlefttp", "/parenleftex", "/parenleftbt",
      "/bracketlefttp", "/bracketleftex", "/bracketleftbt", "/bracelefttp",
      "/braceleftmid", "/braceleftbt", "/braceex", "", "/angleright",
      "/integral", "/integraltp", "/integralex", "/integralbt", "/parenrighttp",
      "/parenrightex", "/parenrightbt", "/bracketrighttp", "/bracketrightex",
      "/bracketrightbt", "/bracerighttp", "/bracerightmid", "/bracerightbt", ""
      };
    return symbol;
  }
  if(enc == "/zapfDingbatEncoding")
  {
    std::vector<std::string> zapfDingbats =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space", "/a1",
      "/a2", "/a202", "/a3", "/a4", "/a5", "/a119", "/a118", "/a117", "/a11",
      "/a12", "/a13", "/a14", "/a15", "/a16", "/a105", "/a17", "/a18", "/a19",
      "/a20", "/a21", "/a22", "/a23", "/a24", "/a25", "/a26", "/a27", "/a28",
      "/a6", "/a7", "/a8", "/a9", "/a10", "/a29", "/a30", "/a31", "/a32",
      "/a33", "/a34", "/a35", "/a36", "/a37", "/a38", "/a39", "/a40", "/a41",
      "/a42", "/a43", "/a44", "/a45", "/a46", "/a47", "/a48", "/a49", "/a50",
      "/a51", "/a52", "/a53", "/a54", "/a55", "/a56", "/a57", "/a58", "/a59",
      "/a60", "/a61", "/a62", "/a63", "/a64", "/a65", "/a66", "/a67", "/a68",
      "/a69", "/a70", "/a71", "/a72", "/a73", "/a74", "/a203", "/a75", "/a204",
      "/a76", "/a77", "/a78", "/a79", "/a81", "/a82", "/a83", "/a84", "/a97",
      "/a98", "/a99", "/a100", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "/a101", "/a102", "/a103", "/a104", "/a106", "/a107",
      "/a108", "/a112", "/a111", "/a110", "/a109", "/a120", "/a121", "/a122",
      "/a123", "/a124", "/a125", "/a126", "/a127", "/a128", "/a129", "/a130",
      "/a131", "/a132", "/a133", "/a134", "/a135", "/a136", "/a137", "/a138",
      "/a139", "/a140", "/a141", "/a142", "/a143", "/a144", "/a145", "/a146",
      "/a147", "/a148", "/a149", "/a150", "/a151", "/a152", "/a153", "/a154",
      "/a155", "/a156", "/a157", "/a158", "/a159", "/a160", "/a161", "/a163",
      "/a164", "/a196", "/a165", "/a192", "/a166", "/a167", "/a168", "/a169",
      "/a170", "/a171", "/a172", "/a173", "/a162", "/a174", "/a175", "/a176",
      "/a177", "/a178", "/a179", "/a193", "/a180", "/a199", "/a181", "/a200",
      "/a182", "", "/a201", "/a183", "/a184", "/a197", "/a185", "/a194",
      "/a198", "/a186", "/a195", "/a187", "/a188", "/a189", "/a190", "/a191", ""
      };
    return zapfDingbats;
  }
  else
  {
    std::vector<std::string> standard =
      {
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "/space",
      "/exclam", "/quotedbl", "/numbersign", "/dollar", "/percent",
      "/ampersand", "/quoteright", "/parenleft", "/parenright", "/asterisk",
      "/plus", "/comma", "/hyphen", "/period", "/slash", "/zero", "/one",
      "/two", "/three", "/four", "/five", "/six", "/seven", "/eight", "/nine",
      "/colon", "/semicolon", "/less", "/equal", "/greater", "/question", "/at",
      "/A", "/B", "/C", "/D", "/E", "/F", "/G", "/H", "/I", "/J", "/K", "/L",
      "/M", "/N", "/O", "/P", "/Q", "/R", "/S", "/T", "/U", "/V", "/W", "/X",
      "/Y", "/Z", "/bracketleft", "/backslash", "/bracketright", "/asciicircum",
      "/underscore", "/quoteleft", "/a", "/b", "/c", "/d", "/e", "/f", "/g",
      "/h", "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p", "/q", "/r", "/s",
      "/t", "/u", "/v", "/w", "/x", "/y", "/z", "/braceleft", "/bar",
      "/braceright", "/asciitilde", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "/exclamdown", "/cent", "/sterling", "/fraction",
      "/yen", "/florin", "/section", "/currency", "/quotesingle",
      "/quotedblleft", "/guillemotleft", "/guilsinglleft", "/guilsinglright",
      "/fi", "/fl", "", "/endash", "/dagger", "/daggerdbl", "/periodcentered",
      "", "/paragraph", "/bullet", "/quotesinglbase", "/quotedblbase",
      "/quotedblright", "/guillemotright", "/ellipsis", "/perthousand", "",
      "/questiondown", "", "/grave", "/acute", "/circumflex", "/tilde",
      "/macron", "/breve", "/dotaccent", "/dieresis", "", "/ring", "/cedilla",
      "", "/hungarumlaut", "/ogonek", "/caron", "/emdash", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "", "", "", "/AE", "", "/ordfeminine", "",
      "", "", "", "/Lslash", "/Oslash", "/OE", "/ordmasculine", "", "", "", "",
      "", "/ae", "", "", "", "/dotlessi", "", "", "/lslash", "/oslash", "/oe",
      "/germandbls", "", "", "", ""
      };
    return standard;
  }
}