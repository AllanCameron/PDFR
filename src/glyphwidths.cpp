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
  fontref(dic), d(doc)
{
  basefont = fontref.get("/BaseFont");
  getCoreFont();
  if(Width.empty()) getWidthTable();
}

/*---------------------------------------------------------------------------*/
// The two main ways to get glyph widths from a font's dictionary are directly
// under the /Widths entry, or under the /DescendantFonts dictionary. This
// method calls the appropriate parser depending on the entries in the font
// dictionary

void glyphwidths::getWidthTable()
{
  // If widths entry specified, use this by calling parsewidths method
  if (fontref.has("/Widths")) parseWidths();
  // otherwise look in descendants using parseDescendants method
  else if(fontref.hasRefs("/DescendantFonts")) parseDescendants();
  // otherwise we have no font widths specified and need to use default values
}

/*---------------------------------------------------------------------------*/
// This method is only called when a /Widths entry is found in the font
// dictionary. It looks for a /FirstChar entry which specifies the code point
// to which the first width in the array applies. The rest of the array then
// refers to sequential code points after this.

void glyphwidths::parseWidths()
{
  vector<float> widtharray; // usually widths given as ints, but can be floats
  RawChar firstchar = 0x0000; // if no firstchar, default to zero
  if(fontref.hasInts("/FirstChar"))
    firstchar = fontref.getInts("/FirstChar")[0]; // get first char

  // Annoyingly, widths sometimes contains a pointer to another object that
  // contains the width array, either in a stream or as a 'naked object'.
  // Note that contents of a naked object are stored as the object's 'stream'.

  if (fontref.hasRefs("/Widths")) // test if widths is a reference
  {
    object_class* o = d->getobject(fontref.getRefs("/Widths").at(0)); // get ref
    string ostring = o->getStream(); // get stream from ref
    widtharray = getnums(ostring);   // get numbers from stream
  }
  else  // not a reference - get widths directly
    widtharray = fontref.getNums("/Widths");

  if (!widtharray.empty())
  {
    this->widthFromCharCodes = true; // widths are given pre-Unicode translation
    for (size_t i = 0; i < widtharray.size(); i++)
      Width[firstchar + i] = (int) widtharray[i]; // fill width map from array
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
  object_class* desc = d->getobject(fontref.getRefs("/DescendantFonts")[0]);
  dictionary descdict = desc->getDict(); // extract its dictionary...
  string descstream = desc->getStream(); // ...and its stream
  // sometimes the descendantfonts object is just a reference to another object
  if(!getObjRefs(descstream).empty())
    descdict = d->getobject(getObjRefs(descstream)[0])->getDict(); // so use it

  if (descdict.has("/W"))
  {
    string widthstring; // we will fill this string with width array when found

    // sometimes the /W entry only contains a pointer to the containing object
    if (descdict.hasRefs("/W")) // so we use that
      widthstring = d->getobject(descdict.getRefs("/W").at(0))->getStream();
    else // otherwise we assume /W contains the widths needed
      widthstring = descdict.get("/W");

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

       if(basefont == "/Courier") Width = courierwidths;
  else if(basefont == "/Courier-Bold") Width = courierboldwidths;
  else if(basefont == "/Courier-BoldOblique") Width = courierboldobliquewidths;
  else if(basefont == "/Courier-Oblique") Width = courierobliquewidths;
  else if(basefont == "/Helvetica") Width = helveticawidths;
  else if(basefont == "/Helvetica-Bold") Width = helveticaboldwidths;
  else if(basefont == "/Helvetica-Boldoblique") Width = helveticabold_ob_widths;
  else if(basefont == "/Helvetica-Oblique") Width = helveticaobliquewidths;
  else if(basefont == "/Symbol") Width = symbolwidths;
  else if(basefont == "/Times-Bold") Width = timesboldwidths;
  else if(basefont == "/Times-BoldItalic") Width = timesbolditalicwidths;
  else if(basefont == "/Times-Italic") Width =timesitalicwidths;
  else if(basefont == "/Times-Roman") Width = timesromanwidths;
  else if(basefont == "/ZapfDingbats") Width = dingbatswidths;
  else widthFromCharCodes = true; // No unicode -> width mapping - using RawChar
}

/*---------------------------------------------------------------------------*/
// Getter. Finds the width for a given character code. If it is not specified
// returns the default width specified in the macro at the top of this file

int glyphwidths::getwidth(RawChar raw)
{
  if(Width.find(raw) != Width.end()) // safe finder - doesn't insert empty key
    return Width[raw];               // return mapped value
  else
    return DEFAULT_WIDTH;            // else default width
}

/*---------------------------------------------------------------------------*/
// Simple public getter function that returns the mapped character codes as
// a vector without their associated widths

vector<RawChar> glyphwidths::widthKeys()
{
  return getKeys(this->Width); // getKeys template function is from utilities.h
}

/*---------------------------------------------------------------------------*/
// Yet another lexer. This one is specialised to read the "/W" entry of type0
// fonts. These are an array containing arrays of widths. Each sub-array
// is preceeded by the code point to which the first width in the sub-array
// applies, after which the widths apply to consecutive values after the first
// code point. Hence the string "[3[100 200 150] 10[250 300]]" should be
// interpreted as mapping {{3, 100}, {4, 200}, {5, 150}, {10, 250}, {11, 300}}

void glyphwidths::parsewidtharray(string s)
{
  s += " "; // we add an extra whitespace onto the end to ensure that the buffer
            // is emptied and used after the last meaningful character

  enum Wstate {NEWSYMB, INARRAY, INSUBARRAY, END}; // possible states of lexer
  string buf = "";                                 // empty buffer for lexer
  Wstate state = NEWSYMB;                          // Initial state of lexer
  vector<int> vecbuf, resultint;                   // temporary variables
  vector<vector<int>> resultvec;  // container of results to write to width map
  if(s.empty()) return;           // empty string == nothing to do

  // main loop - straight iteration through all the characters in s
  for(auto i : s)
  {
    char a = symbol_type(i); // determine symbol type from method in utilities.h

    // If opening of array not first character, simply wait for '['
    if(state == NEWSYMB)
    {
      switch(a)
      {
        case '[': state = INARRAY; break; // found '[' - now in array
        default : break;
      }
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
    if(state == END)
    {
      break;
    }
  }

  // We now parse the results of the lexing procedure.
  // First we make sure that the starting character codes are equal in length
  // to the number of width arrays, and that neither is empty
  if((resultint.size() == resultvec.size()) && !resultint.empty() )
    // now loop through the vectors and marry char codes to widths
    for(size_t i = 0; i < resultint.size(); i++)
      // Skip any character code that doesn't have an associated width array
      if(!resultvec[i].empty())
        // Now for each member of the width array...
        for(size_t j = 0; j < resultvec[i].size(); j++)
          // map sequential values of char codes to stated widths
          Width[(RawChar) resultint[i] + j] = (int) resultvec[i][j];
}

/*---------------------------------------------------------------------------*/
// The font class needs to know whether to build the glyphmap based on RawChar
// code points or Unicode code points. If the following returns true, the map
// should be built using the raw character values

bool glyphwidths::widthsAreForRaw()
{
  return widthFromCharCodes;
}
