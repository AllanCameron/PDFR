//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Font implementation file                                            //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "dictionary.h"
#include "glyphwidths.h"
#include "encoding.h"
#include "font.h"

//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// The Font constructor simply initializes the private data members, calls
// getFontName() to get the postscript font title, and then makeGlyphTable()
// to create the main data member

Font::Font(shared_ptr<Document> document_ptr,
           Dictionary& font_dictionary,
           const string& font_id)
  : document_(document_ptr),
    font_dictionary_(font_dictionary),
    font_id_(font_id)
{
  ReadFontName_();
  MakeGlyphTable_();
}

/*---------------------------------------------------------------------------*/
// Obtains the font's PostScript name from the font dictionary

void Font::ReadFontName_()
{
  // Reads /BaseFont entry
  string base_font(font_dictionary_.GetString("/BaseFont"));

  if (base_font.size() > 7 && base_font[7] == '+')
  {
    font_name_ = base_font.substr(8, base_font.size() - 8);
  }
  else
  {
    font_name_ = base_font.substr(1, base_font.size() - 1);
  }
}

/*---------------------------------------------------------------------------*/
// Most of the work asked of an object of the Font class will be to provide
// interpretations of raw character codes, in terms of the actual glyphs and
// their sizes intended by the document. This public method allows a vector
// of raw characters to be interpreted. It returns a vector of the same length
// as the input vector, containing a pair of {Unicode glyph, width} at each
// position

vector<pair<Unicode, int>> Font::MapRawChar(const vector<RawChar>& raw_vector)
{
  vector<pair<Unicode, int>> result;
  result.reserve(raw_vector.size());

  for (const auto& raw_char : raw_vector)
  {
    auto finder = glyph_map_.find(raw_char);
    if (finder != glyph_map_.end())
    {
      result.push_back(finder->second);
    }
  }

  return result;
}

/*---------------------------------------------------------------------------*/
// The Font class subcontracts most of the work of its own construction out to
// the encoding and glyphwidth classes. This private method co-ordinates the
// building of the glyphmap using these two component classes

void Font::MakeGlyphTable_()
{
  // Create Encoding object
  Encoding encodings(font_dictionary_, document_);

  // Create glyphwidth object
  GlyphWidths widths(font_dictionary_, document_);

  // get all the mapped RawChars from the Encoding object
  auto encoding_map = encodings.GetEncodingKeys();

  // We need to know whether the width code points refer to the width of raw
  // character codes or to the final Unicode translations

  // If the widths refer to RawChar code points, map every RawChar to a width
  if (widths.WidthsAreForRaw())
  {
    for (auto& key_value_pair : *encoding_map)
    {
      auto& key = key_value_pair.first;
      glyph_map_[key] = make_pair(encodings.Interpret(key),
                                  widths.GetWidth(key));
    }
  }
  // Otherwise widths refer to Unicode glyphs, so map each to a width
  else
  {
    for (auto& key_value_pair : *encoding_map)
    {
      auto& key = key_value_pair.first;
      glyph_map_[key] = make_pair(encodings.Interpret(key),
                                  widths.GetWidth(encodings.Interpret(key)));
    }
  }
}

/*---------------------------------------------------------------------------*/
// Public getter for FontName

std::string Font::GetFontName()
{
  return font_name_;
}

/*---------------------------------------------------------------------------*/
// Public getter for the keys of the glyphmap, needed to output the map from
// the program if required

std::vector<RawChar> Font::GetGlyphKeys()
{
  return GetKeys(glyph_map_);
}
