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

#include "page.h"

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
  Dictionary box_header = header_;
  vector<string> box_names {"/BleedBox", "/CropBox", "/MediaBox",
                           "/ArtBox", "/TrimBox"};
  vector<float> minbox;
  // Need to keep track of whether node has a parent
  bool has_parent = true;

  // This do...while loop allows upwards iteration through /Pages nodes
  do{
    // For each of the box names
    for(auto& box_name : box_names)
    {
      vector<float>&& this_box = box_header.GetFloats(box_name);
      if (!this_box.empty()) minbox = this_box;
    }

    // If no boxes found
    if(minbox.empty())
    {
      // Find the parent object number and get its object dictionary
      if(box_header.ContainsReferences("/Parent"))
      {
        int parent = box_header.GetReference("/Parent");
        box_header = document_->GetObject(parent)->GetDictionary();
      }

      // The loop will exit if it doesn't find a parent node
      else has_parent = false;
    }

    // stop loop if we have minbox or there are no more parent nodes
  } while (minbox.empty() && has_parent);

  minbox_ = Box(minbox);

  // Get the "rotate" value - will need in future feature development
  if (header_.HasKey("/Rotate")) rotate_ = header_.GetFloats("/Rotate")[0];
}

/*--------------------------------------------------------------------------*/
// Page creation starts with identifying the appropriate page header dictionary.
// This private method is called by the constructor to do that.

void Page::ReadHeader()
{
  // uses public member of document class to get the appropriate header
  header_ = document_->GetPageHeader(page_number_);

  // if the header is not of /type /Page, throw an error
  if (header_.GetString("/Type") != "/Page")
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
  if (!header_.ContainsDictionary("/Resources"))
  {
    if(header_.ContainsReferences("/Resources"))
    {
      auto resource_number = header_.GetReference("/Resources");
      resources_ = document_->GetObject(resource_number)->GetDictionary();
    }
  }
  else // Resources contains a subdictionary
  {
    resources_ = header_.GetDictionary("/Resources");
  }
}

/*--------------------------------------------------------------------------*/
// The Page's fonts dictionary bears a similar relationship to the resource
// dictionary as the resource dictionary does to the page header. It may be
// a subdictionary, or it may have its own dictionary in another object.

void Page::ReadFonts()
{
  // If /Font entry of resources_ isn't a dictionary
  if (!resources_.ContainsDictionary("/Font"))
  {
    // It must be a reference - follow this to get the dictionary
    if (resources_.ContainsReferences("/Font"))
    {
      auto font_number = resources_.GetReference("/Font");
      fonts_ = document_->GetObject(font_number)->GetDictionary();
    }
  }
  // Otherwise, it is a dictionary, so we get the result
  else fonts_ = resources_.GetDictionary("/Font");

  // We can now iterate through the font names using getFontNames(), create
  // each font in turn and store it in the fontmap
  for(auto label : fonts_)
  {
    auto found_font = fontmap_.find(label.first);

    // If the font is not in the fontmap, insert it
    if(found_font == fontmap_.end())
    {
      // Find the reference for each font name
      for(auto reference : fonts_.GetReferences(label.first))
      {
        auto font_dict = document_->GetObject(reference)->GetDictionary();
        auto font_ptr = make_shared<Font>(document_, font_dict, label.first);
        fontmap_[label.first] = font_ptr;
      }
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
     // get all leaf nodes of the contents tree using expandContents()
    auto root = make_shared<TreeNode<int>>(0);

    // use expand_contents() to get page header object numbers
    ExpandContents(header_.GetReferences("/Contents"), root);
    auto contents = root->GetLeafs();

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
  if (resources_.HasKey("/XObject"))
    xobject_string = resources_.GetString("/XObject");

  // Sanity check - the entry shouldn't be empty
  if(xobject_string.empty()) return;

  // If /xobject entry is a dictionary, create a dictionary object
  Dictionary xobject_dictionary;
  if(xobject_string.find("<<") != string::npos)
  {
    xobject_dictionary = Dictionary(make_shared<string>(xobject_string));
  }

  // If the /XObject string is a reference, follow the reference to dictionary
  else if (resources_.ContainsReferences("/XObject"))
  {
    auto xobject_number = resources_.GetReference("/XObject");
    xobject_dictionary = document_->GetObject(xobject_number)->GetDictionary();
  }

  // We now have a dictionary of {xobject name: ref} from which to get xobjects
  for(auto& entry : xobject_dictionary)
  {
    std::vector<int> xobjects = xobject_dictionary.GetReferences(entry.first);

    // map xobject strings to the xobject names
    if(!xobjects.empty())
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

void Page::ExpandContents(vector<int> t_object_numbers_to_add, Node t_parent)
{
  // Create new children tree nodes with this one as parent
  t_parent->AddKids(t_object_numbers_to_add);

  // Get a vector of pointers to the new nodes
  auto kid_nodes = t_parent->GetKids();

  // Now for each get a vector of ints for its kid nodes
  for(auto& kid : kid_nodes)
  {
    // Read the contents from the dictionary entry
    auto kid_dictionary = document_->GetObject(kid->Get())->GetDictionary();
    auto content_nodes  = kid_dictionary.GetReferences("/Contents");

    // If it has kids, use recursion to get them
    if (!content_nodes.empty()) ExpandContents(content_nodes, kid);
  }
}

/*--------------------------------------------------------------------------*/
// Simple getter for the PDF-style font names used in a page

vector<string> Page::GetFontNames()
{
  return this->fonts_.GetAllKeys();
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
  if(xobjects_.find(t_object_id) == xobjects_.end())
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
  if(fontmap_.empty()) throw runtime_error("No fonts available for page");

  // If we can't find a specified font, return the first font in the map
  auto font_finder = fontmap_.find(t_font_id);
  if(font_finder == fontmap_.end()) return fontmap_.begin()->second;

  // Otherwise we're all good and return the requested font
  return font_finder->second;
}

