//---------------------------------------------------------------------------//
//                                                                           //
//                         PDFR C++ header file                              //
//                        (C) 2018 Allan Cameron                             //
//                                                                           //
//        This file declares generic functions, classes and templates        //
//        that are used in creating the library's other *.cpp files          //
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

typedef std::map<uint16_t, std::string> EncMap;
typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef std::map<uint16_t, std::pair<std::string, int>> GlyphMap;

class object_class;
class page;
class xref;
class document;

#include "fileio.h"
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

std::string byteStringToString(const std::string& s);

//---------------------------------------------------------------------------//

std::string get_partial_file(const std::string& filename, long start, long stp);

//---------------------------------------------------------------------------//

std::string carveout (const std::string& subject, const std::string& precarve,
                      const std::string& postcarve);

//---------------------------------------------------------------------------//

std::vector<std::string>
  splitter(const std::string& subject, const std::string& matcher);


//---------------------------------------------------------------------------//
//' Rex
//'
//' Return substrings matching a given pattern
//'
//' @param strvec The string(s) to be searched
//' @param matcher The regular expression to be matched
//' @export
// [[Rcpp::export]]
std::vector<std::string>
  Rex (const std::vector<std::string>& strvec, std::string matcher);

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


//---------------------------------------------------------------------------//
//' Return the xref table from a PDF document
//'
//' Returns a data frame of objects and their byte positions in a pdf
//'
//' @param filename the path to a valid PDF file.
//' @export
// [[Rcpp::export]]
Rcpp::DataFrame get_xref(const std::string& filename);

//---------------------------------------------------------------------------//
//' get_object
//'
//' Show the key:value pairs in a PDF object dictionary
//'
//' @param filename the path to a valid PDF file.
//' @param o The object number, given as an integer.
//' @export
// [[Rcpp::export]]
Rcpp::List get_object(const std::string& filename, int o);

//---------------------------------------------------------------------------//
//' get_object_stream
//'
//' Get a pdf object's stream (if any)
//'
//' @param filename the path to a valid PDF file.
//' @param o The object number, given as an integer.
//' @export
// [[Rcpp::export]]
std::string get_obj_stream(const std::string& filename, int o);

//---------------------------------------------------------------------------//
//' pdfdoc
//'
//' List a PDF document's filename, catalogue dictionary, page dictionaries,
//' encryption status and file key (if encrypted)
//'
//' @param filepath the path to a valid PDF file.
//' @export
// [[Rcpp::export]]
Rcpp::List pdfdoc(const std::string & filepath);

//---------------------------------------------------------------------------//
//' pdfpage
//'
//' Returns a list comprising a page's fonts, its Postscript program as a text
//' string and its text elements as an R data frame
//'
//' @param filename the path to a valid PDF file.
//' @param pagenum the page to extract
//' @export
// [[Rcpp::export]]
Rcpp::List pdfpage(const std::string& filename, int pagenum);

//---------------------------------------------------------------------------//
//' pdfpageraw
//'
//' Returns a list comprising a page's fonts, its Postscript program as a text
//' string and its text elements as an R data frame
//'
//' @param rawfile raw pdf data
//' @param pagenum the page to extract
//' @export
// [[Rcpp::export]]
Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum);

#endif
