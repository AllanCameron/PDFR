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

//---------------------------------------------------------------------------//

#define PDFR_H

/* This is the final header in the daisy-chain of headers constituting the PDFR
 * program. Until now, the project was entirely written in C++11 with no
 * reliance on pre-compiled libraries and just one small external library that
 * is included in the source files.
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
 * come before each declaration. This would work just as well if I left out
 * this header file and used the modified comments in the implementation file.
 * However, this way it is possible to see at a glance the functions that are
 * made available for export.
 *
 * There are a few different items that can be returned to R at present. An
 * 'xref' is an R dataframe giving the full cross-reference table for the file.
 * An 'object' is one of the numbered pdf objects in the file, which is a
 * named list with two components - the "header", or object dictionary, as a
 * named character vector, and the "stream", which gives the unencrypted and
 * decompressed contents of an object's stream (if any) as a single string. We
 * can also return a list of the "glyphmaps" for each page. These are the
 * Unicode mappings and widths for each glyph used in the fonts on the page.
 * The number of items in the list is the same as the number of fonts on the
 * page, and each item comprises a dataframe of input characters, output
 * characters and output widths.
 *
 * While these objects are all useful for debugging and exploring the structure
 * of a pdf file, the main purpose is to extract text, and this is done by
 * producing a "page" object. This returns a list with three named entries -
 * the "minbox", or smallest text bounding box for the page, the PageString,
 * which is a single string showing the "source file" of the page description
 * language from which the page was created, and the Elements dataframe, which
 * gives each glyph on the page along with its size, position and the font used
 * to create it, with a single row for each glyph.
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
#include "grid.h"

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
Rcpp::DataFrame get_xref(const std::string& filename);

// [[Rcpp::export(.get_xrefraw)]]
Rcpp::DataFrame get_xrefraw(const std::vector<uint8_t>& rawfile);

//---------------------------------------------------------------------------//
// Returns a specific object from the pdf file. It gives both the dictionary
// and (uncompressed) stream for each object as a list. There are again versions
// that take a file path and a raw vector. The second parameter is the object
// number. Usually it is straightforward to navigate around the objects
// following references in the object dictionary.

// [[Rcpp::export(.get_obj)]]
Rcpp::List get_object(const std::string& filename, int o);

// [[Rcpp::export(.get_objraw)]]
Rcpp::List get_objectraw(const std::vector<uint8_t>& rawfile, int o);

//---------------------------------------------------------------------------//
// The main output of the program is a dataframe of each glyph with its
// position, size and font name. This is produced by the pdfpage function and
// is returned in a list along with the bounding box (to aid plotting) and the
// content string, or page description program, as a single string. This is
// mostly used for debugging. These two versions take a file path or raw data
// respectively, and both take a page number (one-indexed) as a second parameter

// [[Rcpp::export(.pdfpage)]]
Rcpp::List pdfpage(const std::string& filename, int pagenum, bool g);

// [[Rcpp::export(.pdfpageraw)]]
Rcpp::List pdfpageraw(const std::vector<uint8_t>& rawfile, int pagenum, bool g);

//---------------------------------------------------------------------------//
// This function takes a file path and page number as parameters (note there is
// no raw version, as it is mostly used for debugging rather than a user tool).
// It returns a list of dataframes, one for each font used on the specified page
// and each of which has a row for each Unicode mapping and glyph width
// specified for that font.

// [[Rcpp::export(.getglyphmap)]]
Rcpp::DataFrame getglyphmap(const std::string& s, int pagenum);

//---------------------------------------------------------------------------//

// [[Rcpp::export(.pagestring)]]
std::string pagestring(const std::string& s, int pagenum);


//---------------------------------------------------------------------------//

// [[Rcpp::export(.pagestringraw)]]
std::string pagestringraw(const std::vector<uint8_t>& rawfile, int pagenum);

// [[Rcpp::export(.pdfdoc)]]
Rcpp::DataFrame pdfdoc(const std::string& s);

// [[Rcpp::export(.pdfdocraw)]]
Rcpp::DataFrame pdfdocraw(const std::vector<uint8_t>& s);

// [[Rcpp::export(.stopCpp)]]
void stopCpp();

#endif
