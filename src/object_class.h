//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Object header file                                                  //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_OBJECT

//---------------------------------------------------------------------------//

#define PDFR_OBJECT

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
  // Get pdf object from a given object number
  Object(std::shared_ptr<const XRef> xref_ptr, int object_number);

  // Get stream object from inside the holding object, given object number
  Object(std::shared_ptr<Object> holding_object_ptr, int object_number);

  // Default constructor
  Object(){};

  // Returns an Object's stream as a string
  std::string GetStream();

  // Returns an Object's Dictionary
  Dictionary  GetDictionary();

 private:
  std::shared_ptr<const XRef> xref_;      // Pointer to creating xref
  int object_number_;                     // The object knows its own number
  Dictionary header_;                     // The object's dictionary
  std::string stream_;                    // The object's stream or contents
  std::vector<size_t> stream_location_;   // Start position and length of stream

  // A lookup of start / stop positions of the objects within an object stream
  std::unordered_map<int, std::pair<int, int>> object_stream_index_;

  // private methods
  void IndexObjectStream_();
  void ApplyFilters_();
  void ReadStreamFromStreamLocations_();
};

//---------------------------------------------------------------------------//

#endif
