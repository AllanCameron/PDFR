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

using namespace std;

//---------------------------------------------------------------------------//
// The two get_page functions are helpers to take either a string representing
// the path to a valid pdf file, or the pdf file itself as a vector of bytes,
// as well as an integer page number, and returning a pointer to a newly created
// page object. This is a common task in the exported functions, so we need to
// seperate these functions out to reduce replication

shared_ptr<page> get_page(string file_name, int page_number)
{
  // Pages are numbered from 1. Any less than this should throw an error
  if(page_number < 1) Rcpp::stop("Invalid page number");

  // Create the document object
  auto document_ptr = make_shared<document>(file_name);

  // Create the page object and return it
  return make_shared<page>(document_ptr, page_number - 1);
}

shared_ptr<page> get_page(vector<uint8_t> raw_file, int page_number)
{
  // Pages are numbered from 1. Any less than this should throw an error
  if(page_number < 1) Rcpp::stop("Invalid page number");

  // Create the document object
  auto document_ptr = make_shared<document>(raw_file);

  // Create the page object and return it
  return make_shared<page>(document_ptr, page_number - 1);
}

//---------------------------------------------------------------------------//
// This exported function is used mainly for debugging the font reading
// process in PDFR. It returns a single dataframe, with a row for every
// Unicode mapping and glyph width for every font on the page (the font used is
// also given its own column. This export may be removed in production or moved
// to a debugging version

Rcpp::DataFrame getglyphmap(const string& file_name, int page_number)
{
  // Create document and page objects
  auto page_ptr = get_page(file_name, page_number);

  // Declare containers for the R dataframe columns
  vector<uint16_t> codepoint, unicode, width;
  vector<string> font_names;

  // For each font string on the page...
  for(auto& font_string : page_ptr->getFontNames())
  {
    // Get a pointer to the font
    shared_ptr<font>&& font_ptr = page_ptr->getFont(font_string);

    // for each code point in the font, copy the fontname and input codepoint
    for(auto& key : font_ptr->getGlyphKeys())
    {
      font_names.push_back(font_string);
      codepoint.push_back(key);
      unicode.push_back(font_ptr->mapRawChar({key}).at(0).first); // get Unicode
      width.push_back(font_ptr->mapRawChar({key}).at(0).second);  // get width
    }
  }

  // Clear the static font map
  page_ptr->clearFontMap();

  // put all the glyphs in a single dataframe and return
  return  Rcpp::DataFrame::create(Rcpp::Named("Font")      = font_names,
                                  Rcpp::Named("Codepoint") = codepoint,
                                  Rcpp::Named("Unicode")   = unicode,
                                  Rcpp::Named("Width")     = width);
}

//---------------------------------------------------------------------------//
// The xrefcreator function is not exported, but does most of the work of
// get_xref. It acts as a helper function and common final pathway for the
// raw and filepath versions of get_xref

Rcpp::DataFrame xrefcreator(shared_ptr<const string> file_string)
{
  // Create the xref from the given string pointer
  xref Xref(file_string);

  // Declare containers used to fill dataframe
  vector<int> object {}, start_byte {}, in_object {};

  // If the xref has entries
  if(!Xref.getObjects().empty())
  {
    // Get all of its object numbers
    vector<int>&& all_objects = Xref.getObjects();

    // Then for each listed object, store its number, start byte and container
    for(int object_num : all_objects)
    {
      object.push_back(object_num);
      start_byte.push_back(Xref.getStart(object_num));
      in_object.push_back(Xref.inObject(object_num));
    }
  }

  // Use the containers to fill the dataframe and return it to caller
  return Rcpp::DataFrame::create(Rcpp::Named("Object") = object,
                                 Rcpp::Named("StartByte") = start_byte,
                                 Rcpp::Named("InObject") = in_object);
}

/*---------------------------------------------------------------------------*/
// Exported filepath version of get_xref. Gets the file string by loading
// the file path into a single large string, a pointer to which is used to
// call the xrefcreator

Rcpp::DataFrame get_xref(const string& filename)
{
  // This one-liner gets the file string, builds the xref and turns it into an
  // R data frame
  return xrefcreator(make_shared<string>(get_file(filename)));
}

//---------------------------------------------------------------------------//
// Exported raw data version of get_xref. Gets the file string by casting the
// raw data vector as a single large string, a pointer to which is used to
// call the xrefcreator

Rcpp::DataFrame get_xrefraw(const vector<uint8_t>& raw_file)
{
  // Cast raw vector to string
  string file_string(raw_file.begin(), raw_file.end());

  // Create a dataframe representing the xref entry
  return xrefcreator(make_shared<string>(file_string));
}

//---------------------------------------------------------------------------//
// The file string version of get_object. It takes a file path as a parameter,
// from which it loads the entire file into a string to create a document.
// The second parameter is the actual pdf object number, which is found by
// the public getobject() method from document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

Rcpp::List get_object(const string& file_name, int object)
{
  // Create the document
  auto document_ptr = make_shared<document>(file_name);

  // Fill an Rcpp::List with the requested object's elements and return
  return Rcpp::List::create(
    Rcpp::Named("header") = document_ptr->getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = document_ptr->getobject(object)->getStream());
}

