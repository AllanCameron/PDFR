//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Font implementation file                                            //
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

#include "font.h"

//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// The Font constructor simply initializes the private data members, calls
// getFontName() to get the postscript font title, and then makeGlyphTable()
// to create the main data member

Font::Font(shared_ptr<Document> doc, Dictionary Fontref, const string& fontid) :
m_d(doc), m_fontref(Fontref), m_FontID(fontid)
{
  getFontName();
  makeGlyphTable();
}

/*---------------------------------------------------------------------------*/
// Obtains the font's PostScript name from the font dictionary

void Font::getFontName()
{
  string BaseFont(m_fontref.GetString("/BaseFont")); // reads BaseFont entry

  if(BaseFont.size() > 7 && BaseFont[7] == '+')
  {
    m_FontName = BaseFont.substr(8, BaseFont.size() - 8);
  }
  else
  {
    m_FontName = BaseFont.substr(1, BaseFont.size() - 1);
  }
}

/*---------------------------------------------------------------------------*/
// Most of the work asked of an object of the Font class will be to provide
// interpretations of raw character codes, in terms of the actual glyphs and
// their sizes intended by the document. This public method allows a vector
// of raw characters to be interpreted. It returns a vector of the same length
// as the input vector, containing a pair of {Unicode glyph, width} at each
// position

vector<pair<Unicode, int>> Font::mapRawChar(const vector<RawChar>& raw)
{
  vector<pair<Unicode, int>> res;
  res.reserve(raw.size());
  for(const auto& i : raw)
  {
    auto g = m_glyphmap.find(i);
    if(g != m_glyphmap.end())
    {
      res.push_back(g->second);
    }
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// The Font class subcontracts most of the work of its own construction out to
// the encoding and glyphwidth classes. This private method co-ordinates the
// building of the glyphmap using these two component classes

void Font::makeGlyphTable()
{
  // Create Encoding object
  Encoding encodings(m_fontref, m_d);

  // Create glyphwidth object
  glyphwidths widths(m_fontref, m_d);

  // get all the mapped RawChars from the Encoding object
  auto encoding_map = encodings.encoding_keys();

  // We need to know whether the width code points refer to the width of raw
  // character codes or to the final Unicode translations

  // If the widths refer to RawChar code points, map every RawChar to a width
  if(widths.widthsAreForRaw())
  {
    for(auto& key_value_pair : *encoding_map)
    {
      auto& key = key_value_pair.first;
      m_glyphmap[key] = make_pair(encodings.interpret(key),
                                  widths.getwidth(key));
    }
  }
  // Otherwise widths refer to Unicode glyphs, so map each to a width
  else
  {
    for(auto& key_value_pair : *encoding_map)
    {
      auto& key = key_value_pair.first;
      m_glyphmap[key] = make_pair(encodings.interpret(key),
                                  widths.getwidth(encodings.interpret(key)));
    }
  }
}

/*---------------------------------------------------------------------------*/
// Public getter for FontName

std::string Font::fontname()
{
  return m_FontName;
}

/*---------------------------------------------------------------------------*/
// Public getter for the keys of the glyphmap, needed to output the map from
// the program if required

std::vector<RawChar> Font::getGlyphKeys()
{
  return getKeys(m_glyphmap);
}
