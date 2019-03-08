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

#include "pdfr.h"
#include<list>

//---------------------------------------------------------------------------//
// This exported function is used mainly for debugging the font reading
// process in PDFR. It returns a single dataframe, with a row for every
// Unicode mapping and glyph width for every font on the page (the font used is
// also given its own column. This export may be removed in production or moved
// to a debugging version

Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum)
{
  std::shared_ptr<document> d = std::make_shared<document>(s);
  std::shared_ptr<page> p = std::make_shared<page>(d, pagenum - 1);
  std::vector<std::string> FontName;// container for the fonts used

  // containers for the dataframe columns
  std::vector<uint16_t> codepoint, unicode, width;

  for(auto& i : p->getFontNames()) // for each font on the page...
  {
    std::shared_ptr<font> loopfont = p->getFont(i); // get a pointer to the font
    for(auto& b : loopfont->getGlyphKeys()) // for each code point in the font
    {
      FontName.push_back(i);    // copy the fontname
      codepoint.push_back(b);   // copy the input codepoint
      unicode.push_back(loopfont->mapRawChar({b}).at(0).first); // get Unicode
      width.push_back(loopfont->mapRawChar({b}).at(0).second);  // get width
    }
  }
  p->clearFontMap();
  // put all the glyphs in a single dataframe and return
  return Rcpp::DataFrame::create(Rcpp::Named("Font") = FontName,
                          Rcpp::Named("Codepoint") = codepoint,
                          Rcpp::Named("Unicode") = unicode,
                          Rcpp::Named("Width") = width);
}

//---------------------------------------------------------------------------//
// The xrefcreator function is not exported, but does most of the work of
// get_xref. It acts as a helper function and common final pathway for the
// raw and filepath versions of get_xref

Rcpp::DataFrame xrefcreator(std::string* fs)
{
  xref Xref = xref(fs); // create the xref from the given string pointer

  // containers used to fill dataframe
  std::vector<int> ob, startb, inob;

  if(!Xref.getObjects().empty()) // if the xref has entries...
  {
    std::vector<int>&& allobs = Xref.getObjects(); // get all of its object #s
    for(int j : allobs) // then for each of them...
    {
      ob.push_back(j);                    // store the object number
      startb.push_back(Xref.getStart(j)); // store the byte offset
      inob.push_back(Xref.inObject(j));   // store the containing object
    }
  }
  // use the containers to fill the dataframe and return it to caller
  return Rcpp::DataFrame::create(Rcpp::Named("Object") = ob,
                                 Rcpp::Named("StartByte") = startb,
                                 Rcpp::Named("InObject") = inob);
}

/*---------------------------------------------------------------------------*/
// Exported filepath version of get_xref. Gets the file string by loading
// the file path into a single large string, a pointer to which is used to
// call the xrefcreator

Rcpp::DataFrame get_xref(const std::string& filename)
{
  std::string fs = get_file(filename); // loads file from path name
  return xrefcreator(&fs);             // returns the output of xrefcreator
}

//---------------------------------------------------------------------------//
// Exported raw data version of get_xref. Gets the file string by casting the
// raw data vector as a single large string, a pointer to which is used to
// call the xrefcreator

Rcpp::DataFrame get_xrefraw(const std::vector<uint8_t>& rawfile)
{
  std::string fs(rawfile.begin(), rawfile.end()); // cast raw vector to string
  return xrefcreator(&fs);  // return results of calling xrefcreator
}

//---------------------------------------------------------------------------//
// The file string version of get_object. It takes a file path as a parameter,
// from which it loads the entire file into a string to create a document.
// The second parameter is the actual pdf object number, which is found by
// the public getobject() method from document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

Rcpp::List get_object(const std::string& filename, int object)
{
  std::shared_ptr<document> doc = std::make_shared<document>(filename);
  // Fill an Rcpp::List with the requested object and return
  return Rcpp::List::create(
    Rcpp::Named("header") = doc->getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = doc->getobject(object)->getStream());
}

//---------------------------------------------------------------------------//
// The raw data version of get_object. It takes a raw vector as a parameter,
// which it recasts as a single large string to create a document.
// The second parameter is the actual pdf object number, which is found by
// the public getobject() method from document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

Rcpp::List get_objectraw(const std::vector<uint8_t>& rawfile, int object)
{
  std::shared_ptr<document> mydoc = std::make_shared<document>(rawfile);
    // Fill an Rcpp::List with the requested object and return
  return Rcpp::List::create(
    Rcpp::Named("header") = mydoc->getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = mydoc->getobject(object)->getStream());
}

//---------------------------------------------------------------------------//
// This is the final common pathway for getting a dataframe of atomic glyphs
// from the parser. It packages the dataframe with a vector of page
// dimensions to allow plotting etc

Rcpp::List getatomic(std::shared_ptr<page> p)
{
  parser G = parser(p); // New parser
  tokenizer(p->pageContents(), &G);   // Read page contents to graphic state
  auto GS = G.output();               // Obtain output from graphic state
  std::vector<std::string> glyph;     // Container for utf-glyphs
  for(auto& i : GS->text) glyph.push_back(utf({i})); // Unicode to utf8
  // Now create the data frame
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") = glyph,
                        Rcpp::Named("left") = GS->left,
                        Rcpp::Named("bottom") = GS->bottom,
                        Rcpp::Named("right") = GS->right,
                        Rcpp::Named("font") = GS->fonts,
                        Rcpp::Named("size") = GS->size,
                        Rcpp::Named("stringsAsFactors") = false);
  return Rcpp::List::create(Rcpp::Named("Box") = G.getminbox(),
                            Rcpp::Named("Elements") = db);
}

