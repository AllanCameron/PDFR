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

object_class::object_class(shared_ptr<const xref> Xref, int object_num) :
  XR(Xref), number(object_num), m_streampos({0, 0})
{
  // Find start and end of object
  size_t startbyte = XR->get_object_start_byte(object_num);
  size_t stopbyte  = XR->get_object_end_byte(object_num);

  // We check to see if the object has a header dictionary by finding '<<'
  if(XR->file()->substr(startbyte, 20).find("<<") == string::npos)
  {
    // No dictionary found - make blank dictionary for header
    header = Dictionary();

    // find start and end of contents
    m_streampos = {XR->file()->find(" obj", startbyte) + 4, stopbyte - 1};
  }

  else // Else the object has a header dictionary
  {
    // Construct the dictionary
    header = Dictionary(XR->file(), startbyte);

    // Find the stream (if any)
    m_streampos = XR->get_stream_location(startbyte);

    // The object may contain an object stream that needs unpacked
    if(header.get_string("/Type") == "/ObjStm")
    {
      // Get the object stream
      stream = XR->file()->substr(m_streampos[0],
                                  m_streampos[1] - m_streampos[0]);

      // Decrypt if necessary
      if(XR->is_encrypted()) XR->decrypt(stream, number, 0);

      // De-deflate if necessary
      if(header.get_string("/Filter").find("/FlateDecode") != string::npos)
      {
        FlateDecode(stream);
      }

      // Index the objects in the stream
      indexObjectStream();
    }
  }
}

//---------------------------------------------------------------------------//
// object streams start with a group of integers representing the object
// numbers and the byte offset of each object relative to the stream. This
// method reads the objects and their positions in the stream, indexing them
// for later retrieval.

void object_class::indexObjectStream()
{
  // Get the first character that is not a digit or space
  int startbyte = stream.find_first_not_of("\n\r\t 0123456789");

  // Now get the substring with the objects proper...
  string s(stream.begin() + startbyte, stream.end());

  // ...and the substring with the registration numbers...
  string pre(stream.begin(), stream.begin() + startbyte - 1);

  // extract these numbers to a vector
  vector<int> index = parse_ints(pre);

  // If this is empty, something has gone wrong.
  if(index.empty()) throw runtime_error("Couldn't parse object stream");

  // We now set up a loop that determines which numbers are object numbers and
  // which are byte offsets
  for(size_t byte_length, i = 1; i < index.size(); i += 2)
  {
    if(i == (index.size() - 1)) byte_length = s.size() - index[i];

    else byte_length = index[i + 2] - index[i];

    objstmIndex[index[i - 1]] = make_pair(index[i] + startbyte, byte_length);
  }
}

//---------------------------------------------------------------------------//
// Gets the string containing the stream object

string object_class::objFromStream(int objnum)
{
  auto i = objstmIndex.find(objnum);

  if(i == objstmIndex.end()) throw runtime_error("Object not found in stream");

  return stream.substr(i->second.first, i->second.second);
}

/*---------------------------------------------------------------------------*/
// The constructor for in-stream objects. This is called automatically by the
// main object constructor if the main object constructor determines that the
// requested object lies inside the stream of another object

object_class::object_class(shared_ptr<object_class> holder, int objnum)
  :  XR(holder->XR), number(objnum), m_streampos({0, 0})
{
  string H = holder->objFromStream(objnum);

  // Most stream objects consist of just a dictionary
  if(H[0] == '<')
  {
    header = Dictionary(make_shared<string>(H)); // read dict as object's header
    stream = "";             // stream objects don't have their own stream
  }
  else // The object is not a dictionary - maybe just an array or int etc
  {
    header = Dictionary();   // gets an empty dictionary as header
    stream = H;              // We'll call the contents a stream for ease

    // Annoyingly, some "objects" in an object stream are just pointers
    // to other objects. This is pointless but does happen and needs to
    // be handled by recursively calling the main creator function
    if(stream.size() < 15 && stream.find(" R", 0) < 15)
    {
      size_t oldnumber = this->number;
      size_t newobjnum = parse_references(stream)[0];
      size_t newholder = XR->get_holding_object_number_of(newobjnum);

      if(newholder == 0)
      {
        *this = object_class(XR, newobjnum);
      }
      else
      {
        shared_ptr<object_class> h = make_shared<object_class>(XR, newholder);
        *this = object_class(h, newobjnum);
      }

      this->number = oldnumber;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Simple public getter for the header dictionary

Dictionary object_class::get_dictionary()
{
  return header;
}

/*---------------------------------------------------------------------------*/
// We have to create the stream on the fly when it is needed rather than
// calculating and storing all the streams upon document creation

string object_class::get_stream()
{
  // no stream - return empty string
  if(!has_stream()) return string {};

  // stream already calculated - return
  else if(!stream.empty()) return stream;

  // get the stream from known stream locations
  else
  {
    stream = XR->file()->substr(m_streampos[0],
                                m_streampos[1] - m_streampos[0]);
  }

  // decrypt if necessary
  if(XR->is_encrypted()) XR->decrypt(stream, number, 0);

  // de-deflate if necessary
  if(header.get_string("/Filter").find("/FlateDecode", 0) != string::npos)
  {
    FlateDecode(stream);
  }

  return stream;
}

/*---------------------------------------------------------------------------*/
// Simple public getter that is a check of whether the object has a stream

bool object_class::has_stream()
{
  return this->stream.empty();
}


