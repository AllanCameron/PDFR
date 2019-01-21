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
#define PDFR_DOCUMENT

#include "object_class.h"

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
  std::vector<uint8_t> filekey;
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
