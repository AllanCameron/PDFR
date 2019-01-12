//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR main implementation file                                            //
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

//---------------------------------------------------------------------------//
// Include Rcpp plugin for c++11:                                            //
// [[Rcpp::plugins(cpp11)]]                                                  //
//---------------------------------------------------------------------------//

#include<Rcpp.h>
#include<string>
#include<vector>
#include<unordered_map>
#include "pdfr.h"
#include "stringfunctions.h"
#include "adobetounicode.h"
#include "chartounicode.h"
#include "dictionary.h"
#include "xref.h"
#include "object_class.h"
#include "glyphwidths.h"
#include "encoding.h"
#include "font.h"
#include "document.h"
#include "page.h"
#include "tokenizer.h"
#include "GraphicsState.h"

//---------------------------------------------------------------------------//

void createpdf(const std::string& filename)
{
  document res = document(filename);
}

//---------------------------------------------------------------------------//

std::string getpagestring(page p){return p.contentstring;}

//---------------------------------------------------------------------------//

std::string getPageString(const std::string& filename, int pagenum)
{
  document d = document(filename);
  return getpagestring(page(&d, pagenum - 1));
}

Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum)
{
  document d = document(s);
  page p = page(&d, pagenum);
  std::vector<std::string> FontName;
  std::vector<uint16_t> codepoint, unicode, width;
  for(auto i : p.fontnames)
    for(auto b : getKeys(p.fontmap[i].glyphmap))
    {
      FontName.push_back(i);
      codepoint.push_back(b);
      unicode.push_back(p.fontmap[i].glyphmap[b].first);
      width.push_back(p.fontmap[i].glyphmap[b].second);
    }
  return Rcpp::DataFrame::create(Rcpp::Named("Font") = FontName,
                          Rcpp::Named("Codepoint") = codepoint,
                          Rcpp::Named("Unicode") = unicode,
                          Rcpp::Named("Width") = width);
}

//---------------------------------------------------------------------------//

Rcpp::List PDFpage(document mypdf, page pg)
{
  return Rcpp::List::create(
    Rcpp::Named("Box")        =  pg.minbox,
    Rcpp::Named("PageString") =  pg.contentstring,
    Rcpp::Named("Elements")   =  GraphicsState(&pg).db
  );
}

/*---------------------------------------------------------------------------*/

Rcpp::DataFrame get_xref(const std::string& filename)
{
  document&& mydoc = document(filename);
  std::vector<int> ob, startb, inob;
  if(!mydoc.Xref.getObjects().empty())
  {
    std::vector<int>&& allobs = mydoc.Xref.getObjects();
    for(int j : allobs)
    {
      ob.push_back(j);
      startb.push_back(mydoc.Xref.getStart(j));
      inob.push_back(mydoc.Xref.inObject(j));
    }
  }
  return Rcpp::DataFrame::create(Rcpp::Named("Object") = ob,
                                 Rcpp::Named("StartByte") = startb,
                                 Rcpp::Named("InObject") = inob);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame get_xrefraw(const std::vector<uint8_t>& rawfile)
{
  document&& mydoc = document(rawfile);
  std::vector<int> ob, startb, inob;
  if(!mydoc.Xref.getObjects().empty())
    for(int j : mydoc.Xref.getObjects())
    {
      ob.push_back(j);
      startb.push_back(mydoc.Xref.getStart(j));
      inob.push_back(mydoc.Xref.inObject(j));
    }
  return Rcpp::DataFrame::create(Rcpp::Named("Object") = ob,
                                 Rcpp::Named("StartByte") = startb,
                                 Rcpp::Named("InObject") = inob);
}

//---------------------------------------------------------------------------//

Rcpp::List get_object(const std::string& filename, int object)
{
  document&& doc = document(filename);
  return Rcpp::List::create(
    Rcpp::Named("header") = doc.getobject(object).getDict().R_out(),
    Rcpp::Named("stream") = doc.getobject(object).getStream());
}

//---------------------------------------------------------------------------//

Rcpp::List get_objectraw(const std::vector<uint8_t>& rawfile, int object)
{
  document&& mydoc = document(rawfile);
  return Rcpp::List::create(
    Rcpp::Named("header") = mydoc.getobject(object).getDict().R_out(),
    Rcpp::Named("stream") = mydoc.getobject(object).getStream());
}

//---------------------------------------------------------------------------//

Rcpp::List pdfdoc(const std::string & filepath)
{
  document&& myfile = document(filepath);
  Rcpp::CharacterVector filename = Rcpp::wrap(myfile.file);
  return Rcpp::List::create(Rcpp::Named("file") = filename);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfdocraw(const std::vector<uint8_t> & rawfile)
{
  document&& myfile = document(rawfile);
  Rcpp::CharacterVector filename = Rcpp::wrap({"From raw data"});
  return Rcpp::List::create(Rcpp::Named("file") = filename);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpage(const std::string& filename, int pagenum)
{
  document myfile = document(filename);
  page p = page(&myfile, pagenum - 1);
  return PDFpage(myfile, p);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum)
{
  document doc = document(rawfile);
  page p = page(&doc, pagenum - 1);
  return PDFpage(doc, p);
}
