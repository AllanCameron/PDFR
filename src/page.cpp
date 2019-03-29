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

std::unordered_map<std::string, shared_ptr<font>> page::fontmap;

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
  dictionary boxheader = header;
  vector<string> boxnames {"/BleedBox", "/CropBox", "/MediaBox",
                           "/ArtBox", "/TrimBox"};
  bool hasparent = true;  // need to keep track of whether node has a parent
  do{
    for(auto& i : boxnames)
    {
      vector<float>&& thisbox = boxheader.getNums(i);
      if (!thisbox.empty()) minbox = thisbox;
    }
    if(minbox.empty()) // if no boxes found
    {
      if(boxheader.hasRefs("/Parent"))  // find the parent
      {
        int parent = boxheader.getRefs("/Parent").at(0); // get the object
        boxheader = d->getobject(parent)->getDict(); // make it the boxheader
      }
      else hasparent = false; // didn't find any parent node - loop will exit
    }
  } while (minbox.empty() && hasparent); // stop loop if we have minbox or
                                        // there are no more parent nodes
  // Get the "rotate" value - will need in future feature development
  if (header.has("/Rotate")) rotate = header.getNums("/Rotate").at(0);
}

/*--------------------------------------------------------------------------*/
// Page creation starts with identifying the appropriate page header dictionary.
// This private method is called by the constructor to do that.

void page::getHeader()
{
  // uses public member of document class to get the appropriate header
  header = d->pageHeader(pagenumber);
  // if the header is not of /type /page, throw an error
  if (header.get("/Type") != "/Page")
  {
    // create an error message in case of missing page
    std::string E = "No header found for page ";
    E += std::to_string(pagenumber);
    throw runtime_error(E);
  }
}

/*--------------------------------------------------------------------------*/
// This is the second private method called as part of page construction.
// It finds the resources dicionary whether it is located in another object or
// as a subdictionary of the page header

void page::getResources()
{
  if (!header.hasDictionary("/Resources")) // if it isn't a dictionary...
  {
    // ...it must be a reference. This loop ensures only one is used (the last)
    vector<int> resourceobjs = header.getRefs("/Resources");
    for (auto q : resourceobjs) resources = d->getobject(q)->getDict();
  }
  else // Resources contains a subdictionary
  {
    string resdict = header.get("/Resources");
    resources = dictionary(make_shared<string>(resdict)); // create dictionary
  }
}

/*--------------------------------------------------------------------------*/
// The page's fonts dictionary bears a similar relationship to the resource
// dictionary as the resource dictionary does to the page header. It may be
// a subdictionary, or it may have its own dictionary in another object.

void page::getFonts()
{
  if (!resources.hasDictionary("/Font")) // if it isn't a dictionary...
  {
    // ...it must be a reference. This loop ensures only one is used (the last)
    std::vector<int> fontobjs = resources.getRefs("/Font");
    if (fontobjs.size() == 1) fonts = d->getobject(fontobjs.at(0))->getDict();
  }
  else // it's a subdictionary
  {
    string fontdict = resources.get("/Font");
    fonts = dictionary(make_shared<string>(fontdict)); // create font dict
  }
  // We can now iterate through the font names using getFontNames(), create
  // each font in turn and store it in the fontmap
  for(auto h : getFontNames())
    if(fontmap.find(h) == fontmap.end()) // fontmap is static
      for(auto hh : fonts.getRefs(h)) // find the ref for each font name
        fontmap[h] = make_shared<font>(d, d->getobject(hh)->getDict(), h);
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
    // use expandKids() to get page header object numbers
    expandContents(header.getRefs("/Contents"), root);
    auto contents = root->getLeafs();
    // get the contents from each object stream and paste them at the bottom
    // of the pagestring with a line break after each one
    contentstring.reserve(35000);
    for (auto m : contents)
      contentstring += d->getobject(m)->getStream() + std::string("\n");
    contentstring.shrink_to_fit();
}

/*---------------------------------------------------------------------------*/
// XObjects are components that can be called from a page description program.
// Most often, these are images, but they can be of many different types. Some
// contain textual components and form an integral part of the page. Any
// xobjects in the resources dictionary therefore needs to be examined for
// textual components and its uncompressed contents stored for later use

