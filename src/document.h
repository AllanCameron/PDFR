//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Document header file                                                //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_DOCUMENT

//---------------------------------------------------------------------------//

#define PDFR_DOCUMENT

/* This is the fifth header file in a daisy-chain of headers that build up the
 * tools required to parse pdfs. It follows just after the definition of the
 * object_class.
 *
 * The tools already in place have abstracted away decryption, decompression,
 * bytewise navigation of the file and parsing of dictionaries. The job of the
 * Document class is therefore to act as an interface to use the pdf objects
 * from which we build up logical structures such as fonts, xobjects and pages.
 *
 * The previous classes have been encapsulated as far as possible to be able to
 * work in isolation with minimal knowledge of each other. The Document class
 * in contrast acts as a creator, container and user of these objects.
 *
 * Each Document will have one and only one xref class. Instead of a pointer to
 * the xref as in other classes, the xref is actually a data member of the
 * Document class. PDF objects are created and stored in a map for easy access.
 * The file string is stored here and any other class that needs to read the
 * file accesses a pointer to the filestring held in the Document class.
 *
 * The Document class is therefore self-contained, in that after the initial
 * step of reading in the file, it has everything in needs to build up its
 * own components and interface. The logical PDF structures we go on to build
 * only need to know about the Document class, and can use it as the interface
 * they need. They "see" the pdf as a random access collection of numbered
 * objects with key:value dictionaries and uncompressed streams without being
 * concerned about how that is implemented.
 *
 * The Document also needs to have an outline of its own logical structure,
 * in terms of the pages it contains and where they are located. Part of the
 * task of Document creation is therefore to count and locate the objects
 * that act as page descriptors. It does this by finding the catalog
 * dictionary and then following pointers to dictionaries that contain
 * individual page headers. There is then a "getter" function for other classes
 * to access the dictionary pertaining to a particular page
 */

#include<string>
#include<vector>
#include<unordered_map>
#include<memory>

class Dictionary;
class XRef;
class Object;

//---------------------------------------------------------------------------//
// The public interface of the Document class comprises constructors and two
// member functions - one to return any object from the pdf and one to retrieve
// a specific page header.

class Document
{
 public:
  // Constructor to create Document from file path
  Document(const std::string& file_path)
   : file_string_(GetFile(file_path))
   { BuildDocument_(); }

  // Constructor to create Document from raw data
  Document(const std::vector<uint8_t>& byte_vector)
   : file_string_(std::string(byte_vector.begin(), byte_vector.end()))
   { BuildDocument_(); }


  // Gets a pointer to the Object specified by object_number. If the object has
  // previously been accessed, it will retrieve a pointer from the Object cache.
  // If it has not been accessed before, it will first create it. If the object
  // is inside an object stream, it will automatically add the holding object to
  // the cache as well.
  std::shared_ptr<Object> GetObject(int object_number);

  // Returns the main header dictionary for page specified by page_number
  Dictionary GetPageHeader(size_t page_number);

  // Accesses the private member containing object numbers of all page headers.
  std::vector<int> GetPageObjectNumbers() const {return page_object_numbers_;};

 private:
  const std::string file_string_;         // Full contents of file
  std::shared_ptr<const XRef> xref_;      // Pointer to creating XRef object
  std::vector<int> page_object_numbers_;  // The object numbers of page headers

  // This map holds Object pointers. Since some objects may be read
  // multiple times, it is best to store them when they are first created,
  // then return the stored object on request rather than creating a new
  // instance of the object every time it is requested.
  std::unordered_map <int, std::shared_ptr<Object>> object_cache_;

  void BuildDocument_();      // The constructors use this as a common pathway

  // This function effectively builds the pages tree.
  std::vector<int> ExpandKids_(const std::vector<int>& object_numbers);
};

//---------------------------------------------------------------------------//



#endif
