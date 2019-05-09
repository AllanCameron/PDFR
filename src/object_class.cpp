//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR object_class implementation file                                    //
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

#include "object_class.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// The main object creator class. It needs a pointer to the xref and a number
// representing the object's number as set out in the xref table.

Object::Object(shared_ptr<const XRef> t_xref, int t_object_number) :
  xref_(t_xref),
  object_number_(t_object_number),
  stream_location_({0, 0})
{
  // Find start and end of object
  size_t start = xref_->GetObjectStartByte(object_number_);
  size_t stop  = xref_->GetObjectEndByte(object_number_);

  // We check to see if the object has a header dictionary by finding '<<'
  if(xref_->File()->substr(start, 20).find("<<") == string::npos)
  {
    // No dictionary found - make blank dictionary for header
    header_ = Dictionary();

    // Finds start and length of contents
    size_t content_start = xref_->File()->find(" obj", start) + 4;
    stream_location_ = {content_start, stop - content_start};
  }

  else // Else the object has a header dictionary
  {
    header_ = Dictionary(xref_->File(), start);

    // Find the stream (if any)
    stream_location_ = xref_->GetStreamLocation(start);

    // The object may contain an object stream that needs unpacked
    if(header_.GetString("/Type") == "/ObjStm")
    {
      // Get the object stream
      ReadStreamFromStreamLocations();

      // Index the objects in the stream
      IndexObjectStream();
    }
  }
}

//---------------------------------------------------------------------------//
// Object streams start with a group of integers representing the object
// numbers and the byte offset of each object relative to the stream. This
// method reads the objects and their positions in the stream, indexing them
// for later retrieval.

void Object::IndexObjectStream()
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
  if(index.empty()) throw runtime_error("Couldn't parse object stream");

  // We now set up a loop that determines which numbers are object numbers and
  // which are byte offsets
  for(size_t byte_length, i = 1; i < index.size(); i += 2)
  {
    if(i == (index.size() - 1)) byte_length = stream_string.size() - index[i];
    else byte_length = index[i + 2] - index[i];
    auto&& index_pair = make_pair(index[i] + startbyte, byte_length);
    object_stream_index_[index[i - 1]] = index_pair;
  }
}

/*---------------------------------------------------------------------------*/
// The constructor for in-stream objects. This is called automatically by the
// main object constructor if the main object constructor determines that the
// requested object lies inside the stream of another object

Object::Object(shared_ptr<Object> t_holder, int t_object_number):
  xref_(t_holder->xref_),
  object_number_(t_object_number),
  stream_location_({0, 0})
{
  auto finder = t_holder->object_stream_index_.find(object_number_);

  if(finder == t_holder->object_stream_index_.end())
  {
    throw runtime_error("Object not found in stream");
  }

  auto index_position = finder->second.first;
  auto index_length   = finder->second.second;
  auto stream_string  = t_holder->stream_.substr(index_position, index_length);

  // Most stream objects consist of just a dictionary
  if(stream_string[0] == '<')
  {
    header_ = Dictionary(make_shared<string>(stream_string));
    stream_ = "";             // stream objects don't have their own stream
  }
  else // The object is not a dictionary - maybe just an array or int etc
  {
    header_ = Dictionary();   // gets an empty dictionary as header
    stream_ = stream_string;  // Call the contents a stream for ease

    // Annoyingly, some "objects" in an object stream are just pointers
    // to other objects. This is pointless but does happen and needs to
    // be handled by recursively calling the constructor
    if(stream_.size() < 15 && stream_.find(" R", 0) < 15)
    {
      size_t new_number = ParseReferences(stream_)[0];
      size_t holder = xref_->GetHoldingNumberOf(new_number);
      if(holder == 0) *this = Object(xref_, new_number);
      else *this = Object(make_shared<Object>(xref_, holder), new_number);
      this->object_number_ = t_object_number;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Simple public getter for the header dictionary

Dictionary Object::GetDictionary()
{
  return header_;
}

/*---------------------------------------------------------------------------*/
// We have to create the stream on the fly when it is needed rather than
// calculating and storing all the streams upon document creation

string Object::GetStream()
{
  // If the stream has not already been processed, do it now
  if(stream_.empty()) ReadStreamFromStreamLocations();
  return stream_;
}

/*---------------------------------------------------------------------------*/
// We will keep all stream processing in one place for easier debugging and
// future development

void Object::ApplyFilters()
{
  // Decrypt if necessary
  if(xref_->IsEncrypted()) xref_->Decrypt(stream_, object_number_, 0);

  // Read filters
  string filters = header_.GetString("/Filter");

  // Apply filters
  if(filters.find("/FlateDecode") != string::npos) FlateDecode(stream_);

}

/*---------------------------------------------------------------------------*/
// Turns stream locations into unencrypted, uncompressed stream

void Object::ReadStreamFromStreamLocations()
{
  // Read the string from the current positions
  stream_ = xref_->File()->substr(stream_location_[0], stream_location_[1]);

  // Apply necessary decryption and deflation
  ApplyFilters();
}
