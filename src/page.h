//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR page header file                                                    //
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

#ifndef PDFR_PAGE

//---------------------------------------------------------------------------//

#define PDFR_PAGE

/* This is the eighth in a sequence of daisy-chained headers that build up the
 * tools needed to read and parse the text content of pdfs. It comes after
 * font.h in the sequence and is the last step of constructing the logical
 * structure of pdf files.
 *
 * Each page object represents a page in the pdf document, taking as its
 * construction parameters just a document pointer and a page number.
 *
 * The page object acts as a container and organiser of the data required to
 * build a representation of the page. This includes the page dimensions,
 * the font objects used on the page, any xobjects, and the contents of the
 * page (as a page description program).
 *
 * The document and pagenumber are used to find the appropriate page header
 * dictionary. This gives the page dimensions, contents and resources (such
 * as fonts and xobjects). These items are pulled in from the relevant
 * pdf objects and processed to get the data members.
 *
 * The public interface needs to be able to extract data from the page as
 * required, and in particular the Tokenizer and GraphicsState classes need to
 * access the contents and fonts, respectively.
 */

#include "font.h"

//---------------------------------------------------------------------------//

class page
{
public:

  // constructor function
  page(document* pdfdoc, int pagenum);

  // public data members
  std::string contentstring;
  std::unordered_map<std::string, std::string> XObjects;
  std::vector<int> contents;
  std::vector<float> minbox;
  std::vector<std::string> fontnames;
  std::unordered_map<std::string, font> fontmap;

private:

  // private data members

  document*           d;
  int                 pagenumber;
  dictionary          header,
                      resources,
                      fonts;
  std::vector<float>  bleedbox,
                      cropbox,
                      mediabox,
                      trimbox,
                      artbox;
  std::string         xobjstring;
  double              rotate;

  // private methods

  void parseXObjStream();
  void boxes();
  void getHeader();
  void getResources();
  void getFonts();
  void getContents();
  std::vector<int> expandContents(std::vector<int> objnums);

};

//---------------------------------------------------------------------------//

#endif
