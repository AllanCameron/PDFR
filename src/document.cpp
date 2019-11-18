//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Document implementation file                                        //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "dictionary.h"
#include "xref.h"
#include "object_class.h"
#include "document.h"
#include<iostream>
#include<Rcpp.h>
#include<iterator>


//---------------------------------------------------------------------------//
// See the document.h file for comments regarding the rationale and use of this
// class. The comments here are for descriptions of the functions themselves.
//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// Constructor for Document class. This takes a file location as an argument
// then uses the GetFile() function from utilities.h to read in the filestring
// from which buildDoc() then creates the object

Document::Document(const string& p_file_path)
  : file_string_(GetFile(p_file_path))
{
  BuildDocument_(); // Call constructor helper to build Document
}

/*---------------------------------------------------------------------------*/
// Constructor for Document class. This takes raw data in the form of a vector
// of bytes and converts it to the filestring before calling the helper function
// to construct the Document object

Document::Document(const vector<uint8_t>& p_byte_vector)
  : file_string_(string(p_byte_vector.begin(), p_byte_vector.end()))
{
  BuildDocument_(); // Call constructor helper to build Document
}

/*---------------------------------------------------------------------------*/
// buildDoc is just a helper function to create Document objects. It is the
// "final common pathway" of both non-default Document constructor functions
// and is seperated out to make this clear and avoid duplication of code

void Document::BuildDocument_()
{
  xref_ = make_shared<const XRef>(make_shared<string>(file_string_));

  // The pointer to the catalog is given under /Root in the trailer dictionary
  int&& root_number = xref_->GetTrailer().GetReference("/Root");

  // With errors handled, we can now just get the pointed-to object's dictionary
  auto&& catalog = Dictionary(GetObject(root_number)->GetDictionary());

  // Else get the object number of the /Pages dictionary
  int&& page_object_number = catalog.GetReference("/Pages");

  // Now fetch that object and store it
  auto&& directory = Dictionary(GetObject(page_object_number)->GetDictionary());

  // Ensure /Pages has /kids entry
  if (!directory.ContainsReferences("/Kids"))
    throw runtime_error("No Kids entry in /Pages");

  // Populate the page object numbers by expanding the /Kids references
  page_object_numbers_ = ExpandKids_(directory.GetReferences("/Kids"));
}

/*---------------------------------------------------------------------------*/
// The /Pages dictionary acts as a root node to point to the actual objects
// that contain page descriptors. These pointers are given in the /Kids entry
// of the /Pages dictionary. Unfortunately, sometimes with a large document,
// the pointers do not point directly to page descriptors, but to further
// /Pages dictionaries with /Kids entries that act as parent nodes for further
// /Pages dictionaries and so on. This is a tree structure, but my attempt to
// model this with a tree structure caused runtime errors on 64 bit
// architectures, so I've created a simpler algorithm using std::list instead.
//
// This function takes a lot of the time needed for document creation. It is
// not that the algorithm is particularly slow; rather, it has to create all the
// objects it comes across, and there are at least as many of these are there
// are pages.

vector<int> Document::ExpandKids_(const vector<int>& p_object_numbers)
{
  // We first copy the vector over to a list because we may need to do a lot
  // of insertions depending on how big the document is, and vectors are not
  // efficient for this purpose.
  list<int> kids_list(p_object_numbers.begin(), p_object_numbers.end());

  // Define an iterator to erase root nodes and replace with child nodes.
  auto kid = kids_list.begin();

  // We now move through the list from left to right. For each number we come
  // across, we look up its object dictionary and find out whether it has child
  // nodes (i.e. a /Kids entry). If it doesn't, it is a leaf node (i.e. it is
  // a page header dictionary) and we increment our iterator. If it does have
  // child nodes, we look them up, insert them (they are inserted before the
  // index node where the iterator sits). Before we erase the root node, we
  // need an iterator that points to the first of the new child nodes because
  // we will also need to examine these for /Kids entries. We therefore copy
  // our iterator while it is on the root node to record an "erase point", back
  // up our main iterator by the number of new nodes we have inserted, and
  // finally erase the root node. This leaves us ready to perform the next loop.
  while (kid != kids_list.end())
  {
    // Look up the /Kids entry to see whether this is a root or leaf node
    auto refs = GetObject(*kid)->GetDictionary().GetReferences("/Kids");

    // If refs contains at least one member, the current node is a root node.
    // We therefore need to replace it with its child nodes.
    if (!refs.empty())
    {
      for (auto new_kid : refs)
      {
        kids_list.insert(kid, new_kid); // Insert is OK since this is a list
      }
      auto erase_point = kid;
      kid = prev(kid, refs.size());
      kids_list.erase(erase_point);
    }
    else // If there are no /Kids, this is a leaf node - increment to next node
    {
      ++kid;
    }
  }

  // Remember to convert the list back into a vector for return
  vector<int> result(kids_list.begin(), kids_list.end());
  return result;
}

/*---------------------------------------------------------------------------*/
// One of two public interface functions after Document object creation. This
// returns an object of Object class with the requested object number. It first
// checks to see whether that object has been retrieved before. If so, it
// returns it from the 'objects' vector. If not, it creates the object then
// stores a copy in the 'objects' vector before returning the requested object.

shared_ptr<Object> Document::GetObject(int p_object_number)
{
  // Check if object n is already stored
  if (object_cache_.find(p_object_number) == object_cache_.end())
  {
    // If it is not stored, check whether it is in an object stream
    size_t holder = xref_->GetHoldingNumberOf(p_object_number);

    // If object is in a stream, create it recursively from the stream object.
    // Otherwise create & store it directly
    if (holder)
    {
      auto object_ptr = make_shared<Object>(GetObject(holder), p_object_number);
      object_cache_[p_object_number] = object_ptr;
    }
    else
    {
      auto object_ptr = make_shared<Object>(xref_, p_object_number);
      object_cache_[p_object_number] = object_ptr;
    }
  }
  return object_cache_[p_object_number];
}


/*---------------------------------------------------------------------------*/
// Public function that gets a specific page header from the pageheader vector

Dictionary Document::GetPageHeader(size_t p_page_number)
{
  // Ensure the pagenumber is valid
  if (page_object_numbers_.size() < p_page_number)
  {
    throw runtime_error("Invalid page number");
  }

  // All good - return the requested header
  return object_cache_[page_object_numbers_[p_page_number]]->GetDictionary();
}
