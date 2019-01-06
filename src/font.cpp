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

#include "pdfr.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "debugtools.h"
#include "document.h"
#include "corefonts.h"
#include "font.h"
#include "adobetounicode.h"
#include "chartounicode.h"
#define DEFAULT_WIDTH 500
using namespace std;

/*---------------------------------------------------------------------------*/

font::font(document& d, dictionary Fontref, const string& fontid) :
FontID(fontid), widthFromCharCodes(false)
{
  dictionary fontref = Fontref;
  FontRef = "In file";
  BaseFont = fontref.get("/BaseFont");
  getCoreFont(BaseFont);
  getFontName();
  getEncoding(fontref, d);
  mapUnicode(fontref, d);
  if(Width.empty())
    getWidthTable(fontref, d);
  makeGlyphTable();
}

/*---------------------------------------------------------------------------*/

void font::getFontName()
{
  size_t BaseFont_start = 1;
  if(BaseFont.size() > 7)
    if(BaseFont[7] == '+')
      BaseFont_start = 8;
  FontName = BaseFont.substr(BaseFont_start, BaseFont.size() - BaseFont_start);
}

/*---------------------------------------------------------------------------*/

void parseDifferences(const string& enc, map<RawChar, Unicode>& symbmap)
{
  string state = "newsymbol";
  string buf = "";
  string minibuf = "";
  vector<string> typestring;
  vector<string> symbstring;

  for(auto i : enc)
  {
    if(state == "stop") break;
    if(state == "newsymbol")
    {
      char n = symbol_type(i);
      switch(n)
      {
        case 'D': buf = i ; state = "number"; break;
        case '/': buf = "/"; state = "name"; break;
        case ']': state = "stop"; break;
        default : buf = ""; break;
      }
      i++; continue;
    }
    if(state == "number")
    {
      char n = symbol_type(i);
      switch(n)
      {
        case 'D': buf += i ; break;
        case '/': typestring.push_back(state);
                  symbstring.push_back(buf);
                  buf = "/"; state = "name"; break;
        default:  typestring.push_back(state);
                  symbstring.push_back(buf);
                  buf = ""; state = "newsymbol"; break;
      }
      i++; continue;
    }
    if(state == "name")
    {
      char n = symbol_type(i);
      switch(n)
      {
        case 'L': buf += i;  break;
        case '.': buf += i;  break;
        case 'D': buf += i;  break;
        case '/': typestring.push_back(state);
                  symbstring.push_back(buf);
                  buf = i ; break;
        case ' ': typestring.push_back(state);
                  symbstring.push_back(buf);
                  buf = "" ; state = "newsymbol"; break;
        default:  typestring.push_back(state);
                  symbstring.push_back(buf);
                  state = "stop"; break;
      }
      i++; continue;
    }
  }

  RawChar k = 0;
  for(size_t i = 0; i < symbstring.size(); i++)
    if(typestring[i] == "number")
      k = (RawChar) stoi(symbstring[i]);
    else
      symbmap[k++] = AdobeToUnicode[symbstring[i]];
}

/*---------------------------------------------------------------------------*/

vector<pair<Unicode, int>> font::mapRawChar(vector<RawChar> raw)
{
  vector<pair<Unicode, int>> res;
  for(auto i : raw)
    if(glyphmap.find(i) != glyphmap.end())
      res.push_back(glyphmap[i]);
  return res;
}

/*---------------------------------------------------------------------------*/

void font::getWidthTable(dictionary& dict, document& d)
{
  if (dict.has("/Widths")) parseWidths(dict, d);
  else if(dict.hasRefs("/DescendantFonts")) parseDescendants(dict, d);
}

/*---------------------------------------------------------------------------*/

void font::parseWidths(dictionary& dict, document& d)
{
  vector<float> widtharray;
  RawChar firstchar = 0x0000;
  string numstring = "\\[(\\[|]| |\\.|\\d+)+";
  if(dict.hasInts("/FirstChar"))
    firstchar = dict.getInts("/FirstChar")[0];
  if (dict.hasRefs("/Widths"))
  {
    object_class o = d.getobject(dict.getRefs("/Widths").at(0));
    string ostring = o.getStream();
    vector<string> arrstrings = Rex(ostring, numstring).get();
    if (!arrstrings.empty())
      widtharray = getnums(arrstrings[0]);
  }
  else widtharray = dict.getNums("/Widths");
  if (!widtharray.empty())
  {
    this->widthFromCharCodes = true;
    for (size_t i = 0; i < widtharray.size(); i++)
      Width[firstchar + i] = (int) widtharray[i];
  }
}

