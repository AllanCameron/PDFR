//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Font header file                                                    //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_FONT

//---------------------------------------------------------------------------//

#define PDFR_FONT

/* This is the seventh step of a daisy-chain of headers comprising the PDFR
 * program as described in headerMap.txt. It #includes the two files that
 * comprise the 6th step - encoding.h and glyphwidths.h.
 *
 * Most of the hard work in creating fonts has been done in the previous step -
 * working out which glyphs are intended by an input pdf string, and what size
 * those glyphs should be when printed.
 *
 * The job of the Font class is therefore to co-ordinate the process of font
 * creation by using these other two classes, then combining their results into
 * a structure that I have called a glyphmap. This is a map which maps any
 * raw character from the pdf input to a pair indicating the intended Unicode
 * output glyph and glyph width for that character code.
 *
 * Its public interface includes constructors which require a document pointer,
 * the font dictionary and the ID of the font, used as shorthand in the
 * page dictionary.
 *
 * The remainder of the public members are: a getter for the actual font name;
 * an enumerator of all the RawChars mapped in the glyphmap, and a function to
 * safely interrogate the glyphmap, returning a vector of paired Unicode code
 * points and integer widths for each glyph, given an input vector of RawChars.
 *
 */

#include<utility>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>

class Dictionary;
class Document;
using Unicode = uint16_t;
using RawChar = uint16_t;


//---------------------------------------------------------------------------//
// The GlyphMap is the main data member of the Font class. Although it is
// constructed from standard library components, it needs a shorthand name

typedef std::pair<Unicode, float> GlyphData;
typedef std::unordered_map<RawChar, GlyphData> GlyphMap;

//---------------------------------------------------------------------------//
// Each Font object is created and stored as an object in a pdf page, as this
// is how the pdf is logically organised. However, its public methods are
// called by other classes, which use Font objects to interpret pdf strings.

class Font
{
 public:
  // Constructor
  Font(std::shared_ptr<Document> document_ptr,
       Dictionary& font_dictionary_ptr,
       const std::string& id);

  // public methods
  std::string GetFontName();            // Gets the actual PostScript Font name
  std::vector<RawChar> GetGlyphKeys();  // Returns vector of all mapped RawChars

  // The most important public method is MapRawChar, which takes a vector of
  // uint16_t representing raw character codes, and returns a vector of pairs
  // containing the Unicode interpretation and its associated width
  std::vector<GlyphData> MapRawChar(const std::vector<RawChar>& raw_chars);

private:
  // private data members
  std::shared_ptr<Document> document_;  // - Pointer to the containing document
  Dictionary& font_dictionary_;         // - The main font dictionary
  std::string font_id_,                 // - The name the font as used in PDF
              font_name_;               // - The actual name of the font
  GlyphMap glyph_map_;                  // - Main data member, mapping RawChar
                                        //   to a {Unicode, width} pair.

  // private methods
  void ReadFontName_();                  // Finds the postscript font name
  void MakeGlyphTable_();                // Co-ordinates font construction
};

//---------------------------------------------------------------------------//

#endif
