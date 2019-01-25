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

#include "pdfr.h"

//---------------------------------------------------------------------------//

Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum)
{
  document d = document(s);
  page p = page(&d, pagenum);
  std::vector<std::string> FontName;
  std::vector<uint16_t> codepoint, unicode, width;
  for(auto i : p.getFontNames())
  {
    font* loopfont = p.getFont(i);
    for(auto b : loopfont->getGlyphKeys())
    {
      FontName.push_back(i);
      codepoint.push_back(b);
      unicode.push_back(loopfont->mapRawChar({b}).at(0).first);
      width.push_back(loopfont->mapRawChar({b}).at(0).second);
    }
  }
  return Rcpp::DataFrame::create(Rcpp::Named("Font") = FontName,
                          Rcpp::Named("Codepoint") = codepoint,
                          Rcpp::Named("Unicode") = unicode,
                          Rcpp::Named("Width") = width);
}

//---------------------------------------------------------------------------//
// This is the function that actually creates the R list containing the
// necessary components to create a rendition of the page

Rcpp::List PDFpage(page* pg)
{
  GSoutput G = graphicsstate(pg).output();
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") = G.text,
                        Rcpp::Named("left") = G.left,
                        Rcpp::Named("bottom") = G.bottom,
                        Rcpp::Named("right") = G.right,
                        Rcpp::Named("font") = G.fonts,
                        Rcpp::Named("size") = G.size,
                        Rcpp::Named("width") = G.width,
                        Rcpp::Named("stringsAsFactors") = false);

  return Rcpp::List::create(
    Rcpp::Named("Box")        =  pg->getminbox(),
    Rcpp::Named("PageString") =  pg->pageContents(),
    Rcpp::Named("Elements")   =  db
  );
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame xrefcreator(std::string* fs)
{
  xref Xref = xref(fs);
  std::vector<int> ob, startb, inob;
  if(!Xref.getObjects().empty())
  {
    std::vector<int>&& allobs = Xref.getObjects();
    for(int j : allobs)
    {
      ob.push_back(j);
      startb.push_back(Xref.getStart(j));
      inob.push_back(Xref.inObject(j));
    }
  }
  return Rcpp::DataFrame::create(Rcpp::Named("Object") = ob,
                                 Rcpp::Named("StartByte") = startb,
                                 Rcpp::Named("InObject") = inob);
}

/*---------------------------------------------------------------------------*/

Rcpp::DataFrame get_xref(const std::string& filename)
{
  std::string fs = get_file(filename);
  return xrefcreator(&fs);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame get_xrefraw(const std::vector<uint8_t>& rawfile)
{
  std::string fs = bytestostring(rawfile);
  return xrefcreator(&fs);
}

//---------------------------------------------------------------------------//

Rcpp::List get_object(const std::string& filename, int object)
{
  document&& doc = document(filename);
  return Rcpp::List::create(
    Rcpp::Named("header") = doc.getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = doc.getobject(object)->getStream());
}

//---------------------------------------------------------------------------//

Rcpp::List get_objectraw(const std::vector<uint8_t>& rawfile, int object)
{
  document&& mydoc = document(rawfile);
  return Rcpp::List::create(
    Rcpp::Named("header") = mydoc.getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = mydoc.getobject(object)->getStream());
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpage(const std::string& filename, int pagenum)
{
  document myfile = document(filename);
  page p = page(&myfile, pagenum - 1);
  return PDFpage(&p);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum)
{
  document doc = document(rawfile);
  page p = page(&doc, pagenum - 1);
  return PDFpage(&p);
}

