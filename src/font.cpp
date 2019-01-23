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

font::font(document* doc, dictionary Fontref, const string& fontid) :
d(doc), fontref(Fontref), FontID(fontid)
{
  getFontName();
  makeGlyphTable();
}

/*---------------------------------------------------------------------------*/
// Obtains the font's PostScript name from the font dictionary

void font::getFontName()
{
  string BaseFont = fontref.get("/BaseFont"); // reads BaseFont entry
  size_t BaseFont_start = 1;  //-----//
  if(BaseFont.size() > 7)            // The basefont name sometimes has a suffix
    if(BaseFont[7] == '+')           // following a '+' at position 7. This
      BaseFont_start = 8;     //-----// conditional expression removes it

  // Store to private data member
  FontName = BaseFont.substr(BaseFont_start, BaseFont.size() - BaseFont_start);
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
  for(auto i : raw)
    if(glyphmap.find(i) != glyphmap.end()) // safe range checker
      res.push_back(glyphmap[i]);          // push found result
  return res;
}

/*---------------------------------------------------------------------------*/
// The font class subcontracts most of the work of its own construction out to
// the encoding and glyphwidth classes. This private method co-ordinates the
// building of the glyphmap using these two component classes

void font::makeGlyphTable()
{
  // Create Encoding object
  Encoding Enc = Encoding(this->fontref, this->d);

  // Create glyphwidth object
  glyphwidths Wid = glyphwidths(this->fontref, this->d);

  // get all the mapped RawChars from the Encoding object
  vector<RawChar> inkeys = Enc.encKeys();

  // We need to know whether the width code points refer to the width of raw
  // character codes or to the final Unicode translations

  if(Wid.widthsAreForRaw()) // The widths refer to RawChar code points
    for(auto i : inkeys) // map every mapped RawChar to a width
      glyphmap[i] = make_pair(Enc.Interpret(i), Wid.getwidth(i));

  else // The widths refer to Unicode glyphs
    for(auto i : inkeys) // map every mapped Unicode to a width
      glyphmap[i] = make_pair(Enc.Interpret(i), Wid.getwidth(Enc.Interpret(i)));
}

/*---------------------------------------------------------------------------*/
// Public getter for FontName

std::string font::fontname()
{
  return this->FontName;
}

/*---------------------------------------------------------------------------*/
// Public getter for the keys of the glyphmap, needed to output the map from
// the program if required

std::vector<RawChar> font::getGlyphKeys()
{
  return getKeys(this->glyphmap); // Uses getKeys() from utilities.h
}
