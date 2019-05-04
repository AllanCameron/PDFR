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

/* This is the joint 6th in a series of daisy-chained headers that build up the
 * tools to read and parse pdfs. It is logically paired with glyphwidths.h
 * in that they both come after document.h and together form the basis for the
 * next step, which is font creation.
 *
 * The reason that font creation comes before page creation is that pages
 * include a list of their fonts in the page description header, and the
 * program needs to know what these are.
 *
 * There are three main parts of font creation pertinent to the task of text
 * extraction: identifying the font's name, working out the width of the glyphs,
 * and working out the correspondence between the characters in a pdf string and
 * the intended glyphs as Unicode code points. The latter of these tasks is
 * called encoding, and is fairly complex.
 *
 * The complexity arises because there are several different methods for
 * encoding fonts in pdf. First, a base encoding scheme can be declared, such as
 * WinAnsiEncoding or MacRomanEncoding. These encodings are stored as static
 * private data members of the class in the form of an unordered_map, though
 * they are defined in the chartounicode.cpp file rather than encoding.cpp to
 * improve code readability.
 *
 * Whether a base encoding is specified or not, the actual encoding used can
 * be modified, for example to include Unicode characters that are not
 * available in the base encoding's character set (a common example is the
 * glyph for the ligatures ff, fi or fl). This is done using an explicit
 * mapping of input characters ("code points") to standard glyph names. That
 * means the program needs to know all these glyph names and how to convert
 * them to Unicode. This is a very large mapping, and again is declared here as
 * a static member but defined in a seperate source file (adobetounicode.h)
 *
 * The encoding may instead be specified in a CMap, which is a type of
 * raw char to Unicode mapping table that usually appears in a (compressed)
 * pdf object stream.
 *
 * The idea behind the encoding class is to use these methods as required to
 * produce a mapping for each font so that each code point encountered has a
 * Unicode interpretation. It keeps the implementation private and its interface
 * is limited to querying its main data member - an unordered map of input
 * characters (represented as 2-byte unsigned integers or uint16_t) to Unicode
 * characters (also represented as uint16_t). Since in most cases the input
 * characters are given as single bytes, these have to be recast as two-byte
 * uints for consistency to handle the odd cases when two-byte characters are
 * supplied in the strings (as is the case with "hexstrings" or ascii-encoded
 * multi-byte character strings).
 *
 * To make the code clearer, both RawChar and Unicode are typedef'd as synonyms
 * of uint16_t so we know at any time whether we are referring to input ("raw")
 * code points or output (Unicode) characters.
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
  Encoding(const Dictionary&, std::shared_ptr<Document>);

  // public member functions
  Unicode interpret(const RawChar&);  // Maps given code point to Unicode
  std::shared_ptr<std::unordered_map<RawChar, Unicode>> encoding_keys();

private:
  // data lookup tables - defined as static, which means only a single
  // instance of each is created rather than a copy for each object.
  // Note these maps are defined in adobetounicode.h and chartounicode.h
  static std::unordered_map<std::string, Unicode> adobe_to_unicode;
  static std::unordered_map<RawChar, Unicode> macroman_to_unicode;
  static std::unordered_map<RawChar, Unicode> winansi_to_unicode;
  static std::unordered_map<RawChar, Unicode> pdfdoc_to_unicode;

  // the main variable data container for the class
  std::unordered_map<RawChar, Unicode> m_encoding_map;

  // private data members used as variables in creating encoding map
  Dictionary m_font_dictionary;       // the main font dictionary
  std::shared_ptr<Document> m_document;  // pointer to the containing document
  std::string m_base_encoding; // value of /BaseEncoding entry
  enum differences_state { NEWSYMB, NUM, NAME, STOP };

  // private member functions

  // uses lexer to parse /Differences entry
  void read_differences(const std::string&);

  // finds encoding dictionary, gets /basencoding and /Differences entries
  void read_encoding();

  // parses CMap encoding ranges
  void process_unicode_range(std::vector<std::string>&);

  // parses CMap direct char-char conversion table
  void process_unicode_chars(std::vector<std::string>&);

  // finds CMap if any and co-ordinates parsers to create mapping
  void map_unicode();
};

//---------------------------------------------------------------------------//

#endif
