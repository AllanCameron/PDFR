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
 *
 * The document class is therefore self-contained, in that after the initial
 * step of reading in the file, it has everything in needs to build up its
 * own components and interface. The logical PDF structures we go on to build
 * only need to know about the document class, and can use it as the interface
 * they need to build up objects purely in terms of asking for objects by
 * number, interrogating the object's dictionary, or by reading its stream
 * contents as decrypted and uncompressed strings.
 *
 * The document also needs to have an outline of its own logical structure,
 * in terms of the pages it contains and where they are located. Part of the
 * task of document creation is therefore to count and locate the objects
 * that act as page descriptors. It does this by finding the catalogue
 * dictionary and then following pointers to dictionaries that contain
 * individual page headers.
 */

#include "object_class.h"

//---------------------------------------------------------------------------//

class document
{
public:
  document(const std::string& filename);
  document(const std::vector<uint8_t>& rawfile);
  document();
  std::ifstream *fileCon;
  size_t filesize;
  std::string file, filestring;
  bool linearized;
  std::vector<dictionary> pageheaders;
  std::unordered_map <int, object_class> objects;
  dictionary trailer;
  dictionary catalogue;
  object_class pagedir;
  xref Xref;
  object_class getobject(int objnum);
  std::vector<int> expandContents(std::vector<int> objnums);

  // member functions
private:
  void get_file();
  std::string subfile(int startbyte, int len);
  void getCatalogue();
  void getPageDir();
  void isLinearized();
  void buildDoc();
  std::vector<int> expandKids(std::vector<int> objnums);
  void getPageHeaders();
};

#endif
