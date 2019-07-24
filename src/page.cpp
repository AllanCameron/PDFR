//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Page implementation file                                            //
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
#include "document.h"
#include "box.h"
#include "page.h"
#include<list>
#include<iterator>

//---------------------------------------------------------------------------//

using namespace std;

unordered_map<string, shared_ptr<Font>> Page::fontmap_;

/*--------------------------------------------------------------------------*/
// The Page constructor calls private methods to build its data members after
// its initializer list

Page::Page(shared_ptr<Document> t_document_ptr, int t_page_number) :
  document_(t_document_ptr), page_number_(t_page_number), rotate_(0)
{
  ReadHeader();        // find the page header
  ReadResources();     // find the resource header
  ReadXObjects();      // identify, parse and store XObjects
  ReadFonts();         // find the fonts dictionaries and build the fontmap
  ReadContents();      // Find the contents entries and build the content string
  ReadBoxes();         // Find the bouding box of the page
}

/*---------------------------------------------------------------------------*/
// The various "boxes" in a page header file define the maximum extent of the
// graphical contents of a page in different technical ways. For our purposes,
// we are really only interested in finding the smallest of these - the "minbox"
// because this will be exported as the page's dimensions. This private method
// finds and stores the minbox

void Page::ReadBoxes()
{
  // sometimes the box dimensions are inherited from an ancestor node of the
  // page header. We therefore need to look for the boxes in the page header,
  // then, if they are not found, iteratively search the parent nodes
  Dictionary box_header = *header_;
  vector<string> box_names {"/BleedBox", "/CropBox", "/MediaBox",
                           "/ArtBox", "/TrimBox"};
  vector<float> minbox;
  // Need to keep track of whether node has a parent
  bool has_parent = true;

  // This do...while loop allows upwards iteration through /Pages nodes
  do{
    // For each of the box names
    for (auto& box_name : box_names)
    {
      vector<float>&& this_box = box_header.GetFloats(box_name);
      if (!this_box.empty()) minbox = this_box;
    }

    // If no boxes found
    if (minbox.empty())
    {
      // Find the parent object number and get its object dictionary
      if (box_header.ContainsReferences("/Parent"))
      {
        int parent = box_header.GetReference("/Parent");
        box_header = document_->GetObject(parent)->GetDictionary();
      }

      // The loop will exit if it doesn't find a parent node
      else has_parent = false;
    }

    // stop loop if we have minbox or there are no more parent nodes
  } while (minbox.empty() && has_parent);

  minbox_ = make_shared<Box>(minbox);

  // Get the "rotate" value - will need in future feature development
  if (header_->HasKey("/Rotate")) rotate_ = header_->GetFloats("/Rotate")[0];
}

/*--------------------------------------------------------------------------*/
// Page creation starts with identifying the appropriate page header dictionary.
// This private method is called by the constructor to do that.

void Page::ReadHeader()
{
  // uses public member of document class to get the appropriate header
  header_ = make_shared<Dictionary>(document_->GetPageHeader(page_number_));

  // if the header is not of /type /Page, throw an error
  if (header_->GetString("/Type") != "/Page")
  {
    // create an error message in case of missing page
    std::string error_message("No header found for page ");
    error_message += std::to_string(page_number_);
    throw runtime_error(error_message);
  }
}

/*--------------------------------------------------------------------------*/
// This is the second private method called as part of Page construction.
// It finds the resources dicionary whether it is located in another object or
// as a subdictionary of the page header

void Page::ReadResources()
{
  // If /Resources doesn't contain a dictionary it must be a reference
  if (!header_->ContainsDictionary("/Resources"))
  {
    if (header_->ContainsReferences("/Resources"))
    {
      auto resource_number = header_->GetReference("/Resources");
      resources_ = make_shared<Dictionary>(
        document_->GetObject(resource_number)->GetDictionary());
    }
  }
  else // Resources contains a subdictionary
  {
    resources_ =  make_shared<Dictionary>(header_->GetDictionary("/Resources"));
  }
}

/*--------------------------------------------------------------------------*/
// The Page's fonts dictionary bears a similar relationship to the resource
// dictionary as the resource dictionary does to the page header. It may be
// a subdictionary, or it may have its own dictionary in another object.

void Page::ReadFonts()
{
  // If /Font entry of resources_ isn't a dictionary
  if (!resources_->ContainsDictionary("/Font"))
  {
    // It must be a reference - follow this to get the dictionary
    if (resources_->ContainsReferences("/Font"))
    {
      auto font_number =  resources_->GetReference("/Font");
      fonts_ =  make_shared<Dictionary>(
        document_->GetObject(font_number)->GetDictionary());
    }
  }
  // Otherwise, it is a dictionary, so we get the result
  else fonts_ =  make_shared<Dictionary>(resources_->GetDictionary("/Font"));

  // We can now iterate through the font dictionary, which will be a sequence
  // of key:value pairs of fontname : font descriptor, where the font descriptor
  // is almost always a reference but can also be a direct dictionary
  for (auto name_descriptor_pair : *fonts_)
  {
    auto& font_name = name_descriptor_pair.first;
    auto& font_descriptor = name_descriptor_pair.second;
    auto found_font = fontmap_.find(font_name);

    // If the font is not in the fontmap, inserts it
    if (found_font == fontmap_.end())
    {
      shared_ptr<Dictionary> font_dict;

      // Handle the font descriptor being a direct dictionary
      if (font_descriptor.find("<<") != string::npos)
      {
        font_dict = make_shared<Dictionary>(
                      make_shared<string>(font_descriptor));
      }

      // If it's not a direct dictionary, it must be a reference
      else
      {
        auto font_reference = fonts_->GetReference(font_name);
        font_dict = make_shared<Dictionary>(
          document_->GetObject(font_reference)->GetDictionary());
      }

      // We should now have a font dictionary from which to create a Font object
      auto font_ptr = make_shared<Font>(document_, font_dict, font_name);
      fontmap_[font_name] = font_ptr;
    }
  }
}

