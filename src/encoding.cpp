//---------------------------------------------------------------------------//
//                                                                           //
// PDFR encoding implementation file                                         //
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

#include "encoding.h"

using namespace std;

/*---------------------------------------------------------------------------*/
// Lexer / parser for Differences entry of encoding dictionary. Takes the
// entry as a string and reads ints as input code points. These are mapped to
// the following glyph name's unicode value. If more than one name follows an
// int, then sequential code points are mapped to each successive name's
// unicode value

void Encoding::Differences(const string& enc)
{
  DiffState state = NEWSYMB; // define starting state
  string buffer = "";        // initialise the buffer string

  // The entries vector gives a pair of type : entry for each entity pushed
  // onto the stack by the lexer. We therefore know whether we are dealing with
  // a code point or a name when we parse the stack
  vector<pair<DiffState, string>> entries;

  // The loop now cycles through each character in the differences string,
  // switching states and pushing results onto the stack according to the
  // type of character it encounters. This is quite a small lexer for a small
  // parsing task, so I have kept it as a single function (albeit one with
  // nested switch - case expressions)

  for(auto i : enc)
  {
    char n = symbol_type(i);  // determine character type
    switch(state)             // state switch
    {
      case NEWSYMB:   switch(n)   // if number switch to NUM, if / to NAME
                      {           // stop if closing bracket, else continue
                        case 'D': buffer = i; state = NUM;      break;
                        case '/': buffer = i; state = NAME;     break;
                        case ']': state = STOP;                 break;
                        default : buffer = "";                  break;
                      };
                      break;

      case NUM:       switch(n)   // write number to buffer, switch to name
                      {           // if char is '/'
                        case 'D': buffer += i ;                 break;
                        case '/': entries.push_back(make_pair(state, buffer));
                                  buffer = i; state = NAME;     break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  buffer = ""; state = NEWSYMB; break;
                      }
                      break;
      case NAME:      switch(n)   // write name unless space or slash etc
                      {
                        case 'L': buffer += i;                  break;
                        case '.': buffer += i;                  break;
                        case 'D': buffer += i;                  break;
                        case '/': entries.push_back(make_pair(state, buffer));
                                  buffer = i ;                  break;
                        case ' ': entries.push_back(make_pair(state, buffer));
                                  buffer = "" ; state = NEWSYMB;break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  state = STOP;                 break;
                      }
                      break;
      default:        break;
    }
    if(state == STOP) break;
  }
  RawChar k = 0; // k represents the code point to be mapped to Unicode
  // This loop writes the results vector to EncodingMap
  for(auto i : entries)
    if(i.first == NUM)  // If the vector entry is a number, convert to RawChar
      k = (RawChar) stoi(i.second);
    else // otherwise it's a name - write to EncodingMap and post-increment k
      EncodingMap[k++] = Encoding::AdobeToUnicode[i.second];
}

/*---------------------------------------------------------------------------*/
// co-ordinates the reading of the /ToUnicode entry, which points to an
// object whose stream contains a CMap

void Encoding::mapUnicode(dictionary& fontref, document* d)
{
  if (!fontref.hasRefs("/ToUnicode")) return; // no entry, nothing to be done
  int unirefint = fontref.getRefs("/ToUnicode")[0]; // else get reference
  string x = d->getobject(unirefint)->getStream(); // stream from reference

  // multicarve gets all strings between the bookending strings
  vector<string> Char =  multicarve(x, "beginbfchar", "endbfchar");
  vector<string> Range = multicarve(x, "beginbfrange", "endbfrange");

  if (Char.size() > 0)  processUnicodeChars(Char); // if chars, process
  if (Range.size() > 0) processUnicodeRange(Range); // if ranges, procexs
}

/*---------------------------------------------------------------------------*/
// This function parses the "bfchar" entries in the CMap and adds them to the
// encoding map