/*---------------------------------------------------------------------------*/

void font::parseDescendants(dictionary& dict, document& d)
{
  string numstring = "\\[(\\[|]| |\\.|\\d+)+";
  vector<int> os = dict.getRefs("/DescendantFonts");
  object_class desc = d.getobject(os[0]);
  dictionary descdict = desc.getDict();
  string descstream = desc.getStream();
  if(!getObjRefs(descstream).empty())
    descdict = d.getobject(getObjRefs(descstream)[0]).getDict();
  if (descdict.has("/W"))
  {
    string widthstring;
    if (descdict.hasRefs("/W"))
      widthstring = d.getobject(descdict.getRefs("/W").at(0)).getStream();
    else
      widthstring = descdict.get("/W");
    vector<string> tmp = Rex(widthstring, numstring).get();
    if(!tmp.empty())
    {
      parsewidtharray(tmp[0]);
      this->widthFromCharCodes = true;
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::mapUnicode(dictionary& fontref, document& d)
{
  if (!fontref.hasRefs("/ToUnicode")) return;
  int unirefint = fontref.getRefs("/ToUnicode")[0];
  string x = d.getobject(unirefint).getStream();
  Rex Char =  Rex(x, "beginbfchar(.|\\s)*?endbfchar");
  Rex Range = Rex(x, "beginbfrange(.|\\s)*?endbfrange");
  if (Char.has())  processUnicodeChars(Char);
  if (Range.has()) processUnicodeRange(Range);
}

/*---------------------------------------------------------------------------*/

void font::processUnicodeChars(Rex& Char)
{
  for(auto j : Char.get())
  {
    j = carveout(j, "beginbfchar", "endbfchar");
    vector<string> charentries = splitter(j, "(\n|\r){1,2}");
    for (auto i : charentries)
    {
      vector<string> entries = Rex(i, "(\\d|a|b|c|d|e|f|A|B|C|D|E|F)+").get();
      if (entries.size() == 2)
      {
        Unicode hex = HexstringToRawChar(entries[1]).at(0);
        RawChar key = HexstringToRawChar(entries[0]).at(0);
        EncodingMap[key] = hex;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::processUnicodeRange(Rex& Range)
{
  for(auto j : Range.get())
  {
    string bfrange = carveout(j, "beginbfrange", "endbfrange");
    for (auto i :  splitter(bfrange, "(\n|\r){1,2}"))
    {
      vector<string> entries = Rex(i, "(\\d|a|b|c|d|e|f|A|B|C|D|E|F)+").get();
      if (entries.size() == 3)
      {
        vector<RawChar> myui;
        for(auto k : entries)
          myui.emplace_back(HexstringToRawChar(k).at(0));
        myui.emplace_back((myui[1] - myui[0]) + 1);
        for (size_t j = 0; j < myui[3]; j++)
          EncodingMap[(RawChar) (myui[0] + j)] = (Unicode) (myui[2] + j);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::getEncoding(dictionary fontref, document& d)
{
  dictionary encref = fontref;
  string encname = encref.get("/Encoding");
  if(fontref.hasRefs("/Encoding"))
  {
    int a = fontref.getRefs("/Encoding").at(0);
    object_class myobj = d.getobject(a);
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
    EncodingMap = standardEncodingToUnicode;
  if(encref.has("/Differences"))
  {
    BaseEncoding = encref.get("/Differences");
    parseDifferences(BaseEncoding, EncodingMap);
  }
}

/*---------------------------------------------------------------------------*/

void font::getCoreFont(string s)
{
  font resfont;
  if(s == "/Courier") this->Width = courierwidths;
  else if(s == "/Courier-Bold") this->Width = courierboldwidths;
  else if(s == "/Courier-BoldOblique")this->Width = courierboldobliquewidths;
  else if(s == "/Courier-Oblique") this->Width = courierobliquewidths;
  else if(s == "/Helvetica") this->Width = helveticawidths;
  else if(s == "/Helvetica-Bold") this->Width = helveticaboldwidths;
  else if(s == "/Helvetica-Boldoblique") this->Width = helveticaboldobliquewidths;
  else if(s == "/Helvetica-Oblique") this->Width = helveticaobliquewidths;
  else if(s == "/Symbol") this->Width = symbolwidths;
  else if(s == "/Times-Bold") this->Width = timesboldwidths;
  else if(s == "/Times-BoldItalic") this->Width = timesbolditalicwidths;
  else if(s == "/Times-Italic") this->Width =timesitalicwidths;
  else if(s == "/Times-Roman") this->Width = timesromanwidths;
  else if(s == "/ZapfDingbats") this->Width = dingbatswidths;
  else widthFromCharCodes = true;
}

/*---------------------------------------------------------------------------*/

void font::makeGlyphTable()
{
  vector<RawChar> inkeys = getKeys(EncodingMap);
  if(widthFromCharCodes)
    for(auto i : inkeys)
    {
      int thiswidth = DEFAULT_WIDTH;
      if(Width.find(i) != Width.end())
        thiswidth = Width[i];
      glyphmap[i] = make_pair(EncodingMap[i], thiswidth);
    }
  else
    for(auto i : inkeys)
    {
      int thiswidth = DEFAULT_WIDTH;
      if(Width.find(EncodingMap[i]) != Width.end())
        thiswidth = Width[EncodingMap[i]];
      glyphmap[i] = make_pair(EncodingMap[i], thiswidth);
    }

}

/*---------------------------------------------------------------------------*/

void font::parsewidtharray(string s)
{
  s += " ";
  string buf, state;
  vector<int> vecbuf, resultint;
  vector<vector<int>> resultvec;
  state = "newsymb";
  buf = "";
  size_t i = 0;
  if(s.length() > 0)
  {
    while(i < s.length())
    {
      char a = symbol_type(s[i]);
      if(state == "newsymb")
      {
        switch(a)
        {
          case '[': state = "inarray"; break;
          default : break;
        }
        i++; continue;
      }
      if(state == "inarray")
      {
        switch(a)
        {
        case 'D' : buf += s[i]; break;
        case '.' : buf += s[i]; break;
        case '[' : state = "insubarray";
                   if(!buf.empty())
                   {
                     vecbuf.push_back(stoi(buf));
                     if(vecbuf.size() == 1)
                       resultint.push_back(vecbuf[0]);
                     else
                       resultvec.push_back(vecbuf);
                   }
                   buf = "";
                   vecbuf.clear();
                   break;
        case ' ' : if(!buf.empty())
                     vecbuf.push_back(stoi(buf));
                  buf = ""; break;
        case ']': state = "end";
                  if(!buf.empty())
                  {
                    vecbuf.push_back(stoi(buf));
                    if(vecbuf.size() == 1) resultint.push_back(vecbuf[0]);
                    else resultvec.push_back(vecbuf);
                  }
                  buf = "";
                  vecbuf.clear();
                  break;
        default: throw (string("Error parsing string ") + s);

        }
        i++; continue;
      }
      if(state == "insubarray")
      {
        switch(a)
        {
        case ' ': if(!buf.empty()) vecbuf.push_back(stoi(buf));
                  buf = ""; break;
        case ']': state = "inarray";
                  if(!buf.empty()) vecbuf.push_back(stoi(buf));
                  resultvec.push_back(vecbuf);
                  vecbuf.clear();
                  buf = ""; break;
        case 'D': buf += s[i]; break;
        default: throw (string("Error parsing string ") + s);
        }
        i++; continue;
      }
      if(state == "end")
      {
        break;
      }
    }
  }
  if((resultint.size() == resultvec.size()) && !resultint.empty() )
    for(size_t i = 0; i < resultint.size(); i++)
      if(!resultvec[i].empty())
        for(size_t j = 0; j < resultvec[i].size(); j++)
          Width[(RawChar) resultint[i] + j] = (int) resultvec[i][j];
}
