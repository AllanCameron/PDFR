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

object_class::object_class(shared_ptr<const xref> Xref, int objnum) :
  XR(Xref), number(objnum), has_stream(false), streampos({0, 0})
{
  size_t startbyte = XR->getStart(objnum);  // Finds start of obj
  size_t stopbyte  = XR->getEnd(objnum);    // Finds endobj

  // We check to see if the object has a header dictionary by finding '<<'
  if(XR->docpointer()->substr(startbyte, 20).find("<<") == string::npos)
  {
    // No dictionary found
    header = dictionary(); // make blank dictionary for header
    // find start of contents
    streampos[0] = XR->docpointer()->find(" obj", startbyte) + 4;
    // find end of contents
    streampos[1] = stopbyte - 1;
    // ensure the resulting "stream" has positive length
    // The scare quotes are there because it is not a true stream, but
    // direct contents such as a string, array or just an int
    has_stream = streampos[1] > streampos[0];
  }
  else
  {
    // The object has a header dictionary
    header = dictionary(XR->docpointer(), startbyte); // construct the dict
    streampos = XR->getStreamLoc(startbyte);   // find the stream (if any)
    has_stream = streampos[1] > streampos[0];  // record stream's existence
    if(header.get("/Type") == "/ObjStm")
    {
      stream = XR->docpointer()->substr(streampos[0],
                              streampos[1] - streampos[0]);
      if(XR->isEncrypted()) // decrypt if necessary
        XR->decrypt(stream, number, 0);
      if(header.get("/Filter").find("/FlateDecode", 0) != string::npos)
        stream = FlateDecode(stream); // de-deflate if necessary
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
  int startbyte = 0;
  // This loop tells us where the boundary between the registration numbers
  // and the objects proper begins using the symbol_type function declared in
  // utilities.h
  for(auto i : stream)
  {
    char a = symbol_type(i);
    if(a != ' ' && a != 'D') break;
    startbyte++;
  }
  // Now get the substring with the objects proper...
  std::string s(stream.begin() + startbyte, stream.end());

  // ...and the substring with the registration numbers...
  std::string pre(stream.begin(), stream.begin() + startbyte - 1);

  // .. from which we extract the numbers as a vector.
  std::vector<int> numarray = getints(pre);

  // If this is empty, something has gone wrong.
  if(numarray.empty()) throw runtime_error("Couldn't parse object stream");

  // We now set up a loop that determines which numbers are object numbers and
  // which are byte offsets
  int objnum, bytenum, bytelen;
  objnum = bytenum = bytelen = 0;
  for(size_t i = 0; i < numarray.size(); i++)
  {
    if(i % 2 == 0)
      objnum = numarray[i]; // even entries are object numbers
    if(i % 2 == 1)
    {
      bytenum = numarray[i] + startbyte; // odd entries are offsets
      if(i == (numarray.size() - 1))
        bytelen = s.size() - numarray[i];
      else
        bytelen = numarray[i + 2] - numarray[i];
      objstmIndex[objnum] = make_pair(bytenum, bytelen);
    }
  }
}

//---------------------------------------------------------------------------//
// Gets the string containing the stream object

string object_class::objFromStream(int objnum)
{
  if(objstmIndex.find(objnum) == objstmIndex.end())
    throw runtime_error("Object not found in stream");
  int pos = objstmIndex[objnum].first;
  int len = objstmIndex[objnum].second;
  return stream.substr(pos, len);
}

/*---------------------------------------------------------------------------*/
// The constructor for in-stream objects. This is called automatically by the
// main object constructor if the main object constructor determines that the
// requested object lies inside the stream of another object

object_class::object_class(shared_ptr<object_class> holder, int objnum)
  :  XR(holder->XR), number(objnum), streampos({0, 0})
{
  std::string H = holder->objFromStream(objnum);
  if(H[0] == '<') // Most stream objects consist of just a dictionary
  {
    header = dictionary(&H); // read the dictionary as the object's header
    stream = "";             // stream objects don't have their own stream
    has_stream = false;      // stream objects don't have their own stream
  }
  else // The object is not a dictionary - maybe just an array or int etc
  {
    header = dictionary();   // gets an empty dictionary as header
    stream = H;              // We'll call the contents a stream for ease
    has_stream = true;       // We'll call the contents a stream for ease
    // Annoyingly, some "objects" in an object stream are just pointers
    // to other objects. This is pointless but does happen and needs to
    // be handled by recursively calling the main creator function
    if(stream.size() < 15 && stream.find(" R", 0) < 15)
    {
      size_t newobjnum = getObjRefs(stream)[0];
      size_t newholder = XR->inObject(newobjnum); // ensure no holding object
      if(newholder == 0) *this = object_class(XR, newobjnum);
      else
      {
        size_t oldnumber = this->number;
        shared_ptr<object_class> holding =
          make_shared<object_class>(XR, newholder);
        *this = object_class(holding, newobjnum);
        this->number = oldnumber;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// Simple public getter for the header dictionary

dictionary object_class::getDict()
{
  return header;
}

/*---------------------------------------------------------------------------*/
// We have to create the stream on the fly when it is needed rather than
// calculating and storing all the streams upon document creation

std::string object_class::getStream()
{
  if(!hasStream()) return ""; // no stream - return empty string
  else if(!stream.empty()) return stream; // stream already calculated - return
  else // get the stream from known stream locations
    stream = XR->docpointer()->substr(streampos[0], streampos[1] -streampos[0]);
  if(XR->isEncrypted()) // decrypt if necessary
    XR->decrypt(stream, number, 0);
  if(header.get("/Filter").find("/FlateDecode", 0) != string::npos)
    stream = FlateDecode(stream); // de-deflate if necessary
  return stream;
}

/*---------------------------------------------------------------------------*/
// Simple public getter that is a check of whether the object has a stream

bool object_class::hasStream()
{
  return has_stream;
}


