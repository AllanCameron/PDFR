//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR page implementation file                                            //
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

#include "page.h"


//---------------------------------------------------------------------------//

using namespace std;

std::unordered_map<std::string, shared_ptr<font>> page::sm_fontmap;

/*--------------------------------------------------------------------------*/
// The page constructor calls private methods to build its data members after
// its initializer list

page::page(shared_ptr<document> doc, int pagenum) :
  m_doc(doc), m_page_number(pagenum), m_rotate(0)
{
  getHeader();        // find the page header
  getResources();     // find the resource header
  parseXObjStream();  // identify, parse and store XObjects
  getFonts();         // find the fonts dictionaries and build the fontmap
  getContents();      // Find the contents entries and build the content string
  boxes();            // Find the bouding box of the page
}

/*---------------------------------------------------------------------------*/
// The various "boxes" in a page header file define the maximum extent of the
// graphical contents of a page in different technical ways. For our purposes,
// we are really only interested in finding the smallest of these - the "minbox"
// because this will be exported as the page's dimensions. This private method
// finds and stores the minbox

void page::boxes()
{
  // sometimes the box dimensions are inherited from an ancestor node of the
  // page header. We therefore need to look for the boxes in the page header,
  // then, if they are not found, iteratively search the parent nodes
  dictionary box_header = m_header;
  vector<string> box_names {"/BleedBox", "/CropBox", "/MediaBox",
                           "/ArtBox", "/TrimBox"};
  vector<float> minbox;
  // Need to keep track of whether node has a parent
  bool has_parent = true;

  // This do...while loop allows upwards iteration through /Pages nodes
  do{
    // For each of the box names
    for(auto& i : box_names)
    {
      vector<float>&& this_box = box_header.getNums(i);
      if (!this_box.empty()) minbox = this_box;
    }

    // If no boxes found
    if(minbox.empty())
    {
      // Find the parent object number and get its object dictionary
      if(box_header.hasRefs("/Parent"))
      {
        int parent = box_header.getRefs("/Parent").at(0);
        box_header = m_doc->getobject(parent)->getDict();
      }

      // The loop will exit if it doesn't find a parent node
      else has_parent = false;
    }

    // stop loop if we have minbox or there are no more parent nodes
  } while (minbox.empty() && has_parent);

  m_minbox = Box(minbox);

  // Get the "rotate" value - will need in future feature development
  if (m_header.has("/Rotate")) m_rotate = m_header.getNums("/Rotate").at(0);
}

/*--------------------------------------------------------------------------*/
// Page creation starts with identifying the appropriate page header dictionary.
// This private method is called by the constructor to do that.

void page::getHeader()
{
  // uses public member of document class to get the appropriate header
  m_header = m_doc->pageHeader(m_page_number);

  // if the header is not of /type /page, throw an error
  if (m_header.get("/Type") != "/Page")
  {
    // create an error message in case of missing page
    std::string error_message("No header found for page ");
    error_message += std::to_string(m_page_number);
    throw runtime_error(error_message);
  }
}

/*--------------------------------------------------------------------------*/
// This is the second private method called as part of page construction.
// It finds the resources dicionary whether it is located in another object or
// as a subdictionary of the page header

void page::getResources()
{
  // If /Resources doesn't contain a dictionary it must be a reference
  if (!m_header.hasDictionary("/Resources"))
  {
    // Ensure it's only one resource reference
    vector<int> resource_objs = m_header.getRefs("/Resources");

    if(!resource_objs.empty())
    {
      m_resources = m_doc->getobject(resource_objs[0])->getDict();
    }
  }
  else // Resources contains a subdictionary
  {
    m_resources = m_header.getDictionary("/Resources");
  }
}

/*--------------------------------------------------------------------------*/
// The page's fonts dictionary bears a similar relationship to the resource
// dictionary as the resource dictionary does to the page header. It may be
// a subdictionary, or it may have its own dictionary in another object.

void page::getFonts()
{
  // If /Font entry of m_resources isn't a dictionary
  if (!m_resources.hasDictionary("/Font"))
  {
    // It must be a reference - follow this to get the dictionary
    std::vector<int> font_objs = m_resources.getRefs("/Font");
    if (font_objs.size() == 1)
    {
      m_fonts = m_doc->getobject(font_objs.at(0))->getDict();
    }
  }
  // Otherwise, it is a dictionary, so we get the result
  else m_fonts = m_resources.getDictionary("/Font");

  // We can now iterate through the font names using getFontNames(), create
  // each font in turn and store it in the fontmap
  for(auto font_label : m_fonts)
  {
    auto found_font = sm_fontmap.find(font_label.first);

    // If the font is not in the fontmap, insert it
    if(found_font == sm_fontmap.end())
    {
      // Find the reference for each font name
      for(auto reference : m_fonts.getRefs(font_label.first))
      {
        sm_fontmap[font_label.first] =  make_shared<font>(m_doc,
                                        m_doc->getobject(reference)->getDict(),
                                        font_label.first);
      }
    }
  }
}

