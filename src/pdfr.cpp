//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR main implementation file                                            //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "letter_grouper.h"
#include "word_grouper.h"
#include "whitespace.h"
#include "deflate.h"
#include "pdfr.h"

//---------------------------------------------------------------------------//

using namespace std;
using namespace Rcpp;

//---------------------------------------------------------------------------//
// The two get_page functions are helpers to take either a string representing
// the path to a valid pdf file, or the pdf file itself as a vector of bytes,
// as well as an integer page number, and returning a pointer to a newly created
// page object. This is a common task in the exported functions, so we need to
// seperate these functions out to reduce replication

shared_ptr<Page> get_page(string file_name, int page_number)
{
  // Pages are numbered from 1. Any less than this should throw an error
  if (page_number < 1) stop("Invalid page number");

  // Create the Document object
  auto document_ptr = make_shared<Document>(file_name);

  // Create the page object and return it
  return make_shared<Page>(document_ptr, page_number - 1);
}

//---------------------------------------------------------------------------//
// Raw version

shared_ptr<Page> get_page(vector<uint8_t> raw_file, int page_number)
{
  // Pages are numbered from 1. Any less than this should throw an error
  if (page_number < 1) stop("Invalid page number");

  // Create the Document object
  auto document_ptr = make_shared<Document>(raw_file);

  // Create the page object and return it
  return make_shared<Page>(document_ptr, page_number - 1);
}

//---------------------------------------------------------------------------//
// This exported function is used mainly for debugging the font reading
// process in PDFR. It returns a single dataframe, with a row for every
// Unicode mapping and glyph width for every font on the page (the font used is
// also given its own column. This export may be removed in production or moved
// to a debugging version

DataFrame get_glyph_map(const string& file_name, int page_number)
{
  // Create Document and page objects
  auto page_ptr = get_page(file_name, page_number);

  // Declare containers for the R dataframe columns
  vector<uint16_t> codepoint, unicode, width;
  vector<string> font_names;

  // For each font string on the page...
  for (auto& font_string : page_ptr->GetFontNames())
  {
    // Get a pointer to the font
    auto&& font_ptr = page_ptr->GetFont(font_string);

    // for each code point in the font, copy the fontname and input codepoint
    for (auto& key : font_ptr->GetGlyphKeys())
    {
      font_names.push_back(font_string);
      codepoint.push_back(key);
      unicode.push_back(font_ptr->MapRawChar({key}).at(0).first); // get Unicode
      width.push_back(font_ptr->MapRawChar({key}).at(0).second);  // get width
    }
  }

  // Clear the static font map
  page_ptr->ClearFontMap();

  // put all the glyphs in a single dataframe and return
  return  DataFrame::create(Named("Font")      = font_names,
                            Named("Codepoint") = codepoint,
                            Named("Unicode")   = unicode,
                            Named("Width")     = width);
}

//---------------------------------------------------------------------------//
// The xrefcreator function is not exported, but does most of the work of
// get_xref. It acts as a helper function and common final pathway for the
// raw and filepath versions of get_xref

DataFrame xrefcreator(shared_ptr<const string> file_string)
{
  // Create the xref from the given string pointer
  XRef Xref(file_string);

  // Declare containers used to fill dataframe
  vector<int> object {}, start_byte {}, holding_object {};

  // If the xref has entries
  if (!Xref.GetAllObjectNumbers().empty())
  {
    // Get all of its object numbers
    vector<int>&& all_objects = Xref.GetAllObjectNumbers();

    // Then for each listed object, store its number, start byte and container
    for (int object_num : all_objects)
    {
      object.push_back(object_num);
      start_byte.push_back(Xref.GetObjectStartByte(object_num));
      holding_object.push_back(Xref.GetHoldingNumberOf(object_num));
    }
  }

  // Use the containers to fill the dataframe and return it to caller
  return DataFrame::create(Named("Object")    = object,
                           Named("StartByte") = start_byte,
                           Named("InObject")  = holding_object);
}