void page::parseXObjStream()
{
  string xobjstring {};
  // first find any xobject entries in the resource dictionary
  if (resources.has("/XObject")) xobjstring = resources.get("/XObject");
  if(xobjstring.empty()) return; // Sanity check - the entry shouldn't be empty
  dictionary objdict; // declare xobject dictionary to be filled when found
  if(xobjstring.find("<<") != string::npos) // Is /xobject entry a dictionary?
    objdict = dictionary(make_shared<string>(xobjstring)); // if so, make it
  else
  { // If the /XObject string is a reference, follow the reference and get dict
    std::vector<int> xos = resources.getRefs("/XObject");
    if (xos.size() == 1)
      objdict = d->getobject(xos.at(0))->getDict();
  }
  std::vector<std::string> dictkeys = objdict.getDictKeys();
  for(auto& i : dictkeys)
  {
    std::vector<int> refints = objdict.getRefs(i);
    if(!refints.empty()) // map xobject strings to the xobject names
      XObjects[i] = d->getobject(refints.at(0))->getStream();
  }
}

/*---------------------------------------------------------------------------*/
// This is similar to the expandKids() method in document class, in that it
// starts with a vector of references to objects which act as nodes of a tree
// structure. Most of the time these point straight to the content object,
// but it is possible to have nested content trees. In any case we only want
// the leaves of the content tree, which are found by this algorithm

void page::expandContents(vector<int> obs,
                                  shared_ptr<tree_node<int>> tree)
{
  tree->add_kids(obs); // create new children tree nodes with this one as parent
  auto kidnodes = tree->getkids(); // get a vector of pointers to the new nodes
  for(auto& i : kidnodes)  // now for each...
  {                        // get a vector of ints for its kid nodes
    vector<int> nodes = d->getobject(i->get())->getDict().getRefs("/Contents");
    if (!nodes.empty())
    {
      expandContents(nodes, i); // if it has kids, use recursion to get them
    }
  }
}

/*--------------------------------------------------------------------------*/
// The page constructor calls private methods to build its data members after
// its initializer list

page::page(shared_ptr<document> doc, int pagenum) :
  d(doc), pagenumber(pagenum), rotate(0)
{
  getHeader();        // find the page header
  getResources();     // find the resource header
  parseXObjStream();  // identify, parse and store XObjects
  getFonts();         // find the fonts dictionaries and build the fontmap
  getContents();      // Find the contents entries and build the content string
  boxes();            // Find the bouding box of the page
}

/*--------------------------------------------------------------------------*/
// Simple getter for the PDF-style font names used in a page

vector<string> page::getFontNames()
{
  return this->fonts.getDictKeys();
}

/*--------------------------------------------------------------------------*/
// Simple getter for the content string of a page

shared_ptr<string> page::pageContents()
{
  return make_shared<string>(this->contentstring);
}

/*--------------------------------------------------------------------------*/
// Finds and returns a particular XObject used on the page

shared_ptr<string> page::getXobject(const string& objID)
{
  // Use a non-inserting finder. If object not found return empty string
  if(XObjects.find(objID) == XObjects.end())
    return make_shared<string>(string(""));
  return make_shared<string>(XObjects[objID]); // else return the Xobject
}

/*--------------------------------------------------------------------------*/
// The parser class needs to use fonts stored in the fontmap. This getter
// will return a pointer to the requested font.

shared_ptr<font> page::getFont(const string& fontID)
{
  // If no fonts on the page, throw an error
  if(fontmap.empty()) throw runtime_error("No fonts available for page");
  // If we can't find a specified font, return the first font in the map
  auto f = fontmap.find(fontID);
  if(f == fontmap.end()) return (fontmap.begin()->second);
  // Otherwise we're all good and return the requested font
  return f->second;
}

/*--------------------------------------------------------------------------*/
// simple getter for page margins

std::vector<float> page::getminbox()
{
  return minbox;
}

/*--------------------------------------------------------------------------*/
// Important! Run this function when a document is destroyed or else the font
// map may accrue name clashes which will screw up encoding

void page::clearFontMap()
{
  this->fontmap.clear();
}
