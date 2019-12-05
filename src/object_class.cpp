//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Object implementation file                                          //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "dictionary.h"
#include "streams.h"
#include "deflate.h"
#include "xref.h"
#include "object_class.h"
#include<iostream>

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// The main object creator class. It needs a pointer to the xref and a number
// representing the object's number as set out in the xref table.

Object::Object(shared_ptr<const XRef> p_xref, int p_object_number) :
  xref_(p_xref),
  object_number_(p_object_number),
  raw_stream_(),
  stream_index_(make_shared<unordered_map<int, pair<int, int>>>())
{
  // Find start and end of object
  size_t start = xref_->GetObjectStartByte(object_number_);
  size_t stop  = xref_->GetObjectEndByte(object_number_);

  if (xref_->File()->substr(start, 20).find("%") != string::npos)
  {
    start = xref_->File()->substr(start, 200).find("\n") + start;
  }

  // We check to see if the object has a header dictionary by finding '<<'
  if (xref_->File()->substr(start, 20).find("<<") == string::npos)
  {
    // No dictionary found - make blank dictionary for header
    header_ = make_shared<Dictionary>(Dictionary());

    // Finds start and length of contents
    size_t c_start = xref_->File()->find(" obj", start) + 4;
    raw_stream_ = {xref_->File()->c_str() + c_start, stop - c_start};
  }

  else // Else the object has a header dictionary
  {
    header_ = make_shared<Dictionary>(Dictionary(xref_->File(), start));
    // Find the stream (if any)
    raw_stream_ = xref_->GetStreamLocation(start);

    // The object may contain an object stream that needs unpacked
    if (header_->GetString("/Type") == "/ObjStm")
    {
      // Get the object stream
      ReadStream_();

      // Index the objects in the stream
      IndexObjectStream_();
    }
  }
}

//---------------------------------------------------------------------------//
// Object streams start with a group of integers representing the object
// numbers and the byte offset of each object relative to the stream. This
// method reads the objects and their positions in the stream, indexing them
// for later retrieval.

void Object::IndexObjectStream_()
{
  // Get the first character that is not a digit or space
  int startbyte = stream_.find_first_not_of("\n\r\t 0123456789");

  // Now get the substring with the objects proper...
  string stream_string(stream_.begin() + startbyte, stream_.end());

  // ...and the substring with the registration numbers...
  string index_string(stream_.begin(), stream_.begin() + startbyte - 1);

  // extract these numbers to a vector
  vector<int> index = ParseInts(index_string);

  // If this is empty, something has gone wrong.
  if (index.empty()) throw runtime_error("Couldn't parse object stream");

  // We now set up a loop that determines which numbers are object numbers and
  // which are byte offsets
  for (size_t byte_length, i = 1; i < index.size(); i += 2)
  {
    if (i == (index.size() - 1)) byte_length = stream_string.size() - index[i];
    else byte_length = index[i + 2] - index[i];
    auto&& index_pair = make_pair(index[i] + startbyte, byte_length);
    (*stream_index_)[index[i - 1]] = index_pair;
  }
}

/*---------------------------------------------------------------------------*/
// The constructor for in-stream objects. This is called automatically by the
// main object constructor if the main object constructor determines that the
// requested object lies inside the stream of another object

Object::Object(shared_ptr<Object> p_holder, int p_object_number):
  xref_(p_holder->xref_),
  object_number_(p_object_number),
  raw_stream_()
{
  auto finder = p_holder->stream_index_->find(object_number_);
  if (finder == p_holder->stream_index_->end())
  {
    throw runtime_error("Object not found in stream");
  }

  auto index_position = finder->second.first;
  auto index_length   = finder->second.second;
  auto stream_string  = p_holder->stream_.substr(index_position, index_length);

  // Most stream objects consist of just a dictionary
  if (stream_string[0] == '<')
  {
    header_ = make_shared<Dictionary>(make_shared<string>(stream_string));
    stream_ = "";             // stream objects don't have their own stream
  }
  else // The object is not a dictionary - maybe just an array or int etc
  {
    header_ = make_shared<Dictionary>(Dictionary());// empty header
    stream_ = stream_string;  // Call the contents a stream for ease

    // Annoyingly, some "objects" in an object stream are just pointers
    // to other objects. This is pointless but does happen and needs to
    // be handled by recursively calling the constructor
    if (stream_.size() < 15 && stream_.find(" R", 0) < 15)
    {
      size_t new_number = ParseReferences(stream_)[0];
      size_t holder = xref_->GetHoldingNumberOf(new_number);
      if (holder == 0) *this = Object(xref_, new_number);
      else *this = Object(make_shared<Object>(xref_, holder), new_number);
      this->object_number_ = p_object_number;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Simple public getter for the header dictionary

Dictionary Object::GetDictionary()
{
  return *header_;
}

/*---------------------------------------------------------------------------*/
// We have to create the stream on the fly when it is needed rather than
// calculating and storing all the streams upon document creation

string Object::GetStream()
{
  // If the stream has not already been processed, do it now
  if (stream_.empty()) ReadStream_();
  return stream_;
}

/*---------------------------------------------------------------------------*/
// We will keep all stream processing in one place for easier debugging and
// future development

void Object::ReadStream_()
{

  string filters = header_->GetString("/Filter");
  bool is_flatedecode = filters.find("/FlateDecode") != string::npos;

  // Decrypt if necessary
  if (xref_->IsEncrypted())
  {
    stream_ = xref_->Decrypt(raw_stream_, object_number_, 0);
    if (is_flatedecode) stream_ = FlateDecode(&stream_);
  }
  else
  {
    if (is_flatedecode) stream_ = FlateDecode(raw_stream_);
  }
}

