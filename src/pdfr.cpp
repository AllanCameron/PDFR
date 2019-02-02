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
#include<list>

//---------------------------------------------------------------------------//
// This exported function is used mainly for debugging the font reading
// process in PDFR. It returns a single dataframe, with a row for every
// Unicode mapping and glyph width for every font on the page (the font used is
// also given its own column. This export may be removed in production or moved
// to a debugging version

Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum)
{
  document d = document(s); // create the document from the given path string
  page p = page(&d, pagenum); // create the page from the document and pagenum
  std::vector<std::string> FontName;// container for the fonts used

  // containers for the dataframe columns
  std::vector<uint16_t> codepoint, unicode, width;

  for(auto i : p.getFontNames()) // for each font on the page...
  {
    font* loopfont = p.getFont(i); // get a pointer to the font
    for(auto b : loopfont->getGlyphKeys()) // for each code point in the font
    {
      FontName.push_back(i);    // copy the fontname
      codepoint.push_back(b);   // copy the input codepoint
      unicode.push_back(loopfont->mapRawChar({b}).at(0).first); // get Unicode
      width.push_back(loopfont->mapRawChar({b}).at(0).second);  // get width
    }
  }
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
  std::string fs = bytestostring(rawfile); // cast raw vector to string
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
  document&& doc = document(filename); // creates document from file path
  // Fill an Rcpp::List with the requested object and return
  return Rcpp::List::create(
    Rcpp::Named("header") = doc.getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = doc.getobject(object)->getStream());
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
  document&& mydoc = document(rawfile); // create document from raw data
    // Fill an Rcpp::List with the requested object and return
  return Rcpp::List::create(
    Rcpp::Named("header") = mydoc.getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = mydoc.getobject(object)->getStream());
}

//---------------------------------------------------------------------------//

Rcpp::List getgrid(page& p)
{
  graphic_state GS = graphic_state(&p);
  grid Grid = grid(GS);
  std::unordered_map<uint8_t, std::vector<GSrow>> gridout = Grid.output();
  std::vector<float> left;
  std::vector<float> right;
  std::vector<float> size;
  std::vector<float> bottom;
  std::vector<std::string> glyph;
  std::vector<std::string> font;
  for(uint8_t i = 0; i < 256; i++)
  {
    for(auto j : gridout[i])
    {
      if(!j.consumed)
      {
        glyph.push_back(utf(j.glyph));
        left.push_back(j.left);
        right.push_back(j.right);
        size.push_back(j.size);
        bottom.push_back(j.bottom);
        font.push_back(j.font);
      }
    }
    if(i == 255) break; // prevent overflow back to 0 and endless loop
  }
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") = glyph,
                        Rcpp::Named("left") = left,
                        Rcpp::Named("bottom") = bottom,
                        Rcpp::Named("right") = right,
                        Rcpp::Named("font") = font,
                        Rcpp::Named("size") = size,
                        Rcpp::Named("stringsAsFactors") = false);
  return Rcpp::List::create(Rcpp::Named("Box") = Grid.getBox(),
                            Rcpp::Named("Elements") = db);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpage(const std::string& s, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  document myfile = document(s); // document from file string
  page p = page(&myfile, pagenum - 1); // get page (convert to zero-indexed!)
  return getgrid(p);
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  document myfile = document(rawfile); // document from file string
  page p = page(&myfile, pagenum - 1); // get page (convert to zero-indexed!)
  return getgrid(p);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfdoc_common(document& myfile)
{
  size_t npages = myfile.pagecount();
  std::vector<float> left;
  std::vector<float> right;
  std::vector<float> size;
  std::vector<float> bottom;
  std::vector<std::string> glyph;
  std::vector<std::string> font;
  std::vector<int> pagenums;
  for(size_t pagenum = 0; pagenum < npages; pagenum++)
  {
    page p = page(&myfile, pagenum); // get page (convert to zero-indexed!)
    graphic_state GS = graphic_state(&p);
    grid Grid = grid(GS);
    std::unordered_map<uint8_t, std::vector<GSrow>> gridout = Grid.output();
    for(uint8_t i = 0; i < 256; i++)
    {
      for(auto j : gridout[i])
      {
        if(!j.consumed)
        {
          glyph.push_back(utf(j.glyph));
          left.push_back(j.left);
          right.push_back(j.right);
          size.push_back(j.size);
          bottom.push_back(j.bottom);
          font.push_back(j.font);
          pagenums.push_back(pagenum + 1);
        }
      }
      if(i == 255) break; // prevent overflow back to 0 and endless loop
    }
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


Rcpp::DataFrame pdfdoc(const std::string& s)
{
  document myfile = document(s); // document from file string
  return pdfdoc_common(myfile);
}

Rcpp::DataFrame pdfdocraw(const std::vector<uint8_t>& s)
{
  document myfile = document(s); // document from raw
  return pdfdoc_common(myfile);
}



//---------------------------------------------------------------------------//

std::string pagestring(const std::string& s, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  document myfile = document(s); // document from file string
  page p = page(&myfile, pagenum - 1); // get page (convert to zero-indexed!)
  std::string res = p.pageContents();
  return res;
}

//---------------------------------------------------------------------------//

std::string pagestringraw(const std::vector<uint8_t>& rawfile, int pagenum)
{
  if(pagenum < 1) Rcpp::stop("Invalid page number");
  document myfile = document(rawfile); // document from file string
  page p = page(&myfile, pagenum - 1); // get page (convert to zero-indexed!)
  std::string s = p.pageContents();
  return s;
}


