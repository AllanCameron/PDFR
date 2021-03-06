//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Encoding header file                                                //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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

#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
#include<utility>

class Dictionary;
class Document;
using Unicode = uint16_t;
using RawChar = uint16_t;

//---------------------------------------------------------------------------//
// The encoding class comprises constructors which use private subroutines
// and large static maps to construct the main variable data member. The
// public interface is a simple RawChar in, Unicode out translator and a
// function to get all of the encoding (RawChar) keys

class Encoding
{
 public:
  // Constructor
  Encoding(Dictionary& font_dictionary,
           std::shared_ptr<Document> ptr_to_document);

  // Maps given raw code point to Unicode
  Unicode Interpret(const RawChar& code_point_to_be_interpreted);

  // This typedef shortens the name of the RawChar to Unicode lookup maps.
  typedef std::unordered_map<RawChar, Unicode> UnicodeMap;

  // Gets all available Raw chars that may be translated to Unicode in the map
  std::shared_ptr<UnicodeMap> GetEncodingKeys();

 private:
  // States used by parser to read "differences" entry in encoding dictionary
  enum DifferencesState { NEWSYMB, NUM, NAME, STOP };

  // Data lookup tables - defined as static, which means only a single
  // instance of each is created rather than a copy for each object.
  // Note these maps are defined in adobetounicode.h and chartounicode.h
  static const std::unordered_map<std::string, Unicode> adobe_to_unicode_;
  static const UnicodeMap macroman_to_unicode_;
  static const UnicodeMap winansi_to_unicode_;
  static const UnicodeMap pdfdoc_to_unicode_;

  UnicodeMap encoding_map_;             // The main data member lookup
  Dictionary& font_dictionary_;         // the main font dictionary
  std::shared_ptr<Document> document_;  // pointer to the containing document
  std::string base_encoding_;           // value of /BaseEncoding entry

  // The entries_ vector gives a pair of type : entry for each entity pushed
  // onto the stack by the lexer. We therefore know whether we are dealing with
  // a code point or a name when we parse the stack
  std::vector<std::pair<DifferencesState, std::string>> entries_;

  // private member functions

  // uses lexer to parse /Differences entry
  void ReadDifferences_(const std::string&);

  // finds encoding dictionary, gets /basencoding and /Differences entries
  void ReadEncoding_();           // Tokenizer
  void ReadDifferenceEntries_();  // Parser

  // parses CMap encoding ranges
  void ProcessUnicodeRange_(std::vector<std::string>&);

  // parses CMap direct char-char conversion table
  void ProcessUnicodeChars_(std::vector<std::string>&);

  // finds CMap if any and co-ordinates parsers to create mapping
  void MapUnicode_();

  // Handles type 1 fonts
  void HandleTypeOneFont_();
  void ParseTypeOneFont_(std::string);

  // Helper function for parser
  void Write_(DifferencesState& state_to_push_to_entries,
              std::string& string_to_push_to_entries);
};

//---------------------------------------------------------------------------//

#endif
