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
  for (auto i : ns) res.push_back(stod(i, & sz));
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
  if (header.has("/Rotate"))  rotate = boxarray(header.get("/Rotate"))[0];

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
    if (fontobjs.size() == 1) fonts = d.getobject(fontobjs[0]).getDict();
  }
  else fonts = dictionary(resources.get("/Font"));

  std::vector<int> cts = header.getRefs("/Contents");
  if (cts.size() > 0)
  {
    contents = d.expandContents(cts);
    for (auto m : contents)
    {
      contentstring += d.getobject(m).getStream();
      contentstring += "\n";
    }
  }

  if (bleedBS.size() > 0) {bleedbox = boxarray(bleedBS); minbox = bleedbox;}
  if (mediaBS.size() > 0) {mediabox = boxarray(mediaBS); minbox = mediabox;}
  if (cropBS.size()  > 0) { cropbox = boxarray(cropBS); minbox = cropbox;}
  if (trimBS.size()  > 0) { trimbox = boxarray(trimBS); minbox = trimbox;}
  if (artBS.size()  > 0) { artbox = boxarray(artBS); minbox = artbox;}

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
