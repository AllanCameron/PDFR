//---------------------------------------------------------------------------//
//                                                                           //
// PDFR glyphwidth implementation file                                       //
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

#include "glyphwidths.h"

//---------------------------------------------------------------------------//
// Where a glyph's width in missing and there is no default width, we need a
// "default" default width. Set as a macro for easy changing and to avoid
// a "magic constant" in the program.

#define DEFAULT_WIDTH 500;

//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// Constructor taking a reference to the main font dictionary and a pointer to
// the containing document. If the font is a core font, get the widths from
// built in static corefont tables. Otherwise find and interpret widths.

glyphwidths::glyphwidths(dictionary& dic, shared_ptr<document> doc):
  m_font_dictionary(dic), m_document(doc)
{
  m_base_font = m_font_dictionary.get_string("/BaseFont");
  getCoreFont();
  if(m_width_map.empty()) getWidthTable();
}

/*---------------------------------------------------------------------------*/
// The two main ways to get glyph widths from a font's dictionary are directly
// under the /Widths entry, or under the /DescendantFonts dictionary. This
// method calls the appropriate parser depending on the entries in the font
// dictionary

void glyphwidths::getWidthTable()
{
  // If widths entry specified, use this by calling parsewidths method
  if (m_font_dictionary.has_key("/Widths")) parseWidths();

  // otherwise look in descendants using parseDescendants method
  else if(m_font_dictionary.contains_references("/DescendantFonts"))
    parseDescendants();

  // otherwise we have no font widths specified and need to use default values
}

/*---------------------------------------------------------------------------*/
// This method is only called when a /Widths entry is found in the font
// dictionary. It looks for a /FirstChar entry which specifies the code point
// to which the first width in the array applies. The rest of the array then
// refers to sequential code points after this.

void glyphwidths::parseWidths()
{
  // Usually widths given as ints, but can be floats
  vector<float> widtharray;

  // If the font dictionary contains no /Firstchar, we'll default to zero
  RawChar firstchr = 0x0000;

  // Otherwise we read the firstchar entry
  if(m_font_dictionary.contains_ints("/FirstChar"))
  {
    firstchr = m_font_dictionary.get_ints("/FirstChar")[0];
  }
  // Annoyingly, widths sometimes contains a pointer to another object that
  // contains the width array, either in a stream or as a 'naked object'.
  // Note that contents of a naked object are stored as the object's 'stream'.

  // Handle /widths being a reference to another object
  if (m_font_dictionary.contains_references("/Widths"))
  {
    // Get the referenced object
    auto width_object =
      m_document->get_object(m_font_dictionary.get_reference("/Widths"));

    // Get the referenced object's stream
    string ostring(width_object->get_stream());

    // Get the numbers from the width array in the stream
    widtharray = parse_floats(ostring);
  }
  // If /Widths is not a reference get the widths directly
  else  widtharray = m_font_dictionary.get_floats("/Widths");

  // If a width array was found
  if (!widtharray.empty())
  {
    // The widths represent post-Unicode translation widths
    this->widthFromCharCodes = true;

    // Now we can fill the width map from the width array
    for (size_t i = 0; i < widtharray.size(); ++i)
      m_width_map[firstchr + i] = (int) widtharray[i];
  }
}

/*---------------------------------------------------------------------------*/
// If the font is is CIDKeyed (type 0) font, it will inherit from a descendant
// font with its own object dictionary. This should contain a /W entry that is
// an array of widths for given ranges of code points and needs to be
// interpreted by its own lexer, also included as a method in this class

void glyphwidths::parseDescendants()
{
  // get a pointer to the descendantfonts object
  auto desc =
    m_document->get_object(m_font_dictionary.get_reference("/DescendantFonts"));

  // Extract its dictionary and its stream
  dictionary descdict = desc->get_dictionary();
  string descstream(desc->get_stream());

  // Handle descendantfonts being just a reference to another object
  if(!parse_references(descstream).empty())
  {
    descdict =
      m_document->get_object(parse_references(descstream)[0])->get_dictionary();
  }

  // We now look for the /W key and if it is found parse its contents
  if (descdict.has_key("/W"))
  {
    // We will fill this string with width array when found
    string widthstring;

    // sometimes the /W entry only contains a pointer to the containing object
    if (descdict.contains_references("/W"))
    {
      widthstring =
        m_document->get_object(descdict.get_reference("/W"))->get_stream();
    }

    // otherwise we assume /W contains the widths needed
    else widthstring = descdict.get_string("/W");

    // in either case widthstring should now contain the /W array which we
    // now need to parse using our lexer method
    parsewidtharray(widthstring);

    // The widths obtained apply to the RawChars, not to post-conversion Unicode
    this->widthFromCharCodes = true;
  }
}

/*---------------------------------------------------------------------------*/
// The creator function includes a string passed from the "BaseFont" entry
// of the encoding dictionary.