/*---------------------------------------------------------------------------*/
// Exported filepath version of get_xref. Gets the file string by loading
// the file path into a single large string, a pointer to which is used to
// call the xrefcreator

DataFrame get_xref_from_string(const string& filename)
{
  // This one-liner gets the file string, builds the xref and turns it into an
  // R data frame
  return xrefcreator(make_shared<string>(GetFile(filename)));
}

//---------------------------------------------------------------------------//
// Exported raw data version of get_xref. Gets the file string by casting the
// raw data vector as a single large string, a pointer to which is used to
// call the xrefcreator

DataFrame get_xref_from_raw(const vector<uint8_t>& raw_file)
{
  // Cast raw vector to string
  string file_string(raw_file.begin(), raw_file.end());

  // Create a dataframe representing the xref entry
  return xrefcreator(make_shared<string>(file_string));
}

//---------------------------------------------------------------------------//
// The file string version of get_object. It takes a file path as a parameter,
// from which it loads the entire file into a string to create a Document.
// The second parameter is the actual pdf object number, which is found by
// the public GetObject() method from Document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

List get_object_from_string(const string& file_name, int object)
{
  // Create the Document
  auto document_ptr = make_shared<Document>(file_name);

  // Fill an List with the requested object's elements and return
  return List::create(
  Named("header") = document_ptr->GetObject(object)->GetDictionary().GetMap_(),
  Named("stream") = document_ptr->GetObject(object)->GetStream()
  );
}

//---------------------------------------------------------------------------//
// The raw data version of get_object(). It takes a raw vector as a parameter,
// which it recasts as a single large string to create a Document.
// The second parameter is the actual pdf object number, which is found by
// the public GetObject() method from Document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

List get_object_from_raw(const vector<uint8_t>& raw_file, int object)
{
  // Create the Document
  auto document_ptr = make_shared<Document>(raw_file);

  // Fill an List with the requested object and return
  return List::create(
    Named("header") =
      document_ptr->GetObject(object)->GetDictionary().GetMap_(),
    Named("stream") = document_ptr->GetObject(object)->GetStream());
}

//---------------------------------------------------------------------------//
// This is the final common pathway for getting a dataframe of atomic glyphs
// from the Parser. It packages the dataframe with a vector of page
// dimensions to allow plotting etc

List get_single_text_elements(shared_ptr<Page> t_page_ptr)
{
  // Create new Parser
  Parser parser_object = Parser(t_page_ptr);

  // Read page contents to Parser
  Tokenizer(t_page_ptr->GetPageContents(), &parser_object);

  // Obtain output from Parser and transpose into a text table
  auto text_box = parser_object.Output();
  TextTable table(*text_box);

  // Ensure the static fontmap is cleared after use
  t_page_ptr->ClearFontMap();

  // Now create the data frame
  DataFrame db =  DataFrame::create(Named("text")   = table.GetText(),
                                    Named("left")   = table.GetLefts(),
                                    Named("bottom") = table.GetBottoms(),
                                    Named("right")  = table.GetRights(),
                                    Named("top")    = table.GetTops(),
                                    Named("font")   = table.GetFontNames(),
                                    Named("size")   = table.GetSizes(),
                                    Named("stringsAsFactors") = false);

  // Return it as a list along with the page dimensions
  return List::create(Named("Box") = t_page_ptr->GetMinbox()->vector(),
                      Named("Elements") = move(db));
}

//---------------------------------------------------------------------------//

