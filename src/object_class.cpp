//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR object_class implementation file                                     /
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


#include "pdfr.h"
#include "stringfunctions.h"
#include "streams.h"
#include "dictionary.h"
#include "document.h"
#include "object_class.h"
#include "crypto.h"

object_class::object_class(document* doc, int objnum) : number(objnum), d(doc),
has_stream(false)
{
  std::string &fs = d->filestring;
  streampos = {0, 0};
  size_t startbyte = d->Xref.getStart(objnum);
  size_t stopbyte  = d->Xref.getEnd(objnum);
  if(objnum != 0)
  {
    if(!d->Xref.isInObject(objnum))
    {
      if(fs.substr(startbyte, 20).find("<<") == string::npos)
      {
        header = dictionary();
        streampos[0] = firstmatch(fs, " obj", startbyte) + 4;
        streampos[1] = stopbyte - 1;
        has_stream = streampos[1] > streampos[0];
      }
      else
      {
        header = dictionary(&fs, startbyte);
        streampos = getStreamLoc(d, fs, startbyte);
        has_stream = streampos[1] > streampos[0];
      }
    }
    else
    {
      int holdingobj = d->Xref.inObject(objnum);
      if(d->Xref.objectExists(holdingobj))
        *this = object_class(d, d->getobject(holdingobj).getStream(), objnum);
    }
  }
  if(header.has("/Kids"))
    Kids = header.getRefs("/Kids");
  if(header.has("/Contents"))
    Contents = header.getRefs("/Contents");
}

/*---------------------------------------------------------------------------*/

object_class::object_class(document* doc, std::string str, int objnum)
  : number(objnum), d(doc)
{
  int startbyte = 0;
  streampos = {0,0};
  for(auto i : str)
  {
    char a = symbol_type(i);
    if(a != ' ' && a != 'D') break;
    startbyte++;
  }
  std::string s(str.begin() + startbyte, str.end());
  std::string pre(str.begin(), str.begin() + startbyte - 1);
  std::vector<int> numarray = getints(pre);
  std::vector<int> objnums, bytenums, bytelen;
  size_t numarrsize = numarray.size();
  if(!numarray.empty())
  {
    for(size_t i = 0; i < numarrsize; i++)
    {
      if(i % 2 == 0) objnums.push_back(numarray[i]);
      if(i % 2 == 1)
      {
        bytenums.push_back(numarray[i]);
        if(i == (numarrsize - 1))
          bytelen.push_back(s.size() - numarray[i]);
        else
          bytelen.push_back(numarray[i + 2] - numarray[i]);
      }
    }
    size_t onsize = objnums.size();
    for(size_t i = 0; i < onsize; i++)
    {
      if(objnums[i] == objnum)
      {
        std::string H = s.substr(bytenums[i], bytelen[i]);
        if(H[0] == '<')
        {
          header = dictionary(&H);
          stream = "";
          has_stream = false;
        }
        else
        {
          header = dictionary();
          stream = H;
          has_stream = true;
          if(stream.size() < 15 && stream.find(" R", 0) < 15)
          {
            *this = object_class(d, getObjRefs(stream)[0]);
          }
        }
        break;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

dictionary object_class::getDict()
{
  return header;
}

/*---------------------------------------------------------------------------*/

std::string object_class::getStream()
{
  if(!hasStream())
    return "";
  else if(!stream.empty())
    return stream;
  else
    stream = d->filestring.substr(streampos[0], streampos[1] - streampos[0]);
  if(d->encrypted)
    stream = decryptStream(stream, d->filekey, number, 0);
  if(header.get("/Filter").find("/FlateDecode", 0) != string::npos)
    stream = FlateDecode(stream);
  return stream;
}

/*---------------------------------------------------------------------------*/

bool object_class::hasStream()
{
  return has_stream;
}


