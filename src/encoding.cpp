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

#include "pdfr.h"
#include "adobetounicode.h"
#include "chartounicode.h"
#include "encoding.h"
#include "font.h"
#include "document.h"
#include "object_class.h"
#include "dictionary.h"
#include "debugtools.h"


enum DiffState
{
  NEWSYMB = 0,
  NUM,
  NAME,
  STOP
};

/*---------------------------------------------------------------------------*/

void parseDifferences(const string& enc, map<RawChar, Unicode>& symbmap)
{
  DiffState state = NEWSYMB;
  string buffer = "";
  vector<pair<DiffState, string>> entries;

  for(auto i : enc)
  {
    char n = symbol_type(i);
    switch(state)
    {
      case NEWSYMB:   switch(n)
                      {
                        case 'D': buffer = i ; state = NUM; break;
                        case '/': buffer = i; state = NAME; break;
                        case ']': state = STOP; break;
                        default : buffer = ""; break;
                      };
                      break;
      case NUM:       switch(n)
                      {
                        case 'D': buffer += i ; break;
                        case '/': entries.push_back(make_pair(state, buffer));
                                  buffer = i; state = NAME; break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  buffer = ""; state = NEWSYMB; break;
                      }
                      break;
      case NAME:      switch(n)
                      {
                        case 'L': buffer += i;  break;
                        case '.': buffer += i;  break;
                        case 'D': buffer += i;  break;
                        case '/': entries.push_back(make_pair(state, buffer));;
                                  buffer = i ; break;
                        case ' ': entries.push_back(make_pair(state, buffer));
                                  buffer = "" ; state = NEWSYMB; break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  state = STOP; break;
                      }
                      break;
      default:        break;
    }
    if(state == STOP) break;
  }
  RawChar k = 0;
  for(size_t i = 0; i < entries.size(); i++)
    if(entries[i].first == NUM)
      k = (RawChar) stoi(entries[i].second);
    else
      symbmap[k++] = AdobeToUnicode[entries[i].second];
}

/*---------------------------------------------------------------------------*/

void Encoding::mapUnicode(dictionary& fontref, document* d)
{
  if (!fontref.hasRefs("/ToUnicode")) return;
  int unirefint = fontref.getRefs("/ToUnicode")[0];
  string x = d->getobject(unirefint).getStream();
  vector<string> Char =  multicarve(x, "beginbfchar", "endbfchar");
  vector<string> Range = multicarve(x, "beginbfrange", "endbfrange");
  if (Char.size() > 0)  processUnicodeChars(Char);
  if (Range.size() > 0) processUnicodeRange(Range);
}

/*---------------------------------------------------------------------------*/

void Encoding::processUnicodeChars(vector<string>& Char)
{
  for(auto j : Char)
  {
    vector<string> allentries = multicarve(j, "<", ">");
    RawChar key;
    for(size_t i = 0; i < allentries.size(); i++)
    {
      if(i % 2 == 0)
        key = HexstringToRawChar(allentries[i]).at(0);
      else
        EncodingMap[key] = (Unicode) HexstringToRawChar(allentries[i]).at(0);
    }
  }
}

/*---------------------------------------------------------------------------*/

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

void Encoding::getEncoding(dictionary fontref, document* d)
{
  dictionary encref = fontref;
  string encname = encref.get("/Encoding");
  if(fontref.hasRefs("/Encoding"))
  {
    int a = fontref.getRefs("/Encoding").at(0);
    object_class myobj = d->getobject(a);
    encref = myobj.getDict();
    if(encref.has("/BaseEncoding"))
      encname = encref.get("/BaseEncoding");
  }
  if( encname == "/WinAnsiEncoding")
    EncodingMap = winAnsiEncodingToUnicode;
  else if(encname == "/MacRomanEncoding")
    EncodingMap = macRomanEncodingToUnicode;
  else if(encname == "/PDFDocEncoding")
    EncodingMap = pdfDocEncodingToUnicode;
  else
    for(RawChar i = 0; i < 256; i++)
      EncodingMap[i] = (Unicode) i;
  if(encref.has("/Differences"))
  {
    BaseEncoding = encref.get("/Differences");
    parseDifferences(BaseEncoding, EncodingMap);
  }
}

/*---------------------------------------------------------------------------*/

Encoding::Encoding(font* f)
{
  fontref = f->fontref;
  d = f->d;
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