//---------------------------------------------------------------------------//
// The raw data version of get_object. It takes a raw vector as a parameter,
// which it recasts as a single large string to create a document.
// The second parameter is the actual pdf object number, which is found by
// the public getobject() method from document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

Rcpp::List get_objectraw(const vector<uint8_t>& raw_file, int object)
{
  // Create the document
  auto document_ptr = make_shared<document>(raw_file);

  // Fill an Rcpp::List with the requested object and return
  return Rcpp::List::create(
    Rcpp::Named("header") = document_ptr->getobject(object)->getDict().R_out(),
    Rcpp::Named("stream") = document_ptr->getobject(object)->getStream());
}

//---------------------------------------------------------------------------//
// This is the final common pathway for getting a dataframe of atomic glyphs
// from the parser. It packages the dataframe with a vector of page
// dimensions to allow plotting etc

Rcpp::List getatomic(shared_ptr<page> page_ptr)
{
  // Create new parser
  parser parser_object = parser(page_ptr);

  // Read page contents to parser
  tokenizer(page_ptr->pageContents(), &parser_object);

  // Obtain output from parser and transpose into a text table
  auto TR = parser_object.output();
  auto TT = TR.transpose();

  // Declare a container for utf-glyphs
  vector<string> glyph;

  // Convert Unicode to utf8
  for(auto& i : TT.text) glyph.push_back(utf({i}));

  // Ensure the static fontmap is cleared after use
  page_ptr->clearFontMap();

  // Now create the data frame
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") = glyph,
                        Rcpp::Named("left") = TT.left,
                        Rcpp::Named("bottom") = TT.bottom,
                        Rcpp::Named("right") = TT.right,
                        Rcpp::Named("font") = TT.fonts,
                        Rcpp::Named("size") = TT.size,
                        Rcpp::Named("stringsAsFactors") = false);

  // Return it as a list along with the page dimensions
  return Rcpp::List::create(
                        Rcpp::Named("Box") = page_ptr->getminbox().vector(),
                        Rcpp::Named("Elements") = move(db));
}

//---------------------------------------------------------------------------//

Rcpp::List getgrid(shared_ptr<page> page_ptr)
{
  // Create new parser
  parser parser_object(page_ptr);

  // Read page contents to parser
  tokenizer(page_ptr->pageContents(), &parser_object);

  // Group letters and words
  letter_grouper LG(parser_object.output());
  word_grouper WG(LG.output());

  // Arrange text into text boxes separated by whitespace
  Whitespace WS(WG.output());

  // Join lines of text within single text boxes
  line_grouper linegrouper(WS.output());

  std::vector<float> left, right, size, bottom;
  std::vector<std::string> glyph, font;
  std::vector<int> polygon;
  int polygonNumber = 0;
  auto LGO = linegrouper.output();

  for(auto& i : LGO)
  {
    for(auto& j : i)
    {
      if(!j->is_consumed())
      {
        left.push_back(j->get_left());
        right.push_back(j->get_right());
        size.push_back(j->get_size());
        bottom.push_back(j->get_bottom());
        glyph.push_back(utf(j->get_glyph()));
        font.push_back(j->get_font());
        polygon.push_back(polygonNumber);
      }
    }
    polygonNumber++;
  }
  page_ptr->clearFontMap();
  Rcpp::DataFrame db =  Rcpp::DataFrame::create(
                        Rcpp::Named("text") = std::move(glyph),
                        Rcpp::Named("left") = std::move(left),
                        Rcpp::Named("right") = std::move(right),
                        Rcpp::Named("bottom") = std::move(bottom),
                        Rcpp::Named("font") = std::move(font),
                        Rcpp::Named("size") = std::move(size),
                        Rcpp::Named("box") = std::move(polygon),
                        Rcpp::Named("stringsAsFactors") = false);
  return Rcpp::List::create(Rcpp::Named("Box") = page_ptr->getminbox().vector(),
                            Rcpp::Named("Elements") = std::move(db));
}

//---------------------------------------------------------------------------//

Rcpp::List pdfpage(const string& file_name, int page_number, bool each_glyph)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Process the page if requested
  if(!each_glyph) return getgrid(page_ptr);

  // Otherwise return a data frame of individual letters
  else return getatomic(page_ptr);
}

//---------------------------------------------------------------------------//

