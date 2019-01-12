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

typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef uint16_t Unicode;
typedef uint16_t RawChar;
typedef std::unordered_map<RawChar, std::pair<Unicode, int>> GlyphMap;

class page;
class document;
//---------------------------------------------------------------------------//

template< typename Mt, typename T >
std::vector<Mt> getKeys(std::unordered_map<Mt, T>& Map)
{
  std::vector<Mt> keyvec;
  keyvec.reserve(Map.size());
  for(typename std::unordered_map<Mt, T>::iterator i = Map.begin(); i != Map.end(); i++)
    keyvec.push_back(i->first);
  return keyvec;
}

//---------------------------------------------------------------------------//

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//

template <typename T>
std::vector<int> order(const std::vector<T>& data)
{
  std::vector<int> index(data.size(), 0);
  int i = 0;
  for (auto &j : index) j = i++;

  sort(index.begin(), index.end(), [&](const T& a, const T& b)
  {
    return (data[a] < data[b]);
  });
  return index;
}

void createpdf(const std::string& filename);
std::string getpagestring(page p);
std::string getPageString(const std::string& filename, int pagenum);

//---------------------------------------------------------------------------//

Rcpp::List PDFpage(document mypdf, page pg, int clump);

//---------------------------------------------------------------------------//
//' rc4
//'
//' Performs an RC4 hash for a given key
//'
//' @param msg The message to be hashed as a raw std::vector
//' @param key the raw std::vector with which to hash
//' @export
// [[Rcpp::export]]
std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key);

//---------------------------------------------------------------------------//
//' md5
//'
//' returns an md5 hash of a given raw std::vector as a raw std::vector
//'
//' @param input A raw std::vector
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

// [[Rcpp::export(.getglyphmap)]]
Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum);

#endif
