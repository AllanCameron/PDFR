//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR object_class header file                                            //
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

#ifndef PDFR_OC

//---------------------------------------------------------------------------//

#define PDFR_OC

/* This is the fourth header file in a daisy-chain of main headers which builds
 * up an interface for parsing pdf files. It comes directly after xref.h and is
 * the last step before the main document class is declared.
 *
 * The object class comprises the data and functions needed to represent a pdf
 * object. Each object_class object is made of two main items of data: a
 * dictionary (which can be empty), and a pair of size_t indicating the offset
 * of the stream's start and stop. The reason we don't just build the stream
 * is that decryption and deflation of large streams is computationally
 * expensive, and we should only do it on request. As an object may be requested
 * more than once however, if we have gone to the trouble of calculating the
 * stream, it is stored as a private data member.
 *
 * Of course, for objects to have this memory of their state, they need to
 * stay in scope from creation until the program exits. This is done by keeping
 * a vector of retrieved objects in the document class, which persists through
 * the lifetime of the program.
 *
 * The job of finding the object, parsing its dictionary and decoding its stream
 * is abstracted away using this class, so that pdf objects can be directly
 * interrogated for key:value pairs and their streams can be parsed as plain
 * text where appropriate. This means that logical structures such as pages,
 * fonts and form objects can be built by interfacing directly with pdf objects
 * rather than indirectly through byte offsets and binary streams
 */

#include "xref.h"

//---------------------------------------------------------------------------//

class object_class
{

public:

  // constructors

  object_class(xref*, int objnum);  // get direct object
  object_class(std::shared_ptr<object_class>, int objnum);
  object_class(){}; // default constructor (needed for document class to
                    // initialize its vector of objects)

  // public member functions

  bool hasStream();               // returns has_stream boolean
  std::string getStream();        // returns stream as string
  dictionary getDict();           // returns header as dictionary object
  std::string objFromStream(int); // returns object from this object's stream


private:
  // private data members

  xref* XR;                       // Pointer to xref allows data to be found
  int number,                     // The object knows its own number
      startpos;                   // The object knows its own starting position
  dictionary header;              // The object's dictionary
  std::string stream;             // The object's stream or contents
  bool has_stream;                // Records whether stream is zero length
  std::vector<size_t> streampos;  // start/stop file offsets for stream position

  // index for object stream holders
  std::unordered_map<int, std::pair<int, int>> objstmIndex;

  // private methods

  void indexObjectStream();       // gets index of objects held by a stream

};

//---------------------------------------------------------------------------//

#endif