Rcpp::List
pdfpageraw(const vector<uint8_t>& raw_file, int page_number, bool each_glyph)
{
  // Create the page object
  auto page_ptr = get_page(raw_file, page_number);

  // Process the page if requested
  if(!each_glyph) return getgrid(page_ptr);

  // Otherwise return a data frame of individual letters
  else return getatomic(page_ptr);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfdoc_common(shared_ptr<document> document_ptr)
{
  auto pagenumbers = document_ptr->pagenums();
  auto npages = pagenumbers.size();
  vector<float> left, right, size, bottom;
  vector<string> glyph, font;
  vector<int> pagenums;
  for(size_t page_number = 0; page_number < npages; page_number++)
  {
    // Create a new page pbject
    auto page_ptr = make_shared<page>(document_ptr, page_number);

    // Create a new parser object
    parser parser_object(page_ptr);

    // Read page contents to parser object
    tokenizer(page_ptr->pageContents(), &parser_object);

    // Join individual letters into words
    letter_grouper LG(move(parser_object.output()));

    // Join individual words into lines or word clusters
    word_grouper WG(LG.output());

    // Get a text table from the output
    text_table gridout = WG.out();

    // Convert text from unicode to utf-8
    for(auto& i : gridout.text) glyph.push_back(utf(i));

    // Join current page's output to final data frame columns
    concat(left, gridout.left);
    concat(right, gridout.right);
    concat(bottom, gridout.bottom);
    concat(font, gridout.fonts);
    concat(size, gridout.size);

    // Add a page number entry for each text element
    while(pagenums.size() < size.size()) pagenums.push_back(page_number + 1);

    // Clear the static font map if we are on the last page.
    if(page_number == (npages - 1)) page_ptr->clearFontMap();
  }

  // Build and return an R data frame
  return  Rcpp::DataFrame::create(
          Rcpp::Named("text")             = glyph,
          Rcpp::Named("left")             = left,
          Rcpp::Named("right")            = right,
          Rcpp::Named("bottom")           = bottom,
          Rcpp::Named("font")             = font,
          Rcpp::Named("size")             = size,
          Rcpp::Named("page")             = pagenums,
          Rcpp::Named("stringsAsFactors") = false);
}

//---------------------------------------------------------------------------//
// This exported function takes a string representing a file path, creates a
// new document object and sends it to pdfdoc_common to create an R data frame
// containing all of the text elements in a document, including their location
// and page number

Rcpp::DataFrame pdfdoc(const string& file_name)
{
  // Simply create a new document pointer from the file name
  auto document_ptr = make_shared<document>(file_name);

  // Feed the document pointer to pdfdoc_common to get the whole document as
  // an R data frame
  return pdfdoc_common(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function takes a raw vector of bytes comprising a pdf file,
// creates a new document object and sends it to pdfdoc_common to create an R
// data frame containing all of the text elements in a document, including their
// location and page number.

Rcpp::DataFrame pdfdocraw(const vector<uint8_t>& raw_data)
{
  // Simply create a new document pointer from the raw data
  auto document_ptr = make_shared<document>(raw_data);

  // Feed the document pointer to pdfdoc_common to get the whole document as
  // an R data frame
  return pdfdoc_common(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a path name

string pagestring(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Clear the static font map
  page_ptr->clearFontMap();

  // Return a dereferenced pointer to the page contents
  return *(page_ptr->pageContents());
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a vector of bytes

string pagestringraw(const vector<uint8_t>& raw_file, int page_number)
{
  // Create the page object
  auto page_ptr = get_page(raw_file, page_number);

  // Clear the static font map
  page_ptr->clearFontMap();

  // Return a dereferenced pointer to the page contents
  return *(page_ptr->pageContents());
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdf_boxes(shared_ptr<page> page_ptr)
{
  // Create an empty parser object
  parser parser_object(page_ptr);

  // Read the page contents into the parser
  tokenizer(page_ptr->pageContents(), &parser_object);

  // Group individual letters into words
  letter_grouper LG(move(parser_object.output()));

  // Group words into lines or word clusters
  word_grouper WG(move(LG.output()));

  // Separate page into text boxes and white space
  Whitespace polygons(move(WG.output()));

  // This step outputs the data we need to create our data frame
  auto Poly = polygons.ws_box_out();

  // Declare holding variables used as columns in the returned dataframe
  vector<float> xmin, ymin, xmax, ymax;
  vector<int> groups;
  int group = 0;

  // Fill our holding vectors from the output of the page parsing algorithm
  for(auto bounding_box : Poly)
  {
    xmin.push_back(bounding_box.get_left());
    ymin.push_back(bounding_box.get_bottom());
    xmax.push_back(bounding_box.get_right());
    ymax.push_back(bounding_box.get_top());
    groups.push_back(group++);
  }

  // Clear the static font map
  page_ptr->clearFontMap();

  // Build and return an R dataframe
  return Rcpp::DataFrame::create(
    Rcpp::Named("xmin") = xmin,
    Rcpp::Named("ymin") = ymin,
    Rcpp::Named("xmax") = xmax,
    Rcpp::Named("ymax") = ymax,
    Rcpp::Named("box") = groups,
    Rcpp::Named("stringsAsFactors") = false
  );
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfboxesString(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Call on pdf_boxes to make our boxes dataframe
  return pdf_boxes(page_ptr);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame pdfboxesRaw(const vector<uint8_t>& raw_data, int page_number)
{
  // Create the page object
  auto page_ptr = get_page(raw_data, page_number);

  // Call on pdf_boxes to make our boxes dataframe
  return pdf_boxes(page_ptr);
}

//---------------------------------------------------------------------------//

#ifdef PROFILER_PDFR
void stopCpp(){TheNodeList::Instance().endprofiler(); }
#endif

#ifndef PROFILER_PDFR
void stopCpp(){}
#endif
