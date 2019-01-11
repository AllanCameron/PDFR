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

#include "pdfr.h"
#include "adobetounicode.h"
#include "chartounicode.h"
#include "corefonts.h"
#include "encoding.h"
#include "font.h"
#include "document.h"
#include "object_class.h"
#include "dictionary.h"

#define DEFAULT_WIDTH 500;

/*---------------------------------------------------------------------------*/

void glyphwidths::getWidthTable(dictionary& dict, document* d)
{
  if (dict.has("/Widths"))
    parseWidths(dict, d);
  else if(dict.hasRefs("/DescendantFonts"))
    parseDescendants(dict, d);
}

/*---------------------------------------------------------------------------*/

void glyphwidths::parseWidths(dictionary& dict, document* d)
{
  vector<float> widtharray;
  RawChar firstchar = 0x0000;
  string numstring = "\\[(\\[|]| |\\.|\\d+)+"; // numeric array regex
  if(dict.hasInts("/FirstChar"))
    firstchar = dict.getInts("/FirstChar")[0];
  if (dict.hasRefs("/Widths"))
  {
    object_class o = d->getobject(dict.getRefs("/Widths").at(0));
    string ostring = o.getStream();
    widtharray = getnums(ostring);
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

void glyphwidths::parseDescendants(dictionary& dict, document* d)
{
  string numstring = "\\[(\\[|]| |\\.|\\d+)+"; // numeric array regex
  vector<int> os = dict.getRefs("/DescendantFonts");
  object_class desc = d->getobject(os[0]);
  dictionary descdict = desc.getDict();
  string descstream = desc.getStream();
  if(!getObjRefs(descstream).empty())
    descdict = d->getobject(getObjRefs(descstream)[0]).getDict();
  if (descdict.has("/W"))
  {
    string widthstring;
    if (descdict.hasRefs("/W"))
      widthstring = d->getobject(descdict.getRefs("/W").at(0)).getStream();
    else
      widthstring = descdict.get("/W");
    parsewidtharray(widthstring);
    this->widthFromCharCodes = true;
  }
}

/*---------------------------------------------------------------------------*/

void glyphwidths::getCoreFont(string s)
{
       if(s == "/Courier") Width = courierwidths;
  else if(s == "/Courier-Bold") Width = courierboldwidths;
  else if(s == "/Courier-BoldOblique") Width = courierboldobliquewidths;
  else if(s == "/Courier-Oblique") Width = courierobliquewidths;
  else if(s == "/Helvetica") Width = helveticawidths;
  else if(s == "/Helvetica-Bold") Width = helveticaboldwidths;
  else if(s == "/Helvetica-Boldoblique") Width = helveticaboldobliquewidths;
  else if(s == "/Helvetica-Oblique") Width = helveticaobliquewidths;
  else if(s == "/Symbol") Width = symbolwidths;
  else if(s == "/Times-Bold") Width = timesboldwidths;
  else if(s == "/Times-BoldItalic") Width = timesbolditalicwidths;
  else if(s == "/Times-Italic") Width =timesitalicwidths;
  else if(s == "/Times-Roman") Width = timesromanwidths;
  else if(s == "/ZapfDingbats") Width = dingbatswidths;
  else widthFromCharCodes = true;
}

/*---------------------------------------------------------------------------*/

int glyphwidths::getwidth(RawChar raw)
{
  if(Width.find(raw) != Width.end())
    return Width[raw];
  else
    return DEFAULT_WIDTH;
}

/*---------------------------------------------------------------------------*/

vector<RawChar> glyphwidths::widthKeys()
{
  return getKeys(this->Width);
}

/*---------------------------------------------------------------------------*/

void glyphwidths::parsewidtharray(string s)
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

/*---------------------------------------------------------------------------*/

glyphwidths::glyphwidths(font* f)
{
  fontref = f->fontref;
  d = f->d;
  getCoreFont(f->BaseFont);
  if(Width.empty())
    getWidthTable(fontref, d);
}
