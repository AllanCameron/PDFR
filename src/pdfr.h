//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR main header file                                                    //
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


#ifndef PDFR_H
#define PDFR_H

#include<string>
#include<map>
#include<vector>
#include<algorithm>
#include<regex>
#include<chrono>
#include<cassert>
#include<iostream>
#include<fstream>
#include<thread>
#include<ratio>
#include<Rcpp.h>

typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef uint16_t Unicode;
typedef std::map<char, std::pair<uint16_t, int>> GlyphMap;

class object_class;
class page;
class xref;
class document;

#include "stringfunctions.h"
#include "streams.h"
#include "dictionary.h"
#include "font.h"
#include "object_class.h"
#include "xref.h"
#include "document.h"
#include "page.h"
#include "GraphicsState.h"

//---------------------------------------------------------------------------//

template< typename Mt, typename T >
std::vector<Mt> getKeys(std::map<Mt, T> Map)
{
  std::vector<Mt> keyvec;
  keyvec.reserve(Map.size());
  for(typename std::map<Mt, T>::iterator i = Map.begin(); i != Map.end(); i++)
  {
    keyvec.push_back(i->first);
  }
  return keyvec;
}

//---------------------------------------------------------------------------//

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//

inline void createpdf(const std::string& filename)
{
  document res = document(filename);
}

//---------------------------------------------------------------------------//

inline std::string getpagestring(page p){return p.contentstring;}

//---------------------------------------------------------------------------//

inline std::string getPageString(const std::string& filename, int pagenum)
{
  return getpagestring(document(filename).getPage(pagenum - 1));
}

//---------------------------------------------------------------------------//

Rcpp::List PDFpage(document mypdf, page pg, int clump);

//---------------------------------------------------------------------------//
//' rc4
//'
//' Performs an RC4 hash for a given key
//'
//' @param msg The message to be hashed as a raw vector
//' @param key the raw vector with which to hash
//' @export
// [[Rcpp::export]]
std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key);

//---------------------------------------------------------------------------//
//' md5
//'
//' returns an md5 hash of a given raw vector as a raw vector
//'
//' @param input A raw vector
//' @export
// [[Rcpp::export]]
std::vector<uint8_t> md5(std::vector<uint8_t> input);


// [[Rcpp::export(.get_xref)]]
Rcpp::DataFrame get_xref(const std::string& filename);

// [[Rcpp::export(.get_xrefraw)]]
Rcpp::DataFrame get_xrefraw(const std::vector<uint8_t>& rawfile);

// [[Rcpp::export(.get_obj)]]
Rcpp::List get_object(const std::string& filename, int o);

// [[Rcpp::export(.get_objraw)]]
Rcpp::List get_objectraw(const std::vector<uint8_t>& rawfile, int o);

// [[Rcpp::export(.pdfdoc)]]
Rcpp::List pdfdoc(const std::string & filepath);

// [[Rcpp::export(.pdfdocraw)]]
Rcpp::List pdfdocraw(const std::vector<uint8_t>& rawfile);

// [[Rcpp::export(.pdfpage)]]
Rcpp::List pdfpage(const std::string& filename, int pagenum);

// [[Rcpp::export(.pdfpageraw)]]
Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum);

// [[Rcpp::export]]
std::string testencoding(std::string s);
#endif
