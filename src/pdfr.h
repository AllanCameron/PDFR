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

#include "GraphicsState.h"

//---------------------------------------------------------------------------//

void createpdf(const std::string& filename);
std::string getpagestring(page p);
std::string getPageString(const std::string& filename, int pagenum);

//---------------------------------------------------------------------------//

Rcpp::List PDFpage(document mypdf, page pg, int clump);

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

// [[Rcpp::export(.getglyphmap)]]
Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum);

#endif