/*--------------------------------------------------------------------------*/
// The contents contain the page description program, sometimes split across
// several objects. Occasionally the contents for a page can be nested such
// that one reference contains a bunch of other references rather than the
// content stream itself.

void Page::ReadContents()
{
  // use expand_contents() to get page header object numbers
  auto contents = ExpandContents(header_->GetReferences("/Contents"));

  // Get the contents from each object stream and paste them at the bottom
  // of the pagestring with a line break after each one
  for (auto element : contents)
  {
    content_string_.append(document_->GetObject(element)->GetStream());
    content_string_.append(string("\n"));
  }
}

/*---------------------------------------------------------------------------*/
// XObjects are components that can be called from a page description program.
// Most often, these are images, but they can be of many different types. Some
// contain textual components and form an integral part of the page. Any
// xobjects in the resources dictionary therefore needs to be examined for
// textual components and its uncompressed contents stored for later use

void Page::ReadXObjects()
{
  string xobject_string {};

  // first find any xobject entries in the resource dictionary
  if (resources_->HasKey("/XObject"))
    xobject_string = resources_->GetString("/XObject");

  // Sanity check - the entry shouldn't be empty
  if (xobject_string.empty()) return;

  // If /xobject entry is a dictionary, create a dictionary object
  Dictionary xobject_dictionary;
  if (xobject_string.find("<<") != string::npos)
  {
    xobject_dictionary = Dictionary(make_shared<string>(xobject_string));
  }

  // If the /XObject string is a reference, follow the reference to dictionary
  else if (resources_->ContainsReferences("/XObject"))
  {
    auto xobject_number = resources_->GetReference("/XObject");
    xobject_dictionary = document_->GetObject(xobject_number)->GetDictionary();
  }

  // We now have a dictionary of {xobject name: ref} from which to get xobjects
  for (auto& entry : xobject_dictionary)
  {
    std::vector<int> xobjects = xobject_dictionary.GetReferences(entry.first);

    // map xobject strings to the xobject names
    if (!xobjects.empty())
    {
      xobjects_[entry.first] = document_->GetObject(xobjects[0])->GetStream();
    }
  }
}

/*---------------------------------------------------------------------------*/
// This is similar to the expandKids() method in document class, in that it
// starts with a vector of references to objects which act as nodes of a tree
// structure. Most of the time these point straight to the content object,
// but it is possible to have nested content trees. In any case we only want
// the leaves of the content tree, which are found by this algorithm

vector<int> Page::ExpandContents(vector<int> t_object_numbers)
{
  std::list<int> kids_list(t_object_numbers.begin(), t_object_numbers.end());
  auto kid = kids_list.begin();
  while (kid != kids_list.end())
  {
    auto content_dictionary = document_->GetObject(*kid)->GetDictionary();
    auto refs = content_dictionary.GetReferences("/Contents");
    if (!refs.empty())
    {
      for(auto new_kid : refs) kids_list.insert(kid, new_kid);
      auto erase_point = kid;
      kids_list.erase(erase_point);
      kid = std::prev(kid, refs.size());
    }
    else
    {
      ++kid;
    }
  }

  std::vector<int> result(kids_list.begin(), kids_list.end());
  return result;
}

/*--------------------------------------------------------------------------*/
// Simple getter for the PDF-style font names used in a page

vector<string> Page::GetFontNames()
{
  return this->fonts_->GetAllKeys();
}

/*--------------------------------------------------------------------------*/
// Simple getter for the content string of a Page

shared_ptr<string> Page::GetPageContents()
{
  return make_shared<string>(this->content_string_);
}

/*--------------------------------------------------------------------------*/
// Finds and returns a particular XObject used on the page

shared_ptr<string> Page::GetXObject(const string& t_object_id)
{
  // Use a non-inserting finder. If object not found return empty string
  if (xobjects_.find(t_object_id) == xobjects_.end())
  {
    return make_shared<string>(string(""));
  }

  // Else return the Xobject
  return make_shared<string>(xobjects_[t_object_id]);
}

/*--------------------------------------------------------------------------*/
// The parser class needs to use fonts stored in the fontmap. This getter
// will return a pointer to the requested font.

shared_ptr<Font> Page::GetFont(const string& t_font_id)
{
  // If no fonts on the page, throw an error
  if (fontmap_.empty()) throw runtime_error("No fonts available for page");

  // If we can't find a specified font, return the first font in the map
  auto font_finder = fontmap_.find(t_font_id);
  if (font_finder == fontmap_.end()) return fontmap_.begin()->second;

  // Otherwise we're all good and return the requested font
  return font_finder->second;
}

