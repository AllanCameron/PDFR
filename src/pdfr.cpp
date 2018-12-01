//---------------------------------------------------------------------------//
//                                                                           //
//                          PDFR main C++ file                               //
//                        (C) 2018 Allan Cameron                             //
//                                                                           //
//        This file loads all necessary C++ libraries and defines the        //
//        functions that will be exported to be made available in R.         //
//                                                                           //
//---------------------------------------------------------------------------//

//  Include Rcpp plugin for c++11
// [[Rcpp::plugins(cpp11)]]

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//



#include<Rcpp.h>  // Required for building C++ code into R package

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

// Include the required local files to build this library
// These have to be loaded in the correct order, as later files
// rely on functions defined in earlier files.

#include "pdfr.h"
#include "fileio.h"
#include "debugtools.h"
#include "dictionary.h"
#include "stringfunctions.h"
#include "document.h"
#include "miniz.h"
#include "crypto.h"
#include "streams.h"


//---------------------------------------------------------------------------//


// Functions to be exported to R
// The starting point for most of these functions is to create a C++ object
// of "document" class. From this, data from or about the pdf can be
// extracted for use in R functions.

//---------------------------------------------------------------------------//



/*---------------------------------------------------------------------------*/


Rcpp::List PDFpage(document mypdf, page pg)
{
  std::vector<EncMap> fontenc;
  for(auto i : pg.fontnames) fontenc.push_back(pg.fontmap[i].EncodingMap);
  return Rcpp::List::create(
    Rcpp::Named("Resources")  =  pg.resources.R_out(),
    Rcpp::Named("Fonts")      =  pg.fonts.R_out(),
    Rcpp::Named("Encodings")  =  fontenc,
    Rcpp::Named("Box")        =  pg.minbox,
    Rcpp::Named("Rotate")     =  pg.rotate,
    Rcpp::Named("PageString") =  pg.contentstring,
    Rcpp::Named("Elements")   =  GraphicsState(pg).db
  );
}

/*---------------------------------------------------------------------------*/

void createpdf(const std::string& filename){document res = document(filename);}

/*---------------------------------------------------------------------------*/

std::string getpagestring(page p){return p.contentstring;}

/*---------------------------------------------------------------------------*/

std::string getPageString(const std::string& filename, int pagenum)
{
  return getpagestring(document(filename).getPage(pagenum - 1));
}


// [[Rcpp::export]]
std::string byteStringToString(const std::string& s);

// [[Rcpp::export]]
std::string get_partial_file(const std::string& filename, long start, long stop);

// [[Rcpp::export]]
std::vector<unsigned char> bytesFromArray(const std::string& s);



// [[Rcpp::export]]
Rcpp::DataFrame get_xref(const std::string& filename)
  {
    document mydoc = document(filename);
    std::vector<int> ob, startb, stopb, inob;

    if(mydoc.Xref.getObjects().size() > 0)
      {
        for(int j : mydoc.Xref.getObjects())
          {
            ob.push_back(j);
            startb.push_back(mydoc.Xref.getStart(j));
            stopb.push_back(mydoc.Xref.getEnd(j));
            inob.push_back(mydoc.Xref.inObject(j));
          }
      }
    return Rcpp::DataFrame::create(Rcpp::Named("Object") = ob,
                                   Rcpp::Named("StartByte") = startb,
                                   Rcpp::Named("StopByte") = stopb,
                                   Rcpp::Named("InObject") = inob);
  }

//---------------------------------------------------------------------------//

//' get_object
//'
//' Show the key:value pairs in a PDF object dictionary
//'
//' @param filename the path to a valid PDF file.
//' @param o The object number, given as an integer.
//' @export
// [[Rcpp::export]]
Rcpp::List get_object(const std::string& filename, int o)
{
  document mydoc = document(filename);
  return Rcpp::List::create(
    Rcpp::Named("header") = mydoc.getobject(o).getDict().R_out(),
    Rcpp::Named("stream") = mydoc.getobject(o).getStream());
}

// [[Rcpp::export]]
std::string get_obj_stream(const std::string& filename, int o)
{
  document mydoc = document(filename);
  return mydoc.getobject(o).getStream();
}

//---------------------------------------------------------------------------//

//' pdfdoc
//'
//' List a PDF document's filename, catalogue dictionary, page dictionaries,
//' encryption status and file key (if encrypted)
//'
//' @param filepath the path to a valid PDF file.
//' @export
// [[Rcpp::export]]
Rcpp::List pdfdoc(const std::string & filepath)
  {
    document myfile = document(filepath);
    Rcpp::CharacterVector filename = Rcpp::wrap(myfile.file);
    return Rcpp::List::create(Rcpp::Named("file") = filename);
  }

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
Rcpp::List pdfpage(const std::string& filename, int pagenum)
  {
    document myfile = document(filename);
    return PDFpage(myfile, myfile.getPage(pagenum - 1));
  }

//---------------------------------------------------------------------------//

//' getPageString
//'
//' Returns a pdf page's Postscript program as a text string
//'
//' @param filename the path to a valid PDF file.
//' @param pagenum the page to extract
//' @export
// [[Rcpp::export]]
std::string getPageString(const std::string& filename, int pagenum);

//---------------------------------------------------------------------------//

// [[Rcpp::export]]
std::string carveout (const std::string& subject, const std::string& precarve,
                      const std::string& postcarve);

//---------------------------------------------------------------------------//

// [[Rcpp::export]]
std::vector<std::string>
  splitter(const std::string& subject, const std::string& matcher);

//---------------------------------------------------------------------------//

// [[Rcpp::export]]
std::vector<std::string>
  Rex (const std::vector<std::string>& strvec, std::string matcher);


//---------------------------------------------------------------------------//

// [[Rcpp::export]]
std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key);

//---------------------------------------------------------------------------//

// [[Rcpp::export]]
std::vector<uint8_t> md5(std::vector<uint8_t> input);

//---------------------------------------------------------------------------//

