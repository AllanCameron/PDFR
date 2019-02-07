//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR font header file                                                    //
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
 * The job of the font class is therefore to co-ordinate the process of font
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
#include "encoding.h"
#include "glyphwidths.h"

//---------------------------------------------------------------------------//
// The GlyphMap is the main data member of the font class. Although it is
// constructed from standard library components, it needs a shorthand name

typedef std::unordered_map<RawChar, std::pair<Unicode, int>> GlyphMap;

//---------------------------------------------------------------------------//
// Each font object is created and stored as an object in a pdf page, as this
// is how the pdf is logically organised. However, its public methods are
// called by other classes, which use font objects to interpret pdf strings.

class font
{

public:

  // constructors

  font(std::shared_ptr<document>, dictionary, const std::string&);
  font(){};

  // public methods

  std::string fontname();               // get the actual postscript font name
  std::vector<RawChar> getGlyphKeys();  // vector of all mapped RawChars

  // The most important public method is mapRawChar, which takes a vector of
  // uint16_t representing raw character codes, and returns a vector of pairs
  // containing the Unicode interpretation and its associated width
  std::vector<std::pair<Unicode, int>> mapRawChar(std::vector<RawChar>);


private:

  // private data members

  std::shared_ptr<document> d;  // pointer to the containing document
  dictionary fontref;   // the font dictionary
  std::string FontID,   // The name the font is given in the PDF
              FontName; // The actual name of the font
  GlyphMap glyphmap;    // Main data member, mapping RawChar to {Unicode, width}

  // private methods

  void getFontName();   // Finds the postscript font name

  // Main creator function; makes glyphmap from encoding and glyphwidth classes
  void makeGlyphTable();

};

//---------------------------------------------------------------------------//

#endif
