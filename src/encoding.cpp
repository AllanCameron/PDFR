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
// /Differences entry as a string and reads its ints as input code points. These
// are mapped to the subsequent glyph name's Unicode value. If more than one
// name follows an int, then sequential code points are mapped to each
// successive name's Unicode value. This requires the static adobetounicode map.

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
  for(auto& i : entries)
    if(i.first == NUM)  // If the vector entry is a number, convert to RawChar
      k = (RawChar) stoi(i.second);
    else
    // otherwise it's a name - convert it to Unicode, write it to the
    // EncodingMap and post-increment the mapping rawchar in case the next
    // entry is also a name (it will be over-written if it is an int)
      EncodingMap[k++] = Encoding::AdobeToUnicode[i.second];
}

/*---------------------------------------------------------------------------*/
// co-ordinates the reading of the /ToUnicode entry, which points to an
// object whose stream contains a CMap

void Encoding::mapUnicode(dictionary& fontref, shared_ptr<document> d)
{
  if (!fontref.hasRefs("/ToUnicode")) return; // no entry, nothing to be done
  int unirefint = fontref.getRefs("/ToUnicode")[0]; // else get reference
  string x = d->getobject(unirefint)->getStream(); // get stream from reference

  // multicarve gets all strings between the bookending strings. These are
  // stored in a vector and are substrates for the processing methods below
  vector<string> Char =  multicarve(x, "beginbfchar", "endbfchar");
  vector<string> Range = multicarve(x, "beginbfrange", "endbfrange");

  if (!Char.empty())  processUnicodeChars(Char); // if chars, processChar
  if (!Range.empty()) processUnicodeRange(Range); // if ranges, processRange
}

/*---------------------------------------------------------------------------*/
// This method parses the "bfchar" entries in the CMap and adds them to the
// encoding map

void Encoding::processUnicodeChars(vector<string>& Char)
{
  for(auto& j : Char)
  {
    // use multicarve() to get ascii-encoded byte representations
    vector<string> allentries = multicarve(j, "<", ">");
    // This loop takes the ascii-encoded bytes in the first column and
    // converts them to rawchar. It stores this number as 'key', then
    // converts the ascii-encoded bytes in the second column, converts them
    // to Unicode and makes them the raw key's mapped value in the encoding map
    for(size_t i = 0; i < (allentries.size() - 1); i += 2)
    {
        RawChar key = HexstringToRawChar(allentries[i])[0];
        EncodingMap[key] = (Unicode) HexstringToRawChar(allentries[i + 1])[0];
    }
  }
}

/*---------------------------------------------------------------------------*/
// This function parses the "bfrange" entries in the CMap and adds them to the
// encoding map. The bfrange comprises three numbers: the first code point to be
// translated in a range of code points, the last code point to be
// translated in the range, and the Unicode point from which to start the
// translation - hence { 1; 4; 10 } would generate {1,10; 2,11; 3,12; 4,13}

void Encoding::processUnicodeRange(vector<string>& Range)
{
  for(auto& i : Range)
  { // uses multicarve() from utilities.h to get ascii-endoded byte strings
    vector<string> allentries = multicarve(i, "<", ">");
    // sanity check - there should be at least three entries for a valid range
    if(allentries.size() < 3) throw runtime_error("No entries in range");
    // loop to calculate entries and fill encoding map
    for(size_t j = 2; j < allentries.size(); j += 3)
    {
      // first column == first code point
      RawChar first = HexstringToRawChar(allentries[j - 2]).at(0);
      // second column == first code point
      RawChar last = HexstringToRawChar(allentries[j - 1]).at(0);
      // We call the HexstringtoRawChar function as it outputs uint16
      // However, 'start' is Unicode and we make this explicit
      Unicode start = (Unicode) HexstringToRawChar(allentries[j]).at(0);
      // Now we can fill the encoding map from the data in the row
      for(int k = 0; k <= (last - first); k++)
        EncodingMap[first + k] = start + k;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Extract the encoding dictionary and read off entries for base encoding.
// Calls the Differences lexer if there is a dictionary entry for /Differences

void Encoding::getEncoding(dictionary& fontref, shared_ptr<document> d)
{
  dictionary encref = fontref;                  // start with font dictionary
  string encname = encref.get("/Encoding");     // read encoding entry
  if(fontref.hasRefs("/Encoding"))              //------//
  {                                                     // if an encoding
    int a = fontref.getRefs("/Encoding").at(0);         // dictionary exists,
    shared_ptr<object_class> myobj = d->getobject(a);   // get it and read off
    encref = myobj->getDict();                          // the baseencoding
    if(encref.has("/BaseEncoding"))                     // entry
      encname = encref.get("/BaseEncoding");            //
  }                                             //------//
  if( encname == "/WinAnsiEncoding")            //---------------------------//
    EncodingMap = Encoding::winAnsiEncodingToUnicode; //
  else if(encname == "/MacRomanEncoding")             // Fill encoding map with
    EncodingMap = Encoding::macRomanEncodingToUnicode;// the appropriate base
  else if(encname == "/PDFDocEncoding")               // encoding. If none is
    EncodingMap = Encoding::pdfDocEncodingToUnicode;  // specified, fill map
  else                                                // with 1:1 Raw : Unicode
    for(RawChar i = 0; i < 256; ++i)                  //
      EncodingMap[i] = (Unicode) i;             //---------------------------//
  if(encref.has("/Differences"))
  {// call Differences() if a /Differences entry is found to modify encoding
    BaseEncoding = encref.get("/Differences");
    Differences(BaseEncoding);
  }
}

/*---------------------------------------------------------------------------*/
// The constructor function needs to do three things - get the encoding
// dictionary, get the base encoding, parse any /Differences entry, and parse
// any /ToUnicode entry. The first three are co-ordinated by getEncoding()
// and the last is co-ordinated by mapUnicode()

Encoding::Encoding(const dictionary& fontdict, shared_ptr<document> doc):
  fontref(fontdict), d(doc)
{
  getEncoding(fontref, d);
  mapUnicode(fontref, d);
}

/*---------------------------------------------------------------------------*/
// This is a simple public interface to the underlying map data. It provides
// a range-checked lookup of a given RawChar. If it finds no Unicode entry
// in the map it returns the original RawChar

Unicode Encoding::Interpret(RawChar raw)
{
  if(EncodingMap.find(raw) != EncodingMap.end()) // non-inserting lookup
    return EncodingMap[raw];  // found entry
  else
    return raw;               // no entry - have your char back untranslated
}

/*---------------------------------------------------------------------------*/
// Public function to return a vector that enumerates all the keys in the main
// Rawchar - Unicode map

std::shared_ptr<std::unordered_map<RawChar, Unicode>> Encoding::encKeys()
{
  return std::make_shared<std::unordered_map<RawChar, Unicode>>(EncodingMap);
}
