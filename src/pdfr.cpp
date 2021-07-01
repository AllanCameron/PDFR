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

#include "utilities.h"
#include "dictionary.h"
#include "object_class.h"
#include "deflate.h"
#include "document.h"
#include "page.h"
#include "tokenizer.h"
#include "letter_grouper.h"
#include "word_grouper.h"
#include "whitespace.h"
#include "line_grouper.h"
#include "truetype.h"
#include "pdfr.h"
#include <iomanip>

//---------------------------------------------------------------------------//

using namespace std;
using namespace Rcpp;

//---------------------------------------------------------------------------//
// The two GetPage functions are helpers to take either a string representing
// the path to a valid pdf file, or the pdf file itself as a vector of bytes,
// as well as an integer page number, and returning a pointer to a newly created
// page object. This is a common task in the exported functions, so we need to
// seperate these functions out to reduce replication

shared_ptr<Page> GetPage(string file_name, int page_number)
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

shared_ptr<Page> GetPage(vector<uint8_t> raw_file, int page_number)
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

DataFrame GetGlyphMap(const string& file_name, int page_number)
{
  // Create Document and page objects
  auto page_ptr = GetPage(file_name, page_number);

  // Declare containers for the R dataframe columns
  vector<uint16_t> codepoint, unicode;
  vector<float> width;
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
// The XrefCreator function is not exported, but does most of the work of
// get_xref. It acts as a helper function and common final pathway for the
// raw and filepath versions of get_xref

DataFrame XrefCreator(shared_ptr<const string> file_string)
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
// call the XrefCreator

DataFrame GetXrefFromString(const string& filename)
{
  // This one-liner gets the file string, builds the xref and turns it into an
  // R data frame
  return XrefCreator(make_shared<string>(GetFile(filename)));
}

//---------------------------------------------------------------------------//
// Exported raw data version of get_xref. Gets the file string by casting the
// raw data vector as a single large string, a pointer to which is used to
// call the XrefCreator

DataFrame GetXrefFromRaw(const vector<uint8_t>& raw_file)
{
  // Cast raw vector to string
  string file_string(raw_file.begin(), raw_file.end());

  // Create a dataframe representing the xref entry
  return XrefCreator(make_shared<string>(file_string));
}

//---------------------------------------------------------------------------//
// The file string version of get_object. It takes a file path as a parameter,
// from which it loads the entire file into a string to create a Document.
// The second parameter is the actual pdf object number, which is found by
// the public GetObject() method from Document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

List GetObjectFromString(const string& file_name, int object)
{
  // Create the Document
  auto doc_ptr = make_shared<Document>(file_name);

  auto as_string = doc_ptr->GetObject(object)->GetStream();
  std::vector<uint8_t> as_raw(as_string.begin(), as_string.end());
  // Fill an List with the requested object and return
  if(!IsAscii(as_string) && !as_raw.empty())
  {
  return List::create(
    Named("header") = doc_ptr->GetObject(object)->GetDictionary().GetMap(),
    Named("stream") = as_raw);
  } else {
  return List::create(
    Named("header") = doc_ptr->GetObject(object)->GetDictionary().GetMap(),
    Named("stream") = as_string);
  }
}

//---------------------------------------------------------------------------//
// The raw data version of get_object(). It takes a raw vector as a parameter,
// which it recasts as a single large string to create a Document.
// The second parameter is the actual pdf object number, which is found by
// the public GetObject() method from Document class. It returns a list
// of two named values - the dictionary, as a named character vector, and the
// decrypted / decompressed stream as a single string

List GetObjectFromRaw(const vector<uint8_t>& raw_file, int object)
{
  // Create the Document
  auto doc_ptr = make_shared<Document>(raw_file);
  auto as_string = doc_ptr->GetObject(object)->GetStream();
  std::vector<uint8_t> as_raw(as_string.begin(), as_string.end());
  // Fill an List with the requested object and return
  if(!IsAscii(as_string) && !as_raw.empty())
  {
  return List::create(
    Named("header") = doc_ptr->GetObject(object)->GetDictionary().GetMap(),
    Named("stream") = as_raw);
  } else {
  return List::create(
    Named("header") = doc_ptr->GetObject(object)->GetDictionary().GetMap(),
    Named("stream") = as_string);
  }
}

//---------------------------------------------------------------------------//
// This is the final common pathway for getting a dataframe of atomic glyphs
// from the Parser. It packages the dataframe with a vector of page
// dimensions to allow plotting etc

List GetSingleTextElements(shared_ptr<Page> page_ptr)
{
  // Create new Parser
  Parser parser_object = Parser(page_ptr);

  // Read page contents to Parser
  Tokenizer(page_ptr->GetPageContents(), &parser_object);

  // Obtain output from Parser and transpose into a text table
  auto text_box = parser_object.Output();
  TextTable table(*text_box);

  // Ensure the static fontmap is cleared after use
  page_ptr->ClearFontMap();

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
  return List::create(Named("Box") = page_ptr->GetMinbox()->Vector(),
                      Named("Elements") = move(db));
}

//---------------------------------------------------------------------------//

List GetTextBoxes(shared_ptr<Page> page_ptr)
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

return List::create(Named("Box") = page_ptr->GetMinbox()->Vector(),
                    Named("Elements") = move(db));
}

