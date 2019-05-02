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
#include<list>

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
  filestring(std::string(bytevector.begin(), bytevector.end()))
{
  buildDoc(); // Call constructor helper to build document
}

/*---------------------------------------------------------------------------*/
// buildDoc is just a helper function to create document objects. It is the
// "final common pathway" of both non-default document constructor functions
// and is seperated out to make this clear and avoid duplication of code

void document::buildDoc()
{
  Xref = make_shared<const xref>(make_shared<string>(filestring));
  getCatalog();             // Gets the catalog dictionary
  getPageDir();             // Gets the /Pages dictionary
}

/*---------------------------------------------------------------------------*/
// One of two public interface functions after document object creation. This
// returns an object of object_class with the requested object number. It first
// checks to see whether that object has been retrieved before. If so, it
// returns it from the 'objects' vector. If not, it creates the object then
// stores a copy in the 'objects' vector before returning the requested object.

shared_ptr<object_class> document::getobject(int n)
{
  // Check if object n is already stored
  if(objects.find(n) == objects.end())
  {
    // If it is not stored, check whether it is in an object stream
    size_t holder = Xref->get_holding_object_number_of(n);

    // If object is in a stream, create it recursively from the stream object
    if(holder) objects[n] = make_shared<object_class>(getobject(holder), n);

    // Otherwise create & store it directly
    else objects[n] = make_shared<object_class>(Xref, n);
  }

  return objects[n];
}

/*---------------------------------------------------------------------------*/
// The catalog (never spelled 'catalogue') dictionary contains information
// about the pdf, including which object contains the /Pages dictionary.
// This finds the catalog object from the trailer dictionary which is read as
// part of xref creation

void document::getCatalog()
{
  // The pointer to the catalog is given under /Root in the trailer dictionary
  vector<int> rootnums = Xref->get_trailer().get_references("/Root");

  // This is the only place we look for the catalog, so it better be here...
  if (rootnums.empty()) throw runtime_error("Couldn't find catalog dictionary");

  // With errors handled, we can now just get the pointed-to object's dictionary
  catalog = getobject(rootnums[0])->getDict();
}

/*---------------------------------------------------------------------------*/
// The /Pages dictionary contains pointers to every page represented in the pdf
// file. There should be a pointer to it in the catalog dictionary. If its not
// there, the pdf structure is unspecified and we throw an error

void document::getPageDir()
{
  // Throw an error if catalog has no /Pages entry
  if(!catalog.contains_references("/Pages"))
    throw runtime_error("No valid /Pages entry");

  // Else get the object number of the /Pages dictionary
  int pagesobject = catalog.get_references("/Pages")[0];

  // Now fetch that object and store it
  pagedir = getobject(pagesobject)->getDict();

  // Ensure /Pages has /kids entry
  if (!pagedir.contains_references("/Kids"))
    throw runtime_error("No Kids entry in /Pages");

  // Create the page directory tree. Start with the pages object as root node
  auto root = make_shared<tree_node<int>>(pagesobject);

  // Populate the tree
  expandKids(pagedir.get_references("/Kids"), root);

  // Get the leafs of the tree
  pageheaders = root->getLeafs();
}

/*---------------------------------------------------------------------------*/
// The /Pages dictionary acts as a root node to point to the actual objects
// that contain page descriptors. These pointers are given in the /Kids entry
// of the /Pages dictionary. Unfortunately, sometimes with a large document,
// the pointers do not point directly to page descriptors, but to further
// /Pages dictionaries with /Kids entries that act as parent nodes for further
// /Pages dictionaries and so on. This is a tree structure, and for our purposes
// we only want the leaf nodes of the tree. This algorithm uses recursion to
// populate the nodes of the tree class defined in utilities.h.
//
// This function takes a lot of the time needed for document creation. It is
// not that the algorithm is particularly slow; rather, it has to create all the
// objects it comes across, and there are at least as many of these are there
// are pages. Options for speeding this up include getting only the objects
// needed for a particular page's creation, which would mean a major change to
// the way the program works depending on user input, and only getting object
// streams for an object when they are requested. This second way is more
// promising, but it is likely to be complex, and it is not clear that it would
// lead to a major speedup. Getting an object at present takes about 36us.

void document::expandKids(const vector<int>& obs,
                          shared_ptr<tree_node<int>> tree)
{
  // This function is only called from a single point that is already range
  // checked, so does not need error checked.

  // Create new children tree nodes with this one as parent
  tree->add_kids(obs);

  // Get a vector of pointers to the new nodes
  auto kidnodes = tree->getkids();

  // For each new node get a vector of ints for its kid nodes
  for(auto& kid : kidnodes)
  {
    auto refs = getobject(kid->get())->getDict().get_references("/Kids");

    // If it has children, use recursion to get them
    if (!refs.empty()) expandKids(refs, kid);
  }
}

/*---------------------------------------------------------------------------*/
// Public function that gets a specific page header from the pageheader vector

dictionary document::pageHeader(int pagenumber)
{
  // Ensure the pagenumber is valid
  if((pageheaders.size() < (size_t) pagenumber) || pagenumber < 0)
  {
    throw runtime_error("Invalid page number");
  }

  // All good - return the requested header
  return objects[pageheaders[pagenumber]]->getDict();
}