//---------------------------------------------------------------------------//

Rcpp::List getgrid(std::shared_ptr<page> p)
{
  parser GS = parser(p);
  tokenizer(p->pageContents(), &GS);
  letter_grouper Grid = letter_grouper(GS);
  gridoutput gridout = word_grouper(&Grid).out();
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") =   std::move(gridout.text),
                        Rcpp::Named("left") =   std::move(gridout.left),
                        Rcpp::Named("bottom") = std::move(gridout.bottom),
                        Rcpp::Named("right") =  std::move(gridout.right),
                        Rcpp::Named("font") =   std::move(gridout.font),
                        Rcpp::Named("size") =   std::move(gridout.size),
                        Rcpp::Named("stringsAsFactors") = false);
  return Rcpp::List::create(Rcpp::Named("Box") = Grid.getBox(),
                            Rcpp::Named("Elements") = db);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpage(const std::string& s, int pagenum, bool g)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum - 1);
  Rcpp::List res;
  if(!g) res = getgrid(p);
  else res = getatomic(p);
  p->clearFontMap();
  return res;
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum, bool g)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(rawfile);
  std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum - 1);
  Rcpp::List res;
  if(!g) res = getgrid(p);
  else res = getatomic(p);
  p->clearFontMap();
  return res;
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfdoc_common(std::shared_ptr<document> myfile)
{
  size_t npages = myfile->pagecount();
  std::vector<float> left, right, size, bottom;
  std::vector<std::string> glyph, font;
  std::vector<int> pagenums;
  for(size_t pagenum = 0; pagenum < npages; pagenum++)
  {
    std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum);
    parser GS = parser(p);
    tokenizer(p->pageContents(), &GS);
    letter_grouper GR = letter_grouper(GS);
    gridoutput gridout = word_grouper(&GR).out();
    concat(glyph, gridout.text);
    concat(left, gridout.left);
    concat(right, gridout.right);
    concat(bottom, gridout.bottom);
    concat(font, gridout.font);
    concat(size, gridout.size);
    while(pagenums.size() < size.size()) pagenums.push_back(pagenum + 1);
    if(pagenum == (npages - 1)) p->clearFontMap();
  }

  return Rcpp::DataFrame::create(
                        Rcpp::Named("text") = glyph,
                        Rcpp::Named("left") = left,
                        Rcpp::Named("right") = right,
                        Rcpp::Named("bottom") = bottom,
                        Rcpp::Named("font") = font,
                        Rcpp::Named("size") = size,
                        Rcpp::Named("page") = pagenums,
                        Rcpp::Named("stringsAsFactors") = false);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfdoc(const std::string& s)
{
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  return pdfdoc_common(myfile);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfdocraw(const std::vector<uint8_t>& s)
{
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  return pdfdoc_common(myfile);
}

//---------------------------------------------------------------------------//

std::string pagestring(const std::string& s, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum - 1);
  std::string res = p->pageContents();
  p->clearFontMap();
  return res;
}

//---------------------------------------------------------------------------//

std::string pagestringraw(const std::vector<uint8_t>& rawfile, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(rawfile);
  std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum - 1);
  std::string s = p->pageContents();
  p->clearFontMap();
  return s;
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfboxescommon(std::shared_ptr<document> myfile, int pagenum)
{
  std::shared_ptr<page> p = std::make_shared<page>(myfile, pagenum);
  parser G = parser(p); // New parser
  tokenizer(p->pageContents(), &G);   // Read page contents to graphic state
  letter_grouper GR = letter_grouper(G);
  word_grouper WG = word_grouper(&GR);
  Whitespace polygons(WG);
  auto Poly = polygons.ws_box_out();
  std::vector<float> xmin, ymin, xmax, ymax;
  std::vector<int> groups;
  int group = 0;
    for(auto j : Poly)
    {
      xmin.push_back(j.left);
      ymin.push_back(j.bottom);
      xmax.push_back(j.right);
      ymax.push_back(j.top);
      groups.push_back(group++);
    }
  p->clearFontMap();
  return Rcpp::DataFrame::create(
    Rcpp::Named("xmin") = xmin,
    Rcpp::Named("ymin") = ymin,
    Rcpp::Named("xmax") = xmax,
    Rcpp::Named("ymax") = ymax,
    Rcpp::Named("box") = groups,
    Rcpp::Named("stringsAsFactors") = false
  );
}


Rcpp::DataFrame pdfboxesString(const std::string& s, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  return pdfboxescommon(myfile, pagenum - 1);
}

Rcpp::DataFrame pdfboxesRaw(const std::vector<uint8_t>& s, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  std::shared_ptr<document> myfile = std::make_shared<document>(s);
  return pdfboxescommon(myfile, pagenum - 1);
}



#ifdef PROFILER_PDFR
void stopCpp(){TheNodeList::Instance().endprofiler(); }
#endif

#ifndef PROFILER_PDFR
void stopCpp(){}
#endif