List get_text_boxes(shared_ptr<Page> page_ptr)
{
  // Create new Parser
  auto parser_object = new Parser(page_ptr);

  // Read page contents to Parser
  Tokenizer(page_ptr->GetPageContents(), parser_object);

  // Group letters and words
  auto grouped_letters = new LetterGrouper(parser_object->Output());
  delete parser_object;
  auto grouped_words = new WordGrouper(grouped_letters->Output());
  delete grouped_letters;
  auto WS = new Whitespace(grouped_words->Output());
  delete grouped_words;
  auto linegrouper = new LineGrouper(WS->Output());
  delete WS;
  auto text_table = TextTable(linegrouper->Output());
  delete linegrouper;
  page_ptr->ClearFontMap();
  DataFrame db =  DataFrame::create(
                    Named("text")             = move(text_table.GetText()),
                    Named("left")             = move(text_table.GetLefts()),
                    Named("right")            = move(text_table.GetRights()),
                    Named("bottom")           = move(text_table.GetBottoms()),
                    Named("top")              = move(text_table.GetTops()),
                    Named("font")             = move(text_table.GetFontNames()),
                    Named("size")             = move(text_table.GetSizes()),
                    Named("stringsAsFactors") = false);

return List::create(Named("Box") = page_ptr->GetMinbox()->vector(),
                    Named("Elements") = move(db));
}

//---------------------------------------------------------------------------//

List get_pdf_page_from_string (const string& file_name,
                                     int page_number,
                                     bool each_glyph)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Process the page if requested
  if (!each_glyph) return get_text_boxes(page_ptr);

  // Otherwise return a data frame of individual letters
  else return get_single_text_elements(page_ptr);
}

//---------------------------------------------------------------------------//

List get_pdf_page_from_raw(const vector<uint8_t>& raw_file,
                           int page_number,
                           bool each_glyph)
{
  // Create the page object
  auto page_ptr = get_page(raw_file, page_number);

  // Process the page if requested
  if (!each_glyph) return get_text_boxes(page_ptr);

  // Otherwise return a data frame of individual letters
  else return get_single_text_elements(page_ptr);
}

//---------------------------------------------------------------------------//

DataFrame pdfdoc_common(shared_ptr<Document> document_ptr)
{
  auto number_of_pages = document_ptr->GetPageObjectNumbers().size();
  vector<float> left, right, size, bottom;
  vector<string> glyph, font;
  vector<int> page_number_of_element;

  // Loop through each page, get its contents and add it to the output list
  for (size_t page_number = 0; page_number < number_of_pages; page_number++)
  {
    // Create a new page pbject
    auto page_ptr = make_shared<Page>(document_ptr, page_number);

    // Create a new Parser object
    Parser parser_object(page_ptr);

    // Read page contents to Parser object
    Tokenizer(page_ptr->GetPageContents(), &parser_object);

    // Join individual letters into words
    LetterGrouper grouped_letters(move(parser_object.Output()));

    // Join individual words into lines or word clusters
    WordGrouper grouped_words(grouped_letters.Output());

    // Get a text table from the output
    TextTable table = grouped_words.Out();;

    // Join current page's output to final data frame columns
    Concatenate(left,   table.GetLefts());
    Concatenate(right,  table.GetRights());
    Concatenate(bottom, table.GetBottoms());
    Concatenate(font,   table.GetFontNames());
    Concatenate(size,   table.GetSizes());
    Concatenate(glyph,  table.GetText());

    // Add a page number entry for each text element
    while (page_number_of_element.size() < glyph.size())
    {
      page_number_of_element.push_back(page_number + 1);
    }

    // Clear the static font map if we are on the last page.
    if (page_number == (number_of_pages - 1)) page_ptr->ClearFontMap();
  }

  // Build and return an R data frame
  return  DataFrame::create(Named("text")             = glyph,
                            Named("left")             = left,
                            Named("right")            = right,
                            Named("bottom")           = bottom,
                            Named("font")             = font,
                            Named("size")             = size,
                            Named("page")             = page_number_of_element,
                            Named("stringsAsFactors") = false);
}

//---------------------------------------------------------------------------//
// This exported function takes a string representing a file path, creates a
// new Document object and sends it to pdfdoc_common to create an R data frame
// containing all of the text elements in a Document, including their location
// and page number

