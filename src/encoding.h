//---------------------------------------------------------------------------//
//                                                                           //
// PDFR encoding header file                                                 //
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

#ifndef PDFR_ENCODING

//---------------------------------------------------------------------------//

#define PDFR_ENCODING

/* This is the 6th in a series of daisy - chained headers that build up the
 * tools to read and parse pdfs. In fact, it is paired with glyphwidths.h
 * in that they both come logically after document.h and together form the
 * basis for the next step, which is font creation.
 *
 * The reason that font creation comes before page creation is that pages
 * include a list of their fonts in the page description header, and the
 * program needs to know what these are.
 *
 * There are three main parts of font creation pertinent to the task of text
 * extraction: identifying the font's name, working out the width of the glyphs,
 * and working out the correspondence between code points in a pdf string and
 * the intended Unicode code points. The latter of these tasks is called
 * encoding, and is fairly complex.
 *
 * The complexity arises because there are several different methods for
 * encoding fonts in pdf. A base encoding scheme can be declared, such as
 * WinAnsi Encoding or MacRoman Encoding. These encodings are stored as static
 * private data members of the class in the form of an unordered_map, though
 * they are defined in the chartounicode.cpp file rather than encoding.cpp to
 * improve code readability.
 *
 * Whether a base encoding is specified or not, the actual encoding used can
 * be modified, for example to include Unicode characters that are not
 * available in the base encoding's character set (a common example is the
 * glyph for the ligatures ff, fi or fl). This is done using an explicit
 * mapping of input code points to standard glyph names. That means the
 * program needs to know all these glyph names and how to convert them to
 * Unicode. This is a very large mapping, and again is declared here as a
 * static member and defined in a seperate source file (adobetounicode.h)
 *
 * The encoding may instead be specified in a CMap, which is a type of
 * mapping table that usually appears in a compressed stream.
 *
 * The idea behind the encoding class is to use whichever method is required to
 * produce a mapping for each font so that each code point encountered has a
 * Unicode interpretation. It keeps the implementation private and returns
 * only its main data member - an unordered map of input characters (represented
 * as 2-byte unsigned integers or uint16_t) to unicode characters (also
 * represented as uint16_t).
 *
 * To make the code clearer, both RawChar and Unicode are typedef'd as synonyms
 * of uint16_t so we know at any time whether we are referring to input
 * characters or output characters.
 */

#include "document.h"

//---------------------------------------------------------------------------//
// The encoding class comprises constructors which use private subroutines
// and large static maps to construct the main variable data member. The
// public interface is a simple RawChar in, Unicode out translator and a
// function to get all of the encoding (RawChar) keys

class Encoding
{

public:

  // constructor
  Encoding(const dictionary&, document*);

  // public member functions
  Unicode Interpret(RawChar);       // Maps given code point to Unicode
  std::vector<RawChar> encKeys();   // Returns all input code points

private:

  // data lookup tables - defined as static, which means only a single
  // instance of each is created rather than a copy for each object.
  // Note these maps are defined in adobetounicode.h and chartounicode.h
  static std::unordered_map<std::string, Unicode> AdobeToUnicode;
  static std::unordered_map<RawChar, Unicode> macRomanEncodingToUnicode;
  static std::unordered_map<RawChar, Unicode> winAnsiEncodingToUnicode;
  static std::unordered_map<RawChar, Unicode> pdfDocEncodingToUnicode;

  // the main variable data container for the class
  std::unordered_map<RawChar, Unicode> EncodingMap;

  // private data members used as variables in creating encoding map
  dictionary fontref;       // the main font dictionary
  document* d;              // pointer to the containing document
  std::string BaseEncoding; // value of /BaseEncoding entry
  enum DiffState { NEWSYMB, NUM, NAME, STOP }; // states for /Differences lexer

  // private member functions

  // uses lexer to parse /Differences entry
  void Differences(const std::string&, std::unordered_map<RawChar, Unicode>&);

  // finds encoding dictionary, gets /basencoding and /Differences entries
  void getEncoding(dictionary&, document*);

  // parses CMap encoding ranges
  void processUnicodeRange(std::vector<std::string>&);

  // parses CMap direct char-char conversion table
  void processUnicodeChars(std::vector<std::string>&);

  // finds CMap if any and co-ordinates parsers to create mapping
  void mapUnicode(dictionary&, document*);
};

//---------------------------------------------------------------------------//

#endif
