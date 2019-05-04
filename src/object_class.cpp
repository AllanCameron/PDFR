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

Object::Object(shared_ptr<const xref> t_xref, int t_object_number) :
  m_xref(t_xref),
  m_object_number(t_object_number),
  m_stream_location({0, 0})
{
  // Find start and end of object
  size_t startbyte = m_xref->get_object_start_byte(m_object_number);
  size_t stopbyte  = m_xref->get_object_end_byte(m_object_number);

  // We check to see if the object has a header dictionary by finding '<<'
  if(m_xref->file()->substr(startbyte, 20).find("<<") == string::npos)
  {
    // No dictionary found - make blank dictionary for header
    m_header = Dictionary();

    // find start and end of contents
    m_stream_location = {m_xref->file()->find(" obj", startbyte) + 4,
                         stopbyte - 1};
  }

  else // Else the object has a header dictionary
  {
    // Construct the dictionary
    m_header = Dictionary(m_xref->file(), startbyte);

    // Find the stream (if any)
    m_stream_location = m_xref->get_stream_location(startbyte);

    // The object may contain an object stream that needs unpacked
    if(m_header.get_string("/Type") == "/ObjStm")
    {
      // Get the object stream
      read_stream_from_stream_locations();

      // Index the objects in the stream
      index_object_stream();
    }
  }
}

//---------------------------------------------------------------------------//
// object streams start with a group of integers representing the object
// numbers and the byte offset of each object relative to the stream. This
// method reads the objects and their positions in the stream, indexing them
// for later retrieval.

void Object::index_object_stream()
{
  // Get the first character that is not a digit or space
  int startbyte = m_stream.find_first_not_of("\n\r\t 0123456789");

  // Now get the substring with the objects proper...
  string stream_string(m_stream.begin() + startbyte, m_stream.end());

  // ...and the substring with the registration numbers...
  string index_string(m_stream.begin(), m_stream.begin() + startbyte - 1);

  // extract these numbers to a vector
  vector<int> index = parse_ints(index_string);

  // If this is empty, something has gone wrong.
  if(index.empty()) throw runtime_error("Couldn't parse object stream");

  // We now set up a loop that determines which numbers are object numbers and
  // which are byte offsets
  for(size_t byte_length, i = 1; i < index.size(); i += 2)
  {
    if(i == (index.size() - 1)) byte_length = stream_string.size() - index[i];
    else byte_length = index[i + 2] - index[i];
    auto&& index_pair = make_pair(index[i] + startbyte, byte_length);
    m_object_stream_index[index[i - 1]] = index_pair;
  }
}

/*---------------------------------------------------------------------------*/
// The constructor for in-stream objects. This is called automatically by the
// main object constructor if the main object constructor determines that the
// requested object lies inside the stream of another object

Object::Object(shared_ptr<Object> t_holder, int t_object_number):
  m_xref(t_holder->m_xref),
  m_object_number(t_object_number),
  m_stream_location({0, 0})
{
  auto finder = t_holder->m_object_stream_index.find(m_object_number);

  if(finder == t_holder->m_object_stream_index.end())
  {
    throw runtime_error("Object not found in stream");
  }

  auto index_position = finder->second.first;
  auto index_length = finder->second.second;
  auto stream_string = t_holder->m_stream.substr(index_position, index_length);

  // Most stream objects consist of just a dictionary
  if(stream_string[0] == '<')
  {
    m_header = Dictionary(make_shared<string>(stream_string));
    m_stream = "";             // stream objects don't have their own stream
  }
  else // The object is not a dictionary - maybe just an array or int etc
  {
    m_header = Dictionary();   // gets an empty dictionary as header
    m_stream = stream_string;  // Call the contents a stream for ease

    // Annoyingly, some "objects" in an object stream are just pointers
    // to other objects. This is pointless but does happen and needs to
    // be handled by recursively calling the main creator function
    if(m_stream.size() < 15 && m_stream.find(" R", 0) < 15)
    {
      size_t new_number = parse_references(m_stream)[0];
      size_t holder = m_xref->get_holding_number_of(new_number);
      if(holder == 0) *this = Object(m_xref, new_number);
      else *this = Object(make_shared<Object>(m_xref, holder), new_number);
      this->m_object_number = t_object_number;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Simple public getter for the header dictionary

Dictionary Object::get_dictionary()
{
  return m_header;
}

/*---------------------------------------------------------------------------*/
// We have to create the stream on the fly when it is needed rather than
// calculating and storing all the streams upon document creation

string Object::get_stream()
{
  // If the stream has not already been processed, do it now
  if(m_stream.empty()) read_stream_from_stream_locations();

  return m_stream;
}

/*---------------------------------------------------------------------------*/
// We will keep all stream processing in one place for easier debugging and
// future development

void Object::apply_filters()
{
  // Decrypt if necessary
  if(m_xref->is_encrypted()) m_xref->decrypt(m_stream, m_object_number, 0);

  // Read filters
  string filters = m_header.get_string("/Filter");

  // Apply filters
  if(filters.find("/FlateDecode") != string::npos) FlateDecode(m_stream);

}

/*---------------------------------------------------------------------------*/
// Simple public getter that is a check of whether the object has a stream

bool Object::has_stream()
{
  return this->m_stream.empty();
}


/*---------------------------------------------------------------------------*/
// Turns stream locations into unencrypted, uncompressed stream

void Object::read_stream_from_stream_locations()
{
  // Find the stream's start position and length
  int stream_length = m_stream_location[1] - m_stream_location[0];
  int stream_start  = m_stream_location[0];

  // Read the string from the current positions
  m_stream          = m_xref->file()->substr(stream_start, stream_length);

  // Apply necessary decryption and deflation
  apply_filters();
}
