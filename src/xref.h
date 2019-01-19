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

//---------------------------------------------------------------------------//

#define PDFR_XREF

/* This is the third main header in the daisy-chain of #includes that builds up
 * the tools needed to read and parse pdf, after utilities.h and dictionary.h.
 * It also includes a couple of other headers which are needed to decrypt and
 * decode encrypted and compressed streams (streams.h and crypto.h)
 *
 * The cross reference table (xref) is a data structure (or more accurately a
 * group of data structures) that forms part of the pdf file format and allows
 * for the rapid random access of the pdf objects from which a document is
 * comprised. At its simplest, this is a table containing the object number,
 * the generation number of the object, and the number of bytes from the start
 * of the file where that object is located.
 *
 * However, it is not always quite that simple. Firstly, documents can and do
 * have more than one xref that lists different objects. Secondly, the xref
 * can itself be a compressed and encrypted stream which must be found and
 * translated before being read. This means the xref class must have access to
 * decryption and decoding algorithms.
 *
 * Fortunately, the location of the start of an xref table (as number of bytes
 * offset from the start of the file) is given right at the end of a file, just
 * before the %%EOF on the last line. It is thus simple to get to the start of
 * an xref from this number. For a normal uncompressed xref, this takes us to
 * the top of a table which is just read and parsed. At the end of the table is
 * a special dictionary which does not belong to any object. This is the
 * trailer dictionary. If there are other xrefs in the file, this tells us
 * where the next one is, and we can continue to hop around and read the xrefs
 * until none are left and we have a complete "roadmap" of where the objects
 * are in the file.
 *
 * If, however, the xref is located in a stream, things get more complicated.
 * The stream belongs to an object, and the dictionary at the beginning of that
 * object doubles as the trailer dictionary. As well as being possibly
 * encrypted and compressed, the stream containing the xref is usually encoded
 * as a string of bytes which then need to be interpreted using the algorithm
 * normally used for decompressing PNG files. This makes handling xref streams
 * complex enough to warrant their own class.
*/

#include "dictionary.h"
#include "streams.h"
#include "crypto.h"

//---------------------------------------------------------------------------//
// The main xref data member is an unordered map with the key being the object
// number and the value being a struct of named ints as defined here

struct xrefrow
{
  int object,
      startbyte,
      stopbyte,
      in_object;
};

/*---------------------------------------------------------------------------*/

class xref
{
private:
  std::string file;
  std::vector<int> Xreflocations,
                   objenum;
  std::vector<std::string>  Xrefstrings;
  std::vector<bool> XrefsAreStreams;
  void locateXrefs();
  void xrefstrings();
  void xrefIsstream();
  void xrefFromStream(int);
  void xrefFromString(std::string&);
  void buildXRtable();
  std::unordered_map<int, xrefrow> xreftab;
  dictionary TrailerDictionary;
  bytes getPassword(const std::string&, dictionary&);
  void getFilekey(dictionary&);
  void checkKeyR2(dictionary&);
  void checkKeyR3(dictionary&);
  void get_cryptkey();

public:
  xref(){};
  xref(const std::string&);
  bool encrypted;
  std::vector<uint8_t> filekey;
  std::string fs;
  dictionary trailer() {return TrailerDictionary;}
  size_t getStart(int);
  size_t getEnd(int);
  bool isInObject(int);
  size_t inObject(int);
  std::vector<int> getObjects();
  bool objectExists(int);
  std::vector<size_t> getStreamLoc(int);
};

//---------------------------------------------------------------------------//
// The xrefstream class is really just a helper class for xref. It contains
// only private members and functions. Its functions could all sit in the xref
// class, but it has been seperated out to remove clutter and because it
// represents one encapsulated and complex task.

class xrefstream
{
  friend class xref;
  xref* XR;
  dictionary dict,
             subdict;
  std::vector<std::vector<int>> rawMatrix,
                                finalArray,
                                result;
  std::vector<int> arrayWidths,
                   objectIndex,
                   objectNumbers,
                   indexEntries;
  int ncols,
      nrows,
      firstObject,
      predictor,
      objstart;
  void getIndex();
  void getParms();
  void getRawMatrix();
  void diffup();
  void modulotranspose();
  void expandbytes();
  void mergecolumns();
  void numberRows();
  xrefstream(xref*, int objstart);
  std::vector<std::vector<int>> table();
};

//---------------------------------------------------------------------------//

#endif