void glyphwidths::getCoreFont()
{
  // If the font is one of the 14 core fonts, the widths are already specified.
  // Note that these widths represent the widths of the actual Unicode glyphs
  // so any encoding differences should take place before the widths are
  // interpreted. This is not the case where /Differences or a specific
  // /Width map is included - in these cases the widths refer to the glyphs
  // that will result from the given RawChar codes. This is therefore flagged
  // by the boolean widthsFromCharCodes.

  if (m_base_font.find("/Courier") != string::npos)
    m_width_map = courier_widths;
  else if(m_base_font == "/Helvetica")
    m_width_map = helvetica_widths;
  else if(m_base_font == "/Helvetica-Oblique")
    m_width_map = helvetica_widths;
  else if(m_base_font == "/Helvetica-Bold")
    m_width_map = helvetica_bold_widths;
  else if(m_base_font == "/Helvetica-Boldoblique")
    m_width_map = helvetica_bold_widths;
  else if(m_base_font == "/Symbol")
    m_width_map = symbol_widths;
  else if(m_base_font == "/Times-Bold")
    m_width_map = times_bold_widths;
  else if(m_base_font == "/Times-BoldItalic")
    m_width_map = times_bold_italic_widths;
  else if(m_base_font == "/Times-Italic")
    m_width_map = times_italic_widths;
  else if(m_base_font == "/Times-Roman")
    m_width_map = times_roman_widths;
  else if(m_base_font == "/ZapfDingbats")
    m_width_map = dingbats_widths;

  // No unicode -> width mapping - using RawChar
  else widthFromCharCodes = true;
}

/*---------------------------------------------------------------------------*/
// Getter. Finds the width for a given character code. If it is not specified
// returns the default width specified in the macro at the top of this file

int glyphwidths::getwidth(const RawChar& raw)
{
  // Look up the supplied rawChar
  auto found = m_width_map.find(raw);
  if(found != m_width_map.end()) return found->second;

  // No width found - return the default width
  else return DEFAULT_WIDTH;
}

/*---------------------------------------------------------------------------*/
// Simple public getter function that returns the mapped character codes as
// a vector without their associated widths

vector<RawChar> glyphwidths::widthKeys()
{
  return getKeys(this->m_width_map);
}

/*---------------------------------------------------------------------------*/
// Yet another lexer. This one is specialised to read the "/W" entry of type0
// fonts. These are an array containing arrays of widths. Each sub-array
// is preceeded by the code point to which the first width in the sub-array
// applies, after which the widths apply to consecutive values after the first
// code point. Hence the string "[3[100 200 150] 10[250 300]]" should be
// interpreted as mapping {{3, 100}, {4, 200}, {5, 150}, {10, 250}, {11, 300}}

void glyphwidths::parsewidtharray(const string& s)
{
  if(s.empty()) return;           // empty string == nothing to do
  enum Wstate {NEWSYMB, INARRAY, INSUBARRAY, END}; // possible states of lexer
  string buf = "";                                 // empty buffer for lexer
  Wstate state = NEWSYMB;                          // Initial state of lexer
  vector<int> vecbuf, resultint;                   // temporary variables
  vector<vector<int>> resultvec;  // container of results to write to width map

  // main loop - straight iteration through all the characters in s
  for(const auto& i : s)
  {
    char a = symbol_type(i); // determine symbol type from method in utilities.h

    // If opening of array not first character, simply wait for '['
    if(state == NEWSYMB)
    {
      if(a == '[') state = INARRAY;
      continue;
    }
    // In the main array. Either read a code character or find a subarray
    if(state == INARRAY)
    {
      switch(a)
      {
      case 'D' : buf += i; break; // read the number
      case '[' : state = INSUBARRAY;  // Switch to subarray state
                 if(!buf.empty())                     // if something in buffer
                 {
                   vecbuf.push_back(stoi(buf));       // convert and store it
                   if(vecbuf.size() == 1)
                     resultint.push_back(vecbuf[0]); // char code if single
                   else
                     resultvec.push_back(vecbuf); // width values if multiple
                 }                                // numbers have been stored
                 buf.clear();
                 vecbuf.clear();                // in either case clear buffers
                 break;
      case ' ' : if(!buf.empty())               // store number in int buffer
                   vecbuf.push_back(stoi(buf));
                buf.clear(); break;             // clear the string buffer
      case ']': state = END;                    // end of main array
                if(!buf.empty())                // Use what's in the buffers
                {                               // as specified above
                  vecbuf.push_back(stoi(buf));
                  if(vecbuf.size() == 1)
                    resultint.push_back(vecbuf[0]);
                  else resultvec.push_back(vecbuf);
                }
                buf.clear();
                vecbuf.clear();                   // clear the buffers
                break;
      default: throw (string("Error parsing string ") + s); // unspecified
      }
      continue;
    }
    // handle the insubarray state
    if(state == INSUBARRAY)
    {
      switch(a)
      {
      case ' ': if(!buf.empty()) vecbuf.push_back(stoi(buf)); // push to int buf
                buf.clear();
                break;
      case ']': state = INARRAY;  // exited from subarray
                if(!buf.empty()) vecbuf.push_back(stoi(buf)); // use buf content
                resultvec.push_back(vecbuf);
                vecbuf.clear();
                buf.clear(); break;
      case 'D': buf += i; break; // read actual width number
      default: throw (string("Error parsing string ") + s);
      }
      continue;
    }
    if(state == END) break;
  }

  // We now parse the results of the lexing procedure.
  // First we make sure that the starting character codes are equal in length
  // to the number of width arrays, and that neither is empty
  if((resultint.size() == resultvec.size()) && !resultint.empty() )
  {
    // now loop through the vectors and marry char codes to widths
    for(size_t i = 0; i < resultint.size(); ++i)
    {
      // Skip any character code that doesn't have an associated width array
      if(!resultvec[i].empty())
      {
        // Now for each member of the width array...
        for(size_t j = 0; j < resultvec[i].size(); j++)
        {
          // map sequential values of char codes to stated widths
          m_width_map[(RawChar) resultint[i] + j] = (int) resultvec[i][j];
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// The font class needs to know whether to build the glyphmap based on RawChar
// code points or Unicode code points. If the following returns true, the map
// should be built using the raw character values

bool glyphwidths::widthsAreForRaw()
{
  return widthFromCharCodes;
}
