//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR font implementation file                                            //
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
// The font constructor simply initializes the private data members, calls
// getFontName() to get the postscript font title, and then makeGlyphTable()
// to create the main data member

font::font(shared_ptr<document> doc, dictionary Fontref, const string& fontid) :
m_d(doc), m_fontref(Fontref), m_FontID(fontid)
{
  getFontName();
  makeGlyphTable();
}

/*---------------------------------------------------------------------------*/
// Obtains the font's PostScript name from the font dictionary

void font::getFontName()
{
  string BaseFont(m_fontref.get("/BaseFont")); // reads BaseFont entry

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
// Most of the work asked of an object of the font class will be to provide
// interpretations of raw character codes, in terms of the actual glyphs and
// their sizes intended by the document. This public method allows a vector
// of raw characters to be interpreted. It returns a vector of the same length
// as the input vector, containing a pair of {Unicode glyph, width} at each
// position

vector<pair<Unicode, int>> font::mapRawChar(vector<RawChar> raw)
{
  vector<pair<Unicode, int>> res; // container for results
  res.reserve(raw.size());
  for(auto i : raw)
  {
    auto g = m_glyphmap.find(i);
    if(g != m_glyphmap.end()) // safe range checker
    {
      res.push_back(g->second);          // push found result
    }
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// The font class subcontracts most of the work of its own construction out to
// the encoding and glyphwidth classes. This private method co-ordinates the
// building of the glyphmap using these two component classes

void font::makeGlyphTable()
{
  // Create Encoding object
  Encoding Enc(m_fontref, m_d);
  // Create glyphwidth object
  glyphwidths Wid(m_fontref, m_d);
  // get all the mapped RawChars from the Encoding object
  std::shared_ptr<std::unordered_map<RawChar, Unicode>> inkeys = Enc.encKeys();

  // We need to know whether the width code points refer to the width of raw
  // character codes or to the final Unicode translations
  if(Wid.widthsAreForRaw()) // The widths refer to RawChar code points
    for(auto& i : *inkeys) // map every mapped RawChar to a width
      m_glyphmap[i.first] = make_pair(Enc.Interpret(i.first),
                                    Wid.getwidth(i.first));

  else // The widths refer to Unicode glyphs
    for(auto& i : *inkeys) // map every mapped Unicode to a width
      m_glyphmap[i.first] = make_pair(Enc.Interpret(i.first),
                                    Wid.getwidth(Enc.Interpret(i.first)));
}

/*---------------------------------------------------------------------------*/
// Public getter for FontName

std::string font::fontname()
{
  return m_FontName;
}

/*---------------------------------------------------------------------------*/
// Public getter for the keys of the glyphmap, needed to output the map from
// the program if required

std::vector<RawChar> font::getGlyphKeys()
{
  return getKeys(m_glyphmap); // Uses getKeys() from utilities.h
}
