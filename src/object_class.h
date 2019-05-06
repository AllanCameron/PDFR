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
 * object. Each Object object is made of two main items of data: a
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

class Object
{
public:
  // constructors
  Object(std::shared_ptr<const XRef>, int);  // get direct object
  Object(std::shared_ptr<Object>, int);
  Object(){}; // default constructor (needed for document class to
                    // initialize its vector of objects)

  // public member functions
  bool has_stream();               // returns has_stream boolean
  std::string get_stream();        // returns stream as string
  Dictionary get_dictionary();    // returns header as dictionary object

private:
  // private data members
  std::shared_ptr<const XRef> m_xref; // Pointer to xref allows data to be found
  int m_object_number;            // The object knows its own number
  Dictionary m_header;              // The object's dictionary
  std::string m_stream;             // The object's stream or contents
  std::array<size_t, 2> m_stream_location; // start/stop for stream position

  // index for object stream holders
  std::unordered_map<int, std::pair<int, int>> m_object_stream_index;

  // private methods
  void index_object_stream();       // gets index of objects held by a stream
  void read_stream_from_stream_locations();
  void apply_filters();
};

//---------------------------------------------------------------------------//

#endif
