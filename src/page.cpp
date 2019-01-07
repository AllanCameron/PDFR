//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR page implementation file                                            //
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
#include "stringfunctions.h"
#include "Rex.h"
#include "streams.h"
#include "dictionary.h"
#include "document.h"
#include "page.h"
#include "debugtools.h"

/*---------------------------------------------------------------------------*/

void page::boxes()
{
  bleedbox = header.getNums("/BleedBox");
  cropbox  = header.getNums("/CropBox");
  mediabox = header.getNums("/MediaBox");
  artbox   = header.getNums("/ArtBox");
  trimbox  = header.getNums("/TrimBox");
  if (!bleedbox.empty()) minbox = bleedbox;
  if (!mediabox.empty()) minbox = mediabox;
  if (!cropbox.empty())  minbox = cropbox;
  if (!trimbox.empty())  minbox = trimbox;
  if (!artbox.empty())   minbox = artbox;
  if (header.has("/Rotate")) rotate = header.getNums("/Rotate").at(0);
}

/*--------------------------------------------------------------------------*/

void page::getHeader(document& d)
{
  std::string E = "No header found for page ";
  E += std::to_string(pagenumber);
  if ((int) d.pageheaders.size() >= pagenumber)
    header = d.pageheaders[pagenumber];
  else
    Rcpp::stop(E);
  if (!header.has("/Type"))
    Rcpp::stop(E);
  if(header.get("/Type") != "/Page")
    Rcpp::stop(E);
}

/*--------------------------------------------------------------------------*/

void page::getResources(document& d)
{
  if (!header.hasDictionary("/Resources"))
  {
    resourceobjs = header.getRefs("/Resources");
    for (auto q : resourceobjs)
      resources = d.getobject(q).getDict();
  }
  else resources = dictionary(header.get("/Resources"));
}

/*--------------------------------------------------------------------------*/

void page::getFonts(document& d)
{
  if (!resources.hasDictionary("/Font"))
  {
    std::vector<int> fontobjs = resources.getRefs("/Font");
    if (fontobjs.size() == 1)
      fonts = d.getobject(fontobjs.at(0)).getDict();
  }
  else
    fonts = dictionary(resources.get("/Font"));
  fontnames = fonts.getDictKeys();
  for(auto h : fontnames)
    for(auto hh : fonts.getRefs(h))
      fontmap[h] = font(&d, d.getobject(hh).getDict(), h);
}

/*--------------------------------------------------------------------------*/

void page::getContents(document& d)
{
  std::vector<int> cts = header.getRefs("/Contents");
  if (!cts.empty())
  {
    contents = d.expandContents(cts);
    for (auto m : contents)
      contentstring += d.getobject(m).getStream() + std::string("\n");
  }
}

/*---------------------------------------------------------------------------*/

void page::parseXObjStream(document& d)
{
  if (resources.has("/XObject"))
    xobjstring = resources.get("/XObject");
  if(xobjstring.length() > 0)
  {
    if(isDictString(xobjstring))
    {
      dictionary objdict = dictionary(xobjstring);
      std::vector<std::string> dictkeys = objdict.getDictKeys();
      for(auto i : dictkeys)
      {
        std::vector<int> refints = objdict.getRefs(i);
        if(!refints.empty())
          XObjects[i] = d.getobject(refints.at(0)).getStream();
      }
    }
  }
}

/*--------------------------------------------------------------------------*/

page::page(document& d, int pagenum) : pagenumber(pagenum), rotate(0)
{
  std::map<std::string, std::string> blankmap;
  getHeader(d);
  getResources(d);
  parseXObjStream(d);
  getFonts(d);
  getContents(d);
  boxes();
}