void Encoding::processUnicodeChars(vector<string>& Char)
{
  for(auto j : Char)
  {
    // use multicarve() to get ascii-encoded byte representations
    vector<string> allentries = multicarve(j, "<", ">");
    RawChar key; // temporary variable representing code point to be mapped

    // This loop takes the ascii-encoded bytes in the first column and
    // converts them to rawchar. It stores this number as 'key', then
    // converts the ascii-encoded bytes in the second column, converts them
    // to Unicode and makes them the value for 'key' in the encoding map
    for(size_t i = 0; i < allentries.size(); i++)
    {
      if(i % 2 == 0) // even index == first column
        key = HexstringToRawChar(allentries[i]).at(0);
      else // odd index == second column
        EncodingMap[key] = (Unicode) HexstringToRawChar(allentries[i]).at(0);
    }
  }
}

/*---------------------------------------------------------------------------*/
// This function parses the "bfrange" entries in the CMap and adds them to the
// encoding map

void Encoding::processUnicodeRange(vector<string>& Range)
{
  for(auto i : Range)
  {
    vector<string> allentries = multicarve(i, "<", ">");
    if(allentries.size() < 3) throw runtime_error("No entries in range");

    RawChar first, last, start;
    first = last = start = 0;
    for(size_t j = 0; j < allentries.size(); j++)
    {
      if(j % 3 == 0) first = HexstringToRawChar(allentries[j]).at(0);
      if(j % 3 == 1) last = HexstringToRawChar(allentries[j]).at(0);
      if(j % 3 == 2)
      {
        start = HexstringToRawChar(allentries[j]).at(0);
        int nchar = (last - first) + 1;
        for(int k = 0; k < nchar; k++)
          EncodingMap[first + k] = start + k;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// Extract the encoding dictionary and read off entries for base encoding.
// Calls the Differences lexer if there is a dictionary entry for /Differences

void Encoding::getEncoding(dictionary& fontref, document* d)
{
  dictionary encref = fontref;                  // start with font dictionary
  string encname = encref.get("/Encoding");     // read encoding entry

  if(fontref.hasRefs("/Encoding"))              //------//
  {                                                     // if an encoding
    int a = fontref.getRefs("/Encoding").at(0);         // dictionary exists,
    object_class* myobj = d->getobject(a);              // get it and read off
    encref = myobj->getDict();                          // the baseencoding
    if(encref.has("/BaseEncoding"))                     // entry
      encname = encref.get("/BaseEncoding");            //
  }                                             //------//

  if( encname == "/WinAnsiEncoding")            //---------------------------//
    EncodingMap = Encoding::winAnsiEncodingToUnicode; //
  else if(encname == "/MacRomanEncoding")             // fill encoding map with
    EncodingMap = Encoding::macRomanEncodingToUnicode;// the appropriate base
  else if(encname == "/PDFDocEncoding")               // encoding. If none
    EncodingMap = Encoding::pdfDocEncodingToUnicode;  // specified, fill map
  else                                                // with 1:1 raw:unicode
    for(RawChar i = 0; i < 256; i++)                  //
      EncodingMap[i] = (Unicode) i;             //---------------------------//

  // call Differences() if entry found to modify base encoding
  if(encref.has("/Differences"))
  {
    BaseEncoding = encref.get("/Differences");
    Differences(BaseEncoding);
  }
}

/*---------------------------------------------------------------------------*/

Encoding::Encoding(const dictionary& fontdict, document* doc):
  fontref(fontdict), d(doc)
{
  getEncoding(fontref, d);
  mapUnicode(fontref, d);
}

/*---------------------------------------------------------------------------*/

Unicode Encoding::Interpret(RawChar raw)
{
  if(EncodingMap.find(raw) != EncodingMap.end())
    return EncodingMap[raw];
  else
    return raw;
}

/*---------------------------------------------------------------------------*/

vector<RawChar> Encoding::encKeys()
{
  return getKeys(this->EncodingMap);
}
