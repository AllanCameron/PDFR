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

/*---------------------------------------------------------------------------*/

std::vector<double> boxarray(const std::string& box)
{
  std::vector<double> res;
  std::vector<std::string> ns = Rex(box, "(\\.|0|1|2|3|4|5|6|7|8|9)+").get();
  std::string::size_type sz;
  for (auto i : ns)
    res.push_back(stod(i, & sz));
  return res;
}

/*--------------------------------------------------------------------------*/

page::page(document& d, int pagenum) : pagenumber(pagenum)
{
  std::map<std::string, std::string> blankmap;
  std::string bleedBS, cropBS, mediaBS, artBS, trimBS, noheader;
  noheader += "Page range error. No header found for page ";
  noheader += std::to_string(pagenum);
  if (d.pageheaders.size() < (size_t) pagenum) Rcpp::stop(noheader);
  header = d.pageheaders[pagenum];
  if (!header.has("/Type")) Rcpp::stop(noheader);
  if(header.get("/Type") != "/Page") Rcpp::stop(noheader);
  bleedBS = header.get("/BleedBox");
  cropBS = header.get("/CropBox");
  mediaBS = header.get("/MediaBox");
  artBS = header.get("/ArtBox");
  trimBS  = header.get("/TrimBox");
  rotate = 0;
  if (header.has("/Rotate"))  rotate = header.getNums("/Rotate").at(0);
  if (!header.hasDictionary("/Resources"))
    {
      resourceobjs = header.getRefs("/Resources");
      for (auto q : resourceobjs) resources = d.getobject(q).getDict();
    }
  else resources = dictionary(header.get("/Resources"));
  if (resources.has("/XObject")) xobjstring = resources.get("/XObject");
  parseXObjStream(d);
  if (!resources.hasDictionary("/Font"))
  {
    std::vector<int> fontobjs = resources.getRefs("/Font");
    if (fontobjs.size() == 1) fonts = d.getobject(fontobjs.at(0)).getDict();
  }
  else fonts = dictionary(resources.get("/Font"));
  std::vector<int> cts = header.getRefs("/Contents");
  if (!cts.empty())
  {
    contents = d.expandContents(cts);
    for (auto m : contents)
    {
      contentstring += d.getobject(m).getStream();
      contentstring += "\n";
    }
  }
  if (!bleedBS.empty()) {bleedbox = boxarray(bleedBS); minbox = bleedbox;}
  if (!mediaBS.empty()) {mediabox = boxarray(mediaBS); minbox = mediabox;}
  if (!cropBS.empty()) { cropbox = boxarray(cropBS); minbox = cropbox;}
  if (!trimBS.empty()) { trimbox = boxarray(trimBS); minbox = trimbox;}
  if (!artBS.empty()) { artbox = boxarray(artBS); minbox = artbox;}

  fontnames = fonts.getDictKeys();
  for(auto h : fontnames)
    for(auto hh : fonts.getRefs(h))
    {
      fontmap[h] = font(d, d.getobject(hh).getDict(), h);
    }
}

/*---------------------------------------------------------------------------*/


void page::parseXObjStream(document& d)
{
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
