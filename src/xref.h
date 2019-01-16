//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR xref header file                                                    //
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


#ifndef PDFR_XREF
#define PDFR_XREF

#include "dictionary.h"
#include "streams.h"
#include "crypto.h"

typedef std::vector<std::vector<int>> XRtab;

struct xrefrow { int object, startbyte, stopbyte, in_object; };

/*---------------------------------------------------------------------------*/

class xref
{
private:
  std::string file;
  std::vector<int> Xreflocations;
  std::vector<std::string>  Xrefstrings;
  std::vector<bool> XrefsAreStreams;
  void locateXrefs();
  void xrefstrings();
  void xrefIsstream();
  void xrefFromStream(int xrefloc);
  void xrefFromString(std::string& s);
  void buildXRtable();
  std::unordered_map<int, xrefrow> xreftab;
  std::vector<int> objenum;
  dictionary TrailerDictionary;
  std::vector<uint8_t> getPassword(const std::string& key, dictionary& encdict);
  void getFilekey(dictionary& encdict);
  void checkKeyR2(dictionary& encdict);
  void checkKeyR3(dictionary& encdict);
  void get_cryptkey();

public:
  xref(){};
  xref(const std::string&);
  bool encrypted;
  std::vector<uint8_t> filekey;
  std::string fs;
  dictionary trailer() {return TrailerDictionary;}
  size_t getStart(int objnum);
  size_t getEnd(int objnum);
  bool isInObject(int objnum);
  size_t inObject(int objnum);
  std::vector<int> getObjects();
  bool objectExists(int objnum);
  std::vector<size_t> getStreamLoc(int objstart);
};

class xrefstream
{
  xref* XR;
  dictionary dict, subdict;
  std::vector<std::vector<int>> rawMatrix, finalArray, result;
  std::vector<int> arrayWidths, objectIndex, objectNumbers, indexEntries;
  int ncols, nrows, firstObject, predictor, objstart;
  void getIndex();
  void getParms();
  void getRawMatrix();
  void diffup();
  void modulotranspose();
  void expandbytes();
  void mergecolumns();
  void numberRows();

public:
  xrefstream(xref*, int objstart);
  std::vector<std::vector<int>> table();
};



#endif
