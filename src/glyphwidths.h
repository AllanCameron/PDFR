//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GlyphWidths header file                                             //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_WIDTH

//---------------------------------------------------------------------------//

#define PDFR_WIDTH

/* This is the joint 6th in a series of daisy-chained headers that build up the
 * tools to read and parse pdfs. It is logically paired with encoding.h
 * in that they both come after document.h and together form the basis for the
 * next step, which is font creation.
 *
 * Calculating the width of each glyph is necessary for working out the spacing
 * between letters, words, paragraphs and other text elements. The glyph widths
 * in pdf are given in units of text space, where 1000 = 1 point = 1/72 inch in
 * 1-point font size.
 *
 * Getting the glyph widths is one of the more complex tasks in extracting text,
 * since there are various ways for pdf files to describe them. The most
 * explicit way is by listing the font widths at each code point in an array.
 * The array is preceeded by the first code point that is being described,
 * then the array itself comprises numbers for the widths of sequential code
 * points. Often there are several consecutive arrays like this specifying
 * groups of sequential code points. Sometimes the entry is just an array of
 * widths, and the first code point is given seperately in the font
 * dictionary. Sometimes there is a default width for missing glyphs. Sometimes
 * the width array is in the font dictionary; sometimes it is in a descendant
 * font dictionary; other times it is in an encoded stream; still other times
 * it comprises an entire non-dictionary object on its own.
 *
 * In older pdfs, the widths may not be specified at all if the font used is
 * one of 14 core fonts in the pdf specification. A conforming reader is
 * supposed to know the glyph widths for these fonts.
 *
 * The glyphwidth class attempts to work out the method used to describe
 * glyph widths and produce a map of the intended glyphs to their intended
 * widths, without bothering any other classes with its implementation.
 *
 * Among the tools it needs to do this, it requires navigating the document,
 * reading dictionaries and streams, and parsing a width description array.
 * It therefore needs the document.h header which wraps most of these
 * capabilities. The class defines its own lexer for interpreting the special
 * width arrays.
 *
 * It also needs a group of static objects listing the widths of each of the
 * characters used in the 'built-in' fonts used in pdfs. In theory, later
 * versions of pdf require specification of all glyph widths, but for back-
 * compatibility, the widths of the 14 core fonts still need to be defined.
 *
 * The widths are available as an open online resource from Adobe.
 *
 * To preserve encapsulation, this header is included only by the fonts
 * class. The fonts class merges its width map with the encoding map to
 * produce the glyphmap, which gives the intended Unicode code point and
 * width as a paired value for any given input character in a pdf string.
 */

//---------------------------------------------------------------------------//

#include<string>
#include<vector>
#include<unordered_map>
#include<memory>

class Dictionary;
class Document;
using Unicode = uint16_t;
using RawChar = uint16_t;


//---------------------------------------------------------------------------//
// The GlyphWidths class contains private methods to find the description of
// widths for each character in a font. It only makes sense to the font class,
// from whence it is created and accessed.
//
// The core font widths are declared static private because they are only
// needed by this class, and we don't want an extra copy of all of them if
// several fonts are created. This also prevents them polluting the global
// namespace.

class GlyphWidths
{
 public:
  // Constructor
  GlyphWidths(Dictionary& font_dictionary_ptr,
              std::shared_ptr<Document> document_ptr);

  // public methods
  float GetWidth(const RawChar& code_point);   // Get width of character code
  std::vector<RawChar> WidthKeys();            // Returns all map keys

  inline bool WidthsAreForRaw() const { return width_is_pre_interpretation_; }

 private:
  // This enum is used in the width array lexer
  enum WidthState {NEWSYMB, READFIRSTCHAR, READSECONDCHAR,
                   READWIDTH, INSUBARRAY, END};

  // private data
  std::unordered_map<RawChar, float> width_map_;  // The main data member
  Dictionary& font_dictionary_;                 // The font dictionary
  std::shared_ptr<Document> document_;          // Pointer to document
  std::string base_font_;                       // The base font (if any)
  bool width_is_pre_interpretation_;            // Are widths for code points
                                                // pre- or post- translation?
  // private methods
  void ParseWidthArray_(const std::string&);    // Width lexer
  void ReadCoreFont_();                         // Core font getter
  void ParseDescendants_();                     // Gets descendant dictionary
  void ParseWidths_();                          // Parses the width array
  void ReadWidthTable_();                       // Co-ordinates construction

//-- The core fonts as defined in corefonts.cpp ------------------------------//
                                                                              //
  static const std::unordered_map<Unicode, float> courier_widths_;              //
  static const std::unordered_map<Unicode, float> helvetica_widths_;            //
  static const std::unordered_map<Unicode, float> helvetica_bold_widths_;       //
  static const std::unordered_map<Unicode, float> symbol_widths_;               //
  static const std::unordered_map<Unicode, float> times_bold_widths_;           //
  static const std::unordered_map<Unicode, float> times_bold_italic_widths_;    //
  static const std::unordered_map<Unicode, float> times_italic_widths_;         //
  static const std::unordered_map<Unicode, float> times_roman_widths_;          //
  static const std::unordered_map<Unicode, float> dingbats_widths_;             //
                                                                              //
//----------------------------------------------------------------------------//
};

//---------------------------------------------------------------------------//

#endif
