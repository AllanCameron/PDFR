//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR document header file                                                //
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

#ifndef PDFR_DOCUMENT

//---------------------------------------------------------------------------//

#define PDFR_DOCUMENT

/* This is the fifth header file in a daisy-chain of headers that build up the
 * tools required to parse pdfs. It follows just after the definition of the
 * object_class.
 *
 * The tools already in place have abstracted away decryption, decompression,
 * bytewise navigation of the file and parsing of dictionaries. The job of the
 * document class is therefore to act as an interface to use the pdf objects
 * from which we build up logical structures such as fonts, xobjects and pages.
 *
 * The previous classes have been encapsulated as far as possible to be able to
 * work in isolation with minimal knowledge of each other. The document class
 * in contrast acts as a creator, container and user of these objects.
 *
 * Each document will have one and only one xref class. Instead of a pointer to
 * the xref as in other classes, the xref is actually a data member of the
 * document class. PDF objects are created and stored in a map for easy access.
 * The file string is stored here and any other class that needs to read the
 * file accesses a pointer to the filestring held in the document class.
 *
 * The document class is therefore self-contained, in that after the initial
 * step of reading in the file, it has everything in needs to build up its
 * own components and interface. The logical PDF structures we go on to build
 * only need to know about the document class, and can use it as the interface
 * they need. They "see" the pdf as a random access collection of numbered
 * objects with key:value dictionaries and uncompressed streams without being
 * concerned about how that is implemented.
 *
 * The document also needs to have an outline of its own logical structure,
 * in terms of the pages it contains and where they are located. Part of the
 * task of document creation is therefore to count and locate the objects
 * that act as page descriptors. It does this by finding the catalog
 * dictionary and then following pointers to dictionaries that contain
 * individual page headers. There is then a "getter" function for other classes
 * to access the dictionary pertaining to a particular page
 */

#include "object_class.h"
#include<memory>


//---------------------------------------------------------------------------//
// The public interface of the document class comprises constructors and two
// member functions - one to return any object from the pdf and one to retrieve
// a specific page header.
class font;

class document
{
  using Node = std::shared_ptr<tree_node<int>>;

public:
  // constructors
  document(const std::string&);          // create doc from filepath
  document(const std::vector<uint8_t>&);  // create doc from raw data
  document();                                     // default constructor

  // public member functions
  std::shared_ptr<object_class> get_object(int); // creates object and
                                                 // returns pointer
  dictionary get_page_header(int);  // returns header dictionary for page p

  inline std::vector<int> get_page_object_numbers()
  {
    return m_page_object_numbers;
  };

private:
  // private data members
  std::string m_file_path;            // Path used to create file (if used)
  const std::string m_file_string;    // Full contents of file
  std::shared_ptr<const xref> m_xref; // Contains the xref object for navigation
  dictionary m_page_directory;        // dict containing pointers to pages
  dictionary m_catalog;               // The pdf catalog dictionary
  std::vector<int> m_page_object_numbers;

  // This map holds object_class objects. Since some objects may be read
  // multiple times, it is best to store them when they are first created,
  // then return the stored object on request rather than creating a new
  // instance of the object every time it is requested.
  std::unordered_map <int, std::shared_ptr<object_class>> m_objects;

  // private member functions used in construction only
  void read_catalog();        // finds and stores the catalog dictionary
  void read_page_directory(); // finds and stores the /Pages dictionary
  void build_document();      // the constructors use this as a common pathway
  void expand_kids(const std::vector<int>&, Node); // descendants
};                                                 // of /Pages object

//---------------------------------------------------------------------------//



#endif
