//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR document implementation file                                        //
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

#include "document.h"

//---------------------------------------------------------------------------//
// See the document.h file for comments regarding the rationale and use of this
// class. The comments here are for descriptions of the functions themselves.
//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// Constructor for document class. This takes a file location as an argument
// then uses the get_file() function from utilities.h to read in the filestring
// from which buildDoc() then creates the object

document::document(const string& filename) :
  file(filename), filestring(get_file(file))
{
  buildDoc(); // Call constructor helper to build document
}

/*---------------------------------------------------------------------------*/
// Constructor for document class. This takes raw data in the form of a vector
// of bytes and converts it to the filestring before calling the helper function
// to construct the document object

document::document(const vector<uint8_t>& bytevector) :
  filestring(bytestostring(bytevector))
{
  buildDoc(); // Call constructor helper to build document
}

/*---------------------------------------------------------------------------*/
// buildDoc is just a helper function to create document objects. It is the
// "final common pathway" of both non-default document constructor functions
// and is seperated out to make this clear and avoid duplication of code

void document::buildDoc()
{
  Xref = xref(&filestring); // Creates the xref from a pointer to the filestring
  getCatalog();             // Gets the catalog dictionary
  getPageDir();             // Gets the /Pages dictionary
  getPageHeaders();         // Finds all descendant leaf nodes of /Pages
}

/*---------------------------------------------------------------------------*/
// One of two public interface functions after document object creation. This
// returns an object of object_class with the requested object number. It first
// checks to see whether that object has been retrieved before. If so, it
// returns it from the 'objects' vector. If not, it creates the object then
// stores a copy in the 'objects' vector before returning the requested object.

object_class* document::getobject(int n)
{
  if(objects.find(n) == objects.end())    // check if object is stored
  {
    size_t holder = Xref.inObject(n); // ensure no holding object
    if(holder != 0) // if object is in an object stream
    {
      if(objects.find(holder) == objects.end()) // Get holding object if needed
        objects[holder] = object_class(&(this->Xref), holder);
      objects[n] = object_class(&(objects[holder]), n);
    }
    else
    objects[n] = object_class(&(this->Xref), n); // if not, create and store it
  }
  return &(objects[n]); // return a pointer to requested object
}

/*---------------------------------------------------------------------------*/
// The catalog (never spelled 'catalogue') dictionary contains information
// about the pdf, including which object contains the /Pages dictionary.
// This finds the catalog object from the trailer dictionary which is read as
// part of xref creation

void document::getCatalog()
{
  // The pointer to the catalog is given under /Root in the trailer dictionary
  vector<int> rootnums = Xref.trailer().getRefs("/Root");

  // This is the only place we look for the catalog, so it better be here...
  if (rootnums.empty()) throw runtime_error("Couldn't find catalog dictionary");

  // With errors handled, we can now just get the pointed-to object's dictionary
  catalog = getobject(rootnums.at(0))->getDict();
}

/*---------------------------------------------------------------------------*/
// The /Pages dictionary contains pointers to every page represented in the pdf
// file. There should be a pointer to it in the catalog dictionary. If its not
// there, the pdf structure is unspecified and we throw an error

void document::getPageDir()
{
  // Throw an error if catalog has no /Pages entry
  if(!catalog.hasRefs("/Pages")) throw runtime_error("No valid /Pages entry");

  // Else get the object number of the /Pages dictionary
  int pagesobject = catalog.getRefs("/Pages").at(0);

  // Now fetch that object and store it
  pagedir = getobject(pagesobject)->getDict();
}

/*---------------------------------------------------------------------------*/
// The /Pages dictionary acts as a root node to point to the actual objects
// that contain page descriptors. These pointers are given in the /Kids entry
// of the /Pages dictionary. Unfortunately, sometimes with a large document,
// the pointers do not point directly to page descriptors, but to further
// /Pages dictionaries with /Kids entries that act as parent nodes for further
// /Pages dictionaries and so on. This is a tree structure, and for our purposes
// we only want the leaf nodes of the tree. This function is an algorithm for
// finding and storing the leaf nodes in the correct order in a vector, given
// only the array of pointers from the root node.

vector <int> document::expandKids(vector<int> objnums)
{
  // If there are no objects fed to the function, no pages can be found - halt
  if(objnums.empty()) throw runtime_error("No pages found");

  size_t i = 0;                 // initialize iterator
  vector<int> res;              // container for result

  // The given vector of descendant object numbers will grow as the child nodes
  // are expanded, until all leaf nodes have been found. If there are only leaf
  // nodes to begin with, then the tree will stay the same size. In either case,
  // for each node that we assess, we test whether it is a leaf, in which case
  // we push it to our final result. Otherwise, we get its /Kids entry and
  // replace the node with its children. Because we are using a vector, this
  // is not a very efficient process for deeply nested trees, but it does
  // not make any assumptions about the nodes being balanced, and profiling
  // suggests it makes little or no difference to other quicker but less safe
  // alternatives

  while (i < objnums.size())
  {
    object_class* o = getobject(objnums[i]); // get the node object
    if (o->getDict().hasRefs("/Kids"))       // if it has Kids, its not a leaf
    {
      vector<int> newnodes = o->getDict().getRefs("/Kids"); // store kids
      objnums.erase(objnums.begin() + i);                  // delete parent node
      // inset kids to replace deleted parent node
      objnums.insert(objnums.begin() + i, newnodes.begin(), newnodes.end());
      // The next cycle of the loo will start at the first child node, so
      // it is not correct to increment unless a leaf node has been reached
    }
    else // this is a leaf node
    {
      res.push_back(objnums[i]); // store them in the order we find them
      i++;  // and increment the loop to the next node
    }
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// In order to construct the document class, we need to build its vector of page
// header dictionaries. This simply uses the expandKids algorithm to get all
// the leaf nodes from the root /Pages dictionary

void document::getPageHeaders()
{
  // Ensure /Pages has /kids entry
  if (!pagedir.hasRefs("/Kids"))
    throw runtime_error("No /Kids entry in /Pages dictionary.");

    // use expandKids() to get page header object numbers
    std::vector<int> kids = expandKids(pagedir.getRefs("/Kids"));

    // ensure we have enough room for the page headers in our vector
    pageheaders.reserve(kids.size());

    // Now we can just fill up our pageheader vector
    for (auto i : kids) pageheaders.emplace_back(objects.at(i).getDict());
}

/*---------------------------------------------------------------------------*/
// Public function that gets a specific page header from the pageheader vector

dictionary document::pageHeader(int pagenumber)
{
  // Ensure the pagenumber is valid
  if((pageheaders.size() < (size_t) pagenumber) || pagenumber < 0)
    throw runtime_error("Invalid page number");
  // All good - return the requested header
  return pageheaders.at(pagenumber);
}

/*---------------------------------------------------------------------------*/
// Public function that gets the number of pages in a document

size_t document::pagecount()
{
  return pageheaders.size();
}