DataFrame get_pdf_document_from_string(const string& file_name)
{
  // Simply create a new Document pointer from the file name
  auto document_ptr = make_shared<Document>(file_name);

  // Feed the Document pointer to pdfdoc_common to get the whole Document as
  // an R data frame
  return pdfdoc_common(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function takes a raw vector of bytes comprising a pdf file,
// creates a new Document object and sends it to pdfdoc_common to create an R
// data frame containing all of the text elements in a document, including their
// location and page number.

DataFrame get_pdf_document_from_raw(const vector<uint8_t>& raw_data)
{
  // Simply create a new Document pointer from the raw data
  auto document_ptr = make_shared<Document>(raw_data);

  // Feed the Document pointer to pdfdoc_common to get the whole document as
  // an R data frame
  return pdfdoc_common(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a path name

string get_page_string_from_string(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Clear the static font map
  page_ptr->ClearFontMap();

  // Return a dereferenced pointer to the page contents
  return *(page_ptr->GetPageContents());
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a vector of bytes

string get_page_string_from_raw(const vector<uint8_t>& raw_file,
                                int page_number)
{
  // Create the page object
  auto page_ptr = get_page(raw_file, page_number);

  // Clear the static font map
  page_ptr->ClearFontMap();

  // Return a dereferenced pointer to the page contents
  return *(page_ptr->GetPageContents());
}

//---------------------------------------------------------------------------//

DataFrame pdf_boxes(shared_ptr<Page> page_ptr)
{
  // Create an empty Parser object
  Parser parser_object(page_ptr);

  // Read the page contents into the Parser
  Tokenizer(page_ptr->GetPageContents(), &parser_object);

  // Group individual letters into words
  LetterGrouper grouped_letters(move(parser_object.Output()));

  // Group words into lines or word clusters
  WordGrouper grouped_words(move(grouped_letters.Output()));

  // Separate page into text boxes and white space
  Whitespace polygons(move(grouped_words.Output()));

  // This step outputs the data we need to create our data frame
  auto Poly = polygons.WSBoxOut();

  // Declare holding variables used as columns in the returned dataframe
  vector<float> xmin, ymin, xmax, ymax;
  vector<int> groups;
  int group = 0;

  // Fill our holding vectors from the output of the page parsing algorithm
  for (auto bounding_box : Poly)
  {
    xmin.push_back(bounding_box.GetLeft());
    ymin.push_back(bounding_box.GetBottom());
    xmax.push_back(bounding_box.GetRight());
    ymax.push_back(bounding_box.GetTop());
    groups.push_back(group++);
  }

  // Clear the static font map
  page_ptr->ClearFontMap();

  // Build and return an R dataframe
  return DataFrame::create( Named("xmin")             = xmin,
                            Named("ymin")             = ymin,
                            Named("xmax")             = xmax,
                            Named("ymax")             = ymax,
                            Named("box")              = groups,
                            Named("stringsAsFactors") = false
  );
}

//---------------------------------------------------------------------------//

DataFrame get_pdf_boxes_from_string(const string& file_name,
                                          int page_number)
{
  // Create the page object
  auto page_ptr = get_page(file_name, page_number);

  // Call on pdf_boxes to make our boxes dataframe
  return pdf_boxes(page_ptr);
}

//---------------------------------------------------------------------------//

DataFrame get_pdf_boxes_from_raw(const vector<uint8_t>& raw_data,
                                       int page_number)
{
  // Create the page object
  auto page_ptr = get_page(raw_data, page_number);

  // Call on pdf_boxes to make our boxes dataframe
  return pdf_boxes(page_ptr);
}

//---------------------------------------------------------------------------//

std::string Test_Stream(std::vector<uint8_t> raw_vector)
{
  std::string raw_string(raw_vector.begin(), raw_vector.end());
  Deflate test_raw(&raw_string);
  return test_raw.Output();
}

#ifdef PROFILER_PDFR
void stopCpp(){TheNodeList::Instance().endprofiler(); }
#endif

#ifndef PROFILER_PDFR
void stopCpp(){}
#endif
