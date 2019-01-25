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
  // then, if they are not found, iteratively search the parent nodes until
  // they are

  // start with the page header as the boxheader - this can change in the loop
  dictionary boxheader = header;
  bool hasparent = true;  // need to keep track of whether node has a parent

  // Note this is a do...while loop - I don't use these much so pay attention!
  do
  {
    bleedbox = boxheader.getNums("/BleedBox");  //--//
    cropbox  = boxheader.getNums("/CropBox");       //
    mediabox = boxheader.getNums("/MediaBox");      //--> look for / store boxes
    artbox   = boxheader.getNums("/ArtBox");        //
    trimbox  = boxheader.getNums("/TrimBox");   //--//

    if (!bleedbox.empty()) minbox = bleedbox;   //--//
    if (!mediabox.empty()) minbox = mediabox;       //    iterate logically
    if (!cropbox.empty())  minbox = cropbox;        //--> nested boxes to find
    if (!trimbox.empty())  minbox = trimbox;        //    the smallest one
    if (!artbox.empty())   minbox = artbox;     //--//

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
  // create an error message in case of missing page
  std::string E = "No header found for page ";
  E += std::to_string(pagenumber);

  // uses public member of document class to get the appropriate header
  header = d->pageHeader(pagenumber);

  // if the header is not of /type /page, throw an error
  if (!header.has("/Type")) throw runtime_error(E);
  if(header.get("/Type") != "/Page") throw runtime_error(E);
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
    resources = dictionary(&resdict); // create a dictionary from the entry
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
    fonts = dictionary(&fontdict); // create the font dictionary from the entry
  }

  // We can now iterate through the font names using getFontNames(), create
  // each font in turn and store it in the fontmap
  for(auto h : getFontNames())
    for(auto hh : fonts.getRefs(h)) // find the ref for each font name
      fontmap[h] = font(d, d->getobject(hh)->getDict(), h); // create/store font
}

/*--------------------------------------------------------------------------*/
// The contents contain the page description program, sometimes split across
// several objects. Occasionally the contents for a page can be nested such
// that one reference contains a bunch of other references rather than the
// content stream itself.

void page::getContents()
{
  std::vector<int> cts = header.getRefs("/Contents"); // get content refs
  if (!cts.empty()) // If the contents are empty, the page string will be empty
  {                 // - though this is not necessarily an error
     // get all leaf nodes of the contents tree using expandContents()
    vector<int> contents = expandContents(cts);

    // get the contents from each object stream and paste them at the bottom
    // of the pagestring with a line break after each one
    for (auto m : contents)
      contentstring += d->getobject(m)->getStream() + std::string("\n");
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
  string xobjstring;

  // first find any xobject entries in the resource dictionary
  if (resources.has("/XObject")) xobjstring = resources.get("/XObject");

  // Sanity check - the entry shouldn't be empty
  if(xobjstring.empty()) return;

  dictionary objdict; // declare xobject dictionary to be filled when found

  // Look to see if the /xobject entry is a dictionary
  if(xobjstring.find("<<") != string::npos)
  {
    objdict = dictionary(&xobjstring);
  }
  else
  {
    // If the /XObject string is a reference, follow the reference and get dict
    std::vector<int> xos = resources.getRefs("/XObject");
    if (xos.size() == 1)
      objdict = d->getobject(xos.at(0))->getDict();
  }

  // Now we can get the dictionary's keys ...
  std::vector<std::string> dictkeys = objdict.getDictKeys();

  // ...and get the stream contents of all the references pointed to
  for(auto i : dictkeys)
  {
    std::vector<int> refints = objdict.getRefs(i);
    if(!refints.empty())
      // map xobject strings to the xobject names
      XObjects[i] = d->getobject(refints.at(0))->getStream();
  }
}

/*---------------------------------------------------------------------------*/
// This is similar to the expandKids() method in document class, in that it
// starts with a vector of references to objects which act as nodes of a tree
// structure. Most of the time these point straight to the content object,
// but it is possible to have nested content trees. In any case we only want
// the leaves of the content tree, which are found by this algorithm

vector <int> page::expandContents(vector<int> objnums)
{
  if(objnums.empty()) return objnums; // no contents - nothing to do!
  size_t i = 0;
  vector<int> res; // container for results
  while (i < objnums.size())
  {
    object_class* o = d->getobject(objnums[i]);
    if (o->getDict().hasRefs("/Contents"))
    {
      vector<int> newnodes = o->getDict().getRefs("/Contents"); // store refs
      objnums.erase(objnums.begin() + i); // delete parent node
      // write new nodes
      objnums.insert(objnums.begin() + i, newnodes.begin(), newnodes.end());
    }
    else // this is a leaf node - store it and move to next node
    {
      res.push_back(objnums[i]);
      i++;
    }
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// The page constructor calls private methods to build its data members after
// its initializer list

page::page(document* doc, int pagenum) : d(doc), pagenumber(pagenum), rotate(0)
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

string page::pageContents()
{
  return this->contentstring;
}

/*--------------------------------------------------------------------------*/
// Simple getter of the minimum bounding box for text on a page

vector<float> page::getminbox()
{
  return this->minbox;
}

/*--------------------------------------------------------------------------*/
// Finds and returns a particular XObject used on the page

string page::getXobject(const string& objID)
{
  // Use a non-inserting finder. If object not found return empty string
  if(XObjects.find(objID) == XObjects.end()) return "";
  return XObjects[objID]; // else return the Xobject
}

/*--------------------------------------------------------------------------*/
// The graphic_state class needs to use fonts stored in the fontmap. This getter
// will return a pointer to the requested font.

font* page::getFont(const string& fontID)
{
  // If no fonts on the page, throw an error
  if(fontmap.size() == 0) throw runtime_error("No fonts available for page");
  // If we can't find a specified font, return the first font in the map
  if(fontmap.find(fontID) == fontmap.end()) return &(fontmap.begin()->second);
  // Otherwise we're all good and return the requested font
  return &(fontmap[fontID]);
}