//---------------------------------------------------------------------------//

List GetPdfPageFromString (const string& file_name,
                                     int page_number,
                                     bool each_glyph)
{
  // Create the page object
  auto page_ptr = GetPage(file_name, page_number);

  // Process the page if requested
  if (!each_glyph) return GetTextBoxes(page_ptr);

  // Otherwise return a data frame of individual letters
  else return GetSingleTextElements(page_ptr);
}

//---------------------------------------------------------------------------//

List GetPdfPageFromRaw(const vector<uint8_t>& raw_file,
                           int page_number,
                           bool each_glyph)
{
  // Create the page object
  auto page_ptr = GetPage(raw_file, page_number);

  // Process the page if requested
  if (!each_glyph) return GetTextBoxes(page_ptr);

  // Otherwise return a data frame of individual letters
  else return GetSingleTextElements(page_ptr);
}

//---------------------------------------------------------------------------//

DataFrame PdfDocCommon(shared_ptr<Document> document_ptr)
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
// new Document object and sends it to PdfDocCommon to create an R data frame
// containing all of the text elements in a Document, including their location
// and page number

DataFrame GetPdfDocumentFromString(const string& file_name)
{
  // Simply create a new Document pointer from the file name
  auto document_ptr = make_shared<Document>(file_name);

  // Feed the Document pointer to PdfDocCommon to get the whole Document as
  // an R data frame
  return PdfDocCommon(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function takes a raw vector of bytes comprising a pdf file,
// creates a new Document object and sends it to PdfDocCommon to create an R
// data frame containing all of the text elements in a document, including their
// location and page number.

DataFrame GetPdfDocumentFromRaw(const vector<uint8_t>& raw_data)
{
  // Simply create a new Document pointer from the raw data
  auto document_ptr = make_shared<Document>(raw_data);

  // Feed the Document pointer to PdfDocCommon to get the whole document as
  // an R data frame
  return PdfDocCommon(document_ptr);
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a path name

string GetPageStringFromString(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(file_name, page_number);

  // Clear the static font map
  page_ptr->ClearFontMap();

  // Return a dereferenced pointer to the page contents
  return (page_ptr->GetPageContents());
}

//---------------------------------------------------------------------------//
// This exported function allows the page description program to be output to
// the R console, given the pdf file as a vector of bytes

string GetPageStringFromRaw(const vector<uint8_t>& raw_file,
                                int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(raw_file, page_number);

  // Clear the static font map
  page_ptr->ClearFontMap();

  // Return a dereferenced pointer to the page contents
  return (page_ptr->GetPageContents());
}

//---------------------------------------------------------------------------//

DataFrame PdfBoxes(shared_ptr<Page> page_ptr)
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

List GetPaths(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(file_name, page_number);

  // Create an empty Parser object
  Parser parser_object(page_ptr);

  // Read the page contents into the Parser
  Tokenizer(page_ptr->GetPageContents(), &parser_object);

  std::vector<std::shared_ptr<GraphicObject>> boxes = parser_object.GetGraphics();

  auto result = List::create();

  for(size_t i = 0; i < boxes.size(); i++)
  {
    result.push_back(List::create(Named("X") = boxes[i]->GetX(),
                                  Named("Y") = boxes[i]->GetY(),
                                  Named("filled") = boxes[i]->IsFilled(),
                                  Named("stroked") = boxes[i]->IsStroked(),
                                  Named("colour") = boxes[i]->GetColour(),
                                  Named("fill") = boxes[i]->GetFillColour(),
                                  Named("size") = boxes[i]->GetLineWidth(),
                                  Named("text") = boxes[i]->GetText()));
  }

  // Build and return an R dataframe
  return result;
}

//---------------------------------------------------------------------------//

DataFrame GetPdfBoxesFromString(const string& file_name,
                                          int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(file_name, page_number);

  // Call on PdfBoxes to make our boxes dataframe
  return PdfBoxes(page_ptr);
}

//---------------------------------------------------------------------------//

DataFrame GetPdfBoxesFromRaw(const vector<uint8_t>& raw_data,
                                       int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(raw_data, page_number);

  // Call on PdfBoxes to make our boxes dataframe
  return PdfBoxes(page_ptr);
}

//---------------------------------------------------------------------------//
// A helper function to create npc units for drawing grobs

NumericVector npc(NumericVector input) {
  IntegerVector npc_ = IntegerVector::create(0);
  CharacterVector cv = CharacterVector::create("simpleUnit", "unit", "unit_v2");
  input.attr("unit") = npc_;
  input.attr("class") = cv;

  return input;
}

std::string rgb(std::vector<float> col) {
  if(col.size() != 3) {
    Rcpp::stop("rgb function needs a length-3 vector<float>");
  }
  float r = col[0];
  float g = col[1];
  float b = col[2];

  if(r > 1.0 || g > 1.0 || b > 1.0) {
    Rcpp::stop("Colour values must be in range 0 to 1");
  }

  int red = 255 * r;
  int green = 255 * g;
  int blue = 255 * b;

  std::stringstream stream;
  stream << "#" << std::setw(2) << std::setfill('0') << std::hex << red <<
                   std::setw(2) << std::setfill('0') << std::hex << green <<
                   std::setw(2) << std::setfill('0') << std::hex << blue;

  std::string result( stream.str() );
  return result;

}

//---------------------------------------------------------------------------//
// Converts a GraphicObject to a grid::grob in R

List MakeGrobFromGraphics(std::shared_ptr<Page> page,
                          std::shared_ptr<GraphicObject> go,
                          std::string grob_name)
{
  std::shared_ptr<Box> minbox = page->GetMinbox();
  float page_width = minbox->Width();
  float page_height = minbox->Height();

  CharacterVector fill = CharacterVector::create(R_NaString);
  CharacterVector col = CharacterVector::create(R_NaString);
  float lwd = std::abs(go->GetLineWidth());

  if(lwd > 2) lwd = 2;

  if(go->IsFilled()) fill = CharacterVector::create(rgb(go->GetFillColour()));
  if(go->IsStroked() || go->GetText() != "") col = rgb(go->GetColour());

  List gp = List::create(Named("fill") = fill,
                         Named("lwd")  = lwd,
                         Named("col")  = col,
                         Named("fontsize") = go->GetFontSize());

  gp.attr("class") = CharacterVector::create("gpar");

  CharacterVector classes;

  std::vector<float> x_float = go->GetX();
  std::vector<float> y_float = go->GetY();

  if(x_float.size() == 0) {
    classes = CharacterVector::create("null", "grob", "gDesc");
    List nullgrob = List::create(Named("x") = npc(0.5),
                                 Named("y") = npc(0.5),
                                 Named("name") = grob_name,
                                 Named("gp") = R_NilValue,
                                 Named("vp") = R_NilValue);
    nullgrob.attr("class") = classes;
    return nullgrob;
  }

  for(auto i = x_float.begin(); i != x_float.end(); ++i) *i = *i / page_width;
  for(auto i = y_float.begin(); i != y_float.end(); ++i) *i = *i / page_height;

  List grob = List::create(Named("x") = npc(wrap(x_float)),
                           Named("y") = npc(wrap(y_float)),
                           Named("name") = grob_name,
                           Named("gp") = gp,
                           Named("vp") = R_NilValue);

  if(go->GetText() == "" && !go->IsFilled()) {
    classes = CharacterVector::create("polyline", "grob", "gDesc");
    grob.push_back(R_NilValue, "arrow");
    grob.push_back(go->GetSubpaths(), "id");
    grob.push_back(R_NilValue, "id.lengths");
  }

  if(go->GetText() == "" && go->IsFilled()) {
    classes = CharacterVector::create("polygon", "grob", "gDesc");
    grob.push_back(go->GetSubpaths(), "id");
    grob.push_back(R_NilValue, "id.lengths");
  }

  if(go->GetText() != "") {
    classes = CharacterVector::create("text", "grob", "gDesc");
    grob.push_back(go->GetText(), "label");
    grob.push_back(CharacterVector::create("centre"), "just");
    grob.push_back(NumericVector::create(0), "hjust");
    grob.push_back(NumericVector::create(0), "vjust");
    grob.push_back(NumericVector::create(0), "rot");
    grob.push_back(LogicalVector::create(false), "check.overlap");
  }

  grob.attr("class") = classes;

  return grob;
}



//---------------------------------------------------------------------------//
// Outputs a page's graphical content as grobs

List GetGrobs(const string& file_name, int page_number)
{
  // Create the page object
  auto page_ptr = GetPage(file_name, page_number);

  // Create an empty Parser object
  Parser parser_object(page_ptr);

  // Read the page contents into the Parser
  Tokenizer(page_ptr->GetPageContents(), &parser_object);

  std::vector<std::shared_ptr<GraphicObject>> go_s = parser_object.GetGraphics();

  auto result = List::create();

  for(size_t i = 0; i < go_s.size(); i++)
  {
    std::string n = "GRID.Shape." + std::to_string(i);
    result.push_back(MakeGrobFromGraphics(page_ptr, go_s[i], n));
  }

  page_ptr->ClearFontMap();

  // Build and return an R dataframe
  return result;
}

/*---------------------------------------------------------------------------*/

DataFrame ReadFontTable(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  auto  table_of_tables = ttf.GetTable();

  CharacterVector table;
  NumericVector   offset;
  NumericVector   checksum;
  NumericVector   length;

  for(size_t i = 0; i < table_of_tables.size(); i++)
  {
    table.push_back(table_of_tables[i].table_name_);
    offset.push_back(table_of_tables[i].offset_);
    checksum.push_back(table_of_tables[i].checksum_);
    length.push_back(table_of_tables[i].length_);
  }

  DataFrame result = Rcpp::DataFrame::create(
    Named("table")    = table,
    Named("offset")   = offset,
    Named("length")   = length,
    Named("checksum") = checksum);

  return result;
}

/*---------------------------------------------------------------------------*/

List GetFontFileHeader(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);

  HeadTable head = ttf.GetHead();

  return Rcpp::List::create(
    Rcpp::Named("checksumAdjustment") = head.checksumAdjustment,
    Rcpp::Named("created") = head.created,
    Rcpp::Named("flags") = head.flags,
    Rcpp::Named("fontDirectionHint") = head.fontDirectionHint,
    Rcpp::Named("fontRevision") = head.fontRevision,
    Rcpp::Named("glyphDataFormat") = head.glyphDataFormat,
    Rcpp::Named("indexToLocFormat") = head.indexToLocFormat,
    Rcpp::Named("lowestRecPPEM") = head.lowestRecPPEM,
    Rcpp::Named("macStyle") = head.macStyle,
    Rcpp::Named("magicNumber") = head.magicNumber,
    Rcpp::Named("modified") = head.modified,
    Rcpp::Named("unitsPerEm") = head.unitsPerEm,
    Rcpp::Named("version") = head.version,
    Rcpp::Named("xMax") = head.xMax,
    Rcpp::Named("xMin") = head.xMin,
    Rcpp::Named("yMax") = head.yMax,
    Rcpp::Named("yMin") = head.yMin);
}

/*---------------------------------------------------------------------------*/

Rcpp::List GetFontFileCMap(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  std::vector<CMapDirectory> cmaps = ttf.GetCMap();

  Rcpp::List result = List::create();
  for(auto j : cmaps)
  {
    std::vector<uint16_t> key, value;
    for(auto i : j.cmap_)
    {
      key.push_back(i.first);
      value.push_back(i.second);
    }
    result.push_back(List::create(Named("format") = j.format_,
                                  Named("platform_id") = j.platform_id_,
                                  Named("specific_id") = j.specific_id_,
                                  Named("encoding") = j.encoding_,
                                  Named("first")  = key,
                                  Named("second") = value));
  }
  return result;

}

/*---------------------------------------------------------------------------*/

List GetFontFileMaxp(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  auto maxp = ttf.GetMaxp();

  return List::create(
    Named("version")               = maxp.version_,
    Named("numGlyphs")             = maxp.numGlyphs_,
    Named("maxPoints")             = maxp.maxPoints_,
    Named("maxContours")           = maxp.maxContours_,
    Named("maxComponentPoints")    = maxp.maxComponentPoints_,
    Named("maxComponentContours")  = maxp.maxComponentContours_,
    Named("maxZones")              = maxp.maxZones_,
    Named("maxTwilightPoints")     = maxp.maxTwilightPoints_,
    Named("maxStorage")            = maxp.maxStorage_,
    Named("maxFunctionDefs")       = maxp.maxFunctionDefs_,
    Named("maxInstructionDefs")    = maxp.maxInstructionDefs_,
    Named("maxStackElements")      = maxp.maxStackElements_,
    Named("maxSizeOfInstructions") = maxp.maxSizeOfInstructions_,
    Named("maxComponentElements")  = maxp.maxComponentElements_,
    Named("maxComponentDepth")     = maxp.maxComponentDepth_);
};

/*---------------------------------------------------------------------------*/

Rcpp::DataFrame GetFontFileLoca(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  Loca loca = ttf.GetLoca();
  return Rcpp::DataFrame::create(Named("glyph")  = loca.glyph_,
                                 Named("offset") = loca.offset_,
                                 Named("length") = loca.length_);
};

/*---------------------------------------------------------------------------*/

List GetFontFileGlyph(RawVector raw, uint16_t glyph)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  Glyf g = ttf.ReadGlyf(glyph);

  Rcpp::List contours = Rcpp::List::create(
    Rcpp::DataFrame::create(
      Rcpp::Named("xcoords") = g.contours_[0].xcoords,
      Rcpp::Named("ycoords") = g.contours_[0].ycoords,
      Rcpp::Named("shape")   = g.contours_[0].shape)
  );

  if(g.numberOfContours_ < 1)
  {
    for(size_t i = 1; i < g.contours_.size(); i++)
    {
      Rcpp::DataFrame df = Rcpp::DataFrame::create(
        Rcpp::Named("xcoords") = g.contours_[i].xcoords,
        Rcpp::Named("ycoords") = g.contours_[i].ycoords,
        Rcpp::Named("shape")   = g.contours_[i].shape);
      contours.push_back(df);
    }
  }
  return List::create(Named("Glyph")      = glyph,
                      Named("N_Contours") = g.numberOfContours_,
                      Named("xmin")       = g.xMin_,
                      Named("xmax")       = g.xMax_,
                      Named("ymin")       = g.yMin_,
                      Named("ymax")       = g.yMax_,
                      Named("Contours")   = contours);
}


//---------------------------------------------------------------------------//

Rcpp::List GetFontFilePostTable(RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  Post post = ttf.GetPost();
  auto cmap_dir = ttf.GetCMap();
  std::vector<uint16_t> glyph_number;
  std::vector<uint16_t> code_point;
  std::vector<std::string> glyph_name;
  std::map<uint16_t, uint16_t> cmap;
  for(uint16_t i = 0; i < 256; i++) cmap[i] = i;
  bool cmap_found = false;
  for(uint16_t i = 0; i < cmap_dir.size(); i++)
  {
    if(cmap_dir[i].format_ == 4) {
      cmap = cmap_dir[i].cmap_;
      cmap_found = true;
      break;
    }
  }
  if(!cmap_found)
  {
    for(uint16_t i = 0; i < cmap_dir.size(); i++)
    {
      if(cmap_dir[i].format_ == 0) {
        cmap = cmap_dir[i].cmap_;
        break;
      }
    }
  }

  for(auto i = post.mapping.begin(); i != post.mapping.end(); i++)
  {
    for(auto j = cmap.begin(); j != cmap.end(); j++)
    {
      if(j->second == i->first)
      {
        code_point.push_back(j->first);
        glyph_number.push_back(i->first);
        glyph_name.push_back(i->second);
      }
    }


  }
  auto df = Rcpp::DataFrame::create(Named("code_point")   = code_point,
                                    Named("glyph_number") = glyph_number,
                                    Named("glyph_name")   = glyph_name);

  return Rcpp::List::create(
            Named("version") = post.version,
            Named("italic_angle") = post.italic_angle,
            Named("UnderlinePosition") = post.UnderlinePosition,
            Named("UnderlineThickness") = post.UnderlineThickness,
            Named("IsFixedPitch") = post.IsFixedPitch,
            Named("MinMemType42") = post.MinMemType42,
            Named("MaxMemType42") = post.MaxMemType42,
            Named("MinMemType1") = post.MinMemType1,
            Named("MaxMemType1") = post.MaxMemType1,
            Named("Map") = df);
}

//---------------------------------------------------------------------------//

Rcpp::DataFrame GetFontFileNameTable(Rcpp::RawVector raw)
{
  std::vector<uint8_t> fontfile = Rcpp::as<std::vector<uint8_t>>(raw);
  std::string fontstring(fontfile.begin(), fontfile.end());
  TTFont ttf(fontstring);
  Name name = ttf.GetName();
  return Rcpp::DataFrame::create(
    Named("platformID") = name.platformID,
    Named("platformSpecificID") = name.platformSpecificID,
    Named("languageID") = name.languageID,
    Named("nameID") = name.nameID,
    Named("text") = name.text);
}

//---------------------------------------------------------------------------//

#ifdef PROFILER_PDFR
void stopCpp(){TheNodeList::Instance().endprofiler(); }
#endif

#ifndef PROFILER_PDFR
void stopCpp(){}
#endif
