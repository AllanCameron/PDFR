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
#include "encodings.h"
#include "corefonts.h"
#include "font.h"
#include "ucm.h"

using namespace std;

/*---------------------------------------------------------------------------*/

font::font(document& d, const dictionary& Fontref, const string& fontid) :
FontID(fontid)
{
  dictionary fontref = Fontref;
  FontRef = "In file";
  BaseFont = fontref.get("/BaseFont");
  getCoreFont(BaseFont);
  getFontName();
  getEncoding(fontref, d);
  mapUnicode(fontref, d);
  if(Width.size() == 0) getWidthTable(fontref, d);
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

void parseDifferences(const string& enc, map<uint16_t, uint16_t>& symbmap)
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
      case 'D': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = i ; state = "number"; break;
      case '/': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = i ; break;
      case ' ': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = "" ; state = "newsymbol"; break;
      default: typestring.push_back(state);
      symbstring.push_back(buf);
      state = "stop"; break;
      }
      i++; continue;
    }
  }

  size_t k = 0;
  for(size_t i = 0; i < symbstring.size(); i++)
  {
    if(typestring[i] == "number")
    {
      k = stoi(symbstring[i]);
    }
    else
    {
      symbmap[k] = UCM[symbstring[i]];
      k++;
    }
  }
}

/*---------------------------------------------------------------------------*/

vector<pair<uint16_t, int>> font::mapString(const string& s)
{
  GlyphMap &G = glyphmap;
  vector<pair<uint16_t, int>> res;
  for(auto i : s)
    if(G.find((int) i) != G.end())
      res.push_back(G[i]);
  return res;
}

/*---------------------------------------------------------------------------*/

