//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR main header file                                                    //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_H

//---------------------------------------------------------------------------//

#define PDFR_H

/* This is the final header in the daisy-chain of headers constituting the PDFR
 * program. Until now, the project was entirely written in C++11 with no
 * reliance on pre-compiled libraries.
 *
 * This header file describes the interface with R. It comprises functions in
 * the global namespace that return objects which can be directly loaded into
 * R. It does this using the Rcpp package, which abstracts and automates the
 * process of making the functions callable from within R. As a part of this
 * process, Rcpp writes a compilation unit called RcppExports.cpp, which, though
 * a proper and necessary part of the source files, is not hand-written or
 * commented.
 *
 * Rcpp "knows" which functions to export because of the modified comments which
 * come before each declaration.
 *
 * There are a few different items that can be returned to R. An 'xref' is an R
 * dataframe giving the full cross-reference table for the file. An 'object' is
 * one of the numbered pdf objects in the file, which is a named list with two
 * components - the "header", or object dictionary, as a named character vector,
 * and the "stream", which gives the unencrypted and decompressed contents of
 * an object's stream (if any) as a single string. We can also return a list of
 * the "glyphmaps" for each page. These are the Unicode mappings and widths for
 * each glyph used in the fonts on the page. The number of items in the list is
 * the same as the number of fonts on the page, and each item comprises a
 * dataframe of input characters, output characters and output widths.
 *
 * While these objects are all useful for debugging and exploring the structure
 * of a pdf file, the main purpose is to extract text, and this is done by
 * calling pdfpage or pdfdoc, to get a single page or the whole document.
 *
 * There are also some global functions in the implementation file which are
 * not declared in this header file. The reason is that they are just internal
 * helper functions for the exported functions.
 *
 * Note, to maintain type safety it is necessary to have two versions of each
 * function - one that takes the file name as input, and one that takes raw
 * data as input. Both versions are exported to R, and the R function that
 * calls them chooses which to use based on the type of data is passed in as
 * a parameter.
 */

#include<Rcpp.h>
#include "streams.h"
#include "line_grouper.h"

//---------------------------------------------------------------------------//
// Get xref. Returns a dataframe representing all of the cross-reference tables
// in a pdf stuck together. Each row represents an object, and gives the object
// number in the first column, its position or byte offset in the file in the
// second column, and the containing object in the third column if the object
// has come from an object stream. If the object has not come from an object
// stream, the third column will show 0. If the object has come from an object
// stream, it will show the number of that object in the third column but the
// second column will show 0. There are both filename and raw vector versions
// of the getxref function, which just need a valid pdf file, either as a path
// or as a vector of bytes.

// [[Rcpp::export(.get_xref)]]
Rcpp::DataFrame
GetXrefFromString(const std::string& file_name);

// [[Rcpp::export(.get_xrefraw)]]
Rcpp::DataFrame
GetXrefFromRaw(const std::vector<uint8_t>& raw_file);

//---------------------------------------------------------------------------//
// Returns a specific object from the pdf file. It gives both the dictionary
// and (uncompressed) stream for each object as a list. There are again versions
// that take a file path and a raw vector. The second parameter is the object
// number. Usually it is straightforward to navigate around the objects
// following references in the object dictionary.

// [[Rcpp::export(.get_obj)]]
Rcpp::List
GetObjectFromString(const std::string& file_name, int object_number);

// [[Rcpp::export(.get_objraw)]]
Rcpp::List
GetObjectFromRaw(const std::vector<uint8_t>& raw_file, int object_number);

//---------------------------------------------------------------------------//
// The main output of the program is a dataframe of each glyph with its
// position, size and font name. This is produced by the pdfpage function and
// is returned in a list along with the bounding box (to aid plotting) and the
// content string, or page description program, as a single string. This is
// mostly used for debugging. These two versions take a file path or raw data
// respectively, and both take a page number (one-indexed) as a second parameter

// [[Rcpp::export(.pdfpage)]]
Rcpp::List GetPdfPageFromString(const std::string& file_name,
                                int page_number,
                                bool each_glyph);

// [[Rcpp::export(.pdfpageraw)]]
Rcpp::List GetPdfPageFromRaw(const std::vector<uint8_t>& raw_file,
                             int page_number,
                             bool atoms);

//---------------------------------------------------------------------------//
// This function takes a file path and page number as parameters (note there is
// no raw version, as it is mostly used for debugging rather than a user tool).
// It returns a list of dataframes, one for each font used on the specified page
// and each of which has a row for each Unicode mapping and glyph width
// specified for that font.

// [[Rcpp::export(.getglyphmap)]]
Rcpp::DataFrame
GetGlyphMap(const std::string& file_name, int page_number);

//---------------------------------------------------------------------------//
// This function, used mainly for debugging, returns the uncompressed Postscript
// page description program for a given page. It is output as a single string
// which can be passed on to R for examination

// [[Rcpp::export(.pagestring)]]
std::string
GetPageStringFromString(const std::string& file_name, int page_number);

// [[Rcpp::export(.pagestringraw)]]
std::string
GetPageStringFromRaw(const std::vector<uint8_t>& raw_file, int page_number);

//---------------------------------------------------------------------------//
// These two versions of the pdfdoc function return R dataframes with all of
// the extracted text from an entire document.

// [[Rcpp::export(.pdfdoc)]]
Rcpp::DataFrame
GetPdfDocumentFromString(const std::string& file_name);

// [[Rcpp::export(.pdfdocraw)]]
Rcpp::DataFrame
GetPdfDocumentFromRaw(const std::vector<uint8_t>& file_name);

// [[Rcpp::export(.pdfboxesString)]]
Rcpp::DataFrame
GetPdfBoxesFromString(const std::string& file_name, int page_number);

// [[Rcpp::export(.pdfboxesRaw)]]
Rcpp::DataFrame
GetPdfBoxesFromRaw(const std::vector<uint8_t>& file_name, int page_number);

// [[Rcpp::export(.GetPaths)]]
Rcpp::List GetPaths(const std::string& file_name, int page_number);

//---------------------------------------------------------------------------//
// This function can be called from R to stop the underlying C++ code. This can
// be handy in profiling etc.

// [[Rcpp::export(.stopCpp)]]
void stopCpp();

#endif