/*--------------------------------------------------------------------------*/
// The contents contain the page description program, sometimes split across
// several objects. Occasionally the contents for a page can be nested such
// that one reference contains a bunch of other references rather than the
// content stream itself.

void page::getContents()
{
     // get all leaf nodes of the contents tree using expandContents()
    auto root = make_shared<tree_node<int>>(0);

    // use expandContents() to get page header object numbers
    expandContents(m_header.getRefs("/Contents"), root);
    auto contents = root->getLeafs();

    // Get the contents from each object stream and paste them at the bottom
    // of the pagestring with a line break after each one
    for (auto m : contents)
    {
      m_content_string.append(m_doc->getobject(m)->getStream());
      m_content_string.append(std::string("\n"));
    }
}

/*---------------------------------------------------------------------------*/
// XObjects are components that can be called from a page description program.
// Most often, these are images, but they can be of many different types. Some
// contain textual components and form an integral part of the page. Any
// xobjects in the resources dictionary therefore needs to be examined for
// textual components and its uncompressed contents stored for later use

void page::parseXObjStream()
{
  string xobject_string {};

  // first find any xobject entries in the resource dictionary
  if (m_resources.has("/XObject")) xobject_string = m_resources.get("/XObject");

  // Sanity check - the entry shouldn't be empty
  if(xobject_string.empty()) return;

  // If /xobject entry is a dictionary, create a dictionary object
  dictionary xobject_dict;
  if(xobject_string.find("<<") != string::npos)
  {
    xobject_dict = dictionary(make_shared<string>(xobject_string));
  }

  // If the /XObject string is a reference, follow the reference to dictionary
  else
  {
    std::vector<int> xobject_refs = m_resources.getRefs("/XObject");
    if (xobject_refs.size() == 1)
    {
      xobject_dict = m_doc->getobject(xobject_refs[0])->getDict();
    }
  }

  // We now have a dictionary of {xobject name: ref} from which to get xobjects
  for(auto& i : xobject_dict)
  {
    std::vector<int> xobj_objs = xobject_dict.getRefs(i.first);

    // map xobject strings to the xobject names
    if(!xobj_objs.empty())
    {
      m_XObjects[i.first] = m_doc->getobject(xobj_objs[0])->getStream();
    }
  }
}

/*---------------------------------------------------------------------------*/
// This is similar to the expandKids() method in document class, in that it
// starts with a vector of references to objects which act as nodes of a tree
// structure. Most of the time these point straight to the content object,
// but it is possible to have nested content trees. In any case we only want
// the leaves of the content tree, which are found by this algorithm

void page::expandContents(vector<int> objs, shared_ptr<tree_node<int>> tree)
{
  // Create new children tree nodes with this one as parent
  tree->add_kids(objs);

  // Get a vector of pointers to the new nodes
  auto kidnodes = tree->getkids();

  // Now for each get a vector of ints for its kid nodes
  for(auto& kid : kidnodes)
  {
    // Read the contents from the dictionary entry
    auto nodes = m_doc->getobject(kid->get())->getDict().getRefs("/Contents");

    // If it has kids, use recursion to get them
    if (!nodes.empty()) expandContents(nodes, kid);
  }
}

/*--------------------------------------------------------------------------*/
// Simple getter for the PDF-style font names used in a page

vector<string> page::getFontNames()
{
  return this->m_fonts.getDictKeys();
}

/*--------------------------------------------------------------------------*/
// Simple getter for the content string of a page

shared_ptr<string> page::pageContents()
{
  return make_shared<string>(this->m_content_string);
}

/*--------------------------------------------------------------------------*/
// Finds and returns a particular XObject used on the page

shared_ptr<string> page::getXobject(const string& object_ID)
{
  // Use a non-inserting finder. If object not found return empty string
  if(m_XObjects.find(object_ID) == m_XObjects.end())
  {
    return make_shared<string>(string(""));
  }

  // Else return the Xobject
  return make_shared<string>(m_XObjects[object_ID]);
}

/*--------------------------------------------------------------------------*/
// The parser class needs to use fonts stored in the fontmap. This getter
// will return a pointer to the requested font.

shared_ptr<font> page::getFont(const string& fontID)
{
  // If no fonts on the page, throw an error
  if(sm_fontmap.empty()) throw runtime_error("No fonts available for page");

  // If we can't find a specified font, return the first font in the map
  auto f = sm_fontmap.find(fontID);
  if(f == sm_fontmap.end()) return (sm_fontmap.begin()->second);

  // Otherwise we're all good and return the requested font
  return f->second;
}

/*--------------------------------------------------------------------------*/
// simple getter for page margins

Box page::getminbox()
{
  return m_minbox;
}

/*--------------------------------------------------------------------------*/
// Important! Run this function when a document is destroyed or else the font
// map may accrue name clashes which will screw up encoding

void page::clearFontMap()
{
  this->sm_fontmap.clear();
}