void font::getWidthTable(dictionary& dict, document& d)
{
  map<uint16_t, int> resmap;
  vector<int> chararray;
  vector<float> widtharray;
  int firstchar;
  string widthstrings, fcstrings;
  string numstring = "\\[(\\[|]| |\\.|\\d+)+";
  if (dict.has("/Widths"))
  {
    widthstrings = dict.get("/Widths");
    fcstrings = dict.get("/FirstChar");
    if (dict.hasRefs("/Widths"))
    {
      vector<int> os = dict.getRefs("/Widths");
      if (os.size() > 0)
      {
        object_class o = d.getobject(os[0]);
        string ostring = o.getStream();
        vector<string> arrstrings = Rex(ostring, numstring).get();
        if (arrstrings.size() > 0)
        {
          widtharray = getnums(arrstrings[0]);
        }
      }
    }
    else widtharray = dict.getNums("/Widths");
    if (!widtharray.empty() && !fcstrings.empty())
    {
      vector<int> fcnums = getints(fcstrings);
      if (!fcnums.empty())
      {
        firstchar = fcnums[0];
        size_t warrsize = widtharray.size();
        for (unsigned i = 0; i < warrsize; i++)
        {
          resmap[(uint16_t) firstchar + i] = (int) widtharray[i];
        }
        Width = resmap;
      }
    }
  }
  else
  {
    widthstrings = dict.get("/DescendantFonts");
    if(dict.has("/DescendantFonts"))
      if (dict.hasRefs("/DescendantFonts"))
      {
        vector<int> os = dict.getRefs("/DescendantFonts");
        if (!os.empty())
        {
          object_class desc = d.getobject(os[0]);
          dictionary descdict = desc.getDict();
          string descstream = desc.getStream();
          if(!getObjRefs(descstream).empty())
            descdict = d.getobject(getObjRefs(descstream)[0]).getDict();
          if (descdict.has("/W"))
          {
            if (descdict.hasRefs("/W"))
            {
              vector<int> osss = descdict.getRefs("/W");
              if (!osss.empty())
                widthstrings = d.getobject(osss[0]).getStream();
            }
            else widthstrings = descdict.get("/W");
            vector<string> tmp = Rex(widthstrings, numstring).get();
            if(!tmp.empty())
              parsewidtharray(tmp[0]);
          }
        }
      }
  }
  if(Width.empty())
  {
    map<uint16_t, uint16_t> storeEnc = EncodingMap;
    getCoreFont("/Helvetica");
    EncodingMap = storeEnc;
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
        string myhex = entries[1];
        if(myhex.length() < 4) while(myhex.length() < 4) myhex = "0" + myhex;
        upperCase(myhex);
        string mykey = entries[0];
        upperCase(mykey);
        if(mykey.length() < 4) while(mykey.length() < 4) mykey = "0" + mykey;
        mykey = "0x" + mykey.substr(2, 2);
        uint16_t uintkey = stoul(mykey, nullptr, 0);
        if(EncodingMap.find(uintkey) != EncodingMap.end())
          EncodingMap[uintkey] = uintkey;
        else
          EncodingMap[uintkey] = uintkey;
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
        vector<uint16_t> myui;
        for(auto k : entries)
          myui.emplace_back(stoul(string("0x" + k), nullptr, 0));
        myui.emplace_back((myui[1] - myui[0]) + 1);
        for (unsigned int j = 0; j < myui[3]; j++)
        {
          uint16_t unicodeIndex = myui[2] + j;
          uint16_t mykey = myui[0] + j;
          if(EncodingMap.find(mykey) != EncodingMap.end())
            EncodingMap[mykey] = unicodeIndex;
          else EncodingMap[mykey] = mykey;
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::getEncoding(dictionary& fontref, document& d)
{
  dictionary &encref = fontref;
  string encname = encref.get("/Encoding");
  if(fontref.hasRefs("/Encoding"))
  {
    int a = fontref.getRefs("/Encoding").at(0);
    object_class myobj = d.getobject(a);
    encref = myobj.getDict();
    if(encref.has("/BaseEncoding"))
      encname = encref.get("/BaseEncoding");
  }
  if( encname == "/WinAnsiEncoding" ||
      encname == "/MacRomanEncoding" ||
      encname == "/PDFDocEncoding" ||
      encname == "/StandardEncoding")
  {
    BaseEncoding = encname;
    EncodingMap = getBaseEncode(encname);
  }
  else
  {
    vector<int> encrefs = getObjRefs(encname);
    if(!encrefs.empty())
    {
      dictionary encdict = d.getobject(encrefs[0]).getDict();
      BaseEncoding = encdict.get("/Differences");
      EncodingMap = getBaseEncode("/StandardEncoding");
      parseDifferences(BaseEncoding, EncodingMap);
    }
    else
    {
      BaseEncoding = "/StandardEncoding";
      EncodingMap = getBaseEncode(BaseEncoding);
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::getCoreFont(string s)
{
  font resfont;
  if(s == "/Courier") *this = getCourier();
  if(s == "/Courier-Bold") *this = getCourierBold();
  if(s == "/Courier-BoldOblique") *this = getCourierBO();
  if(s == "/Courier-Oblique") *this = getCourierOblique();
  if(s == "/Helvetica") *this = getHelvetica();
  if(s == "/Helvetica-Bold") *this = getHelveticaBold();
  if(s == "/Helvetica-Boldoblique") *this = getHelveticaBO();
  if(s == "/Helvetica-Oblique") *this = getHelveticaOblique();
  if(s == "/Symbol") *this = getSymbol();
  if(s == "/Times-Bold") *this = getTimesBold();
  if(s == "/Times-BoldItalic") *this = getTimesBI();
  if(s == "/Times-Italic") *this = getTimesItalic();
  if(s == "/Times-Roman") *this = getTimesRoman();
  if(s == "/ZapfDingbats") *this = getDingbats();
}

/*---------------------------------------------------------------------------*/

void font::makeGlyphTable()
{
  vector<uint16_t> widthkeys = getKeys(Width);
  int defwidth = 500;
  vector<uint16_t> inkeys = getKeys(EncodingMap);
  for(size_t i = 0; i < inkeys.size(); i++)
  {
    int thiswidth;
    if(Width.find(inkeys[i]) == Width.end()) thiswidth = defwidth;
    else thiswidth = Width[(int) inkeys[i]];
    glyphmap[inkeys[i]] = make_pair(EncodingMap[inkeys[i]], thiswidth);
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
        case '[': state = "insubarray";
                  if(buf.size() > 0)
                  {
                    vecbuf.push_back(stoi(buf));
                    if(vecbuf.size() == 1) resultint.push_back(vecbuf[0]);
                    else resultvec.push_back(vecbuf);
                  }
                  buf = "";
                  vecbuf.clear();
                  break;
        case ' ': if(buf.size() > 0)
                  {
                    vecbuf.push_back(stoi(buf));
                  }
                  buf = ""; break;
        case ']': state = "end";
                  if(buf.size() > 0)
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
        case ' ': if(buf.size() > 0) vecbuf.push_back(stoi(buf));
                  buf = ""; break;
        case ']': state = "inarray";
                  if(buf.size() > 0) vecbuf.push_back(stoi(buf));
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
  map<uint16_t, int> resultmap;
  if((resultint.size() == resultvec.size()) && !resultint.empty() )
  {
    size_t ressize = resultint.size();
    for(size_t i = 0; i < ressize; i++)
    {
      size_t rvecsize = resultvec[i].size();
      if(!resultvec[i].empty())
        for(size_t j = 0; j < rvecsize; j++)
          resultmap[(uint16_t) resultint[i] + j] = resultvec[i][j];
    }
  }
  Width = resultmap;
}
