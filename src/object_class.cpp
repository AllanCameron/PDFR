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
#include "Rex.h"
#include "stringfunctions.h"
#include "streams.h"
#include "dictionary.h"
#include "document.h"
#include "object_class.h"
#include "crypto.h"

object_class::object_class(document& d, int objnum) : number(objnum)
{
  std::string &fs = d.filestring;
  std::vector<uint8_t> &fk = d.filekey;
  int startbyte = d.Xref.getStart(objnum);
  int stopbyte = d.Xref.getEnd(objnum);
  if(!(objnum > 0)) has_stream = false;
  else
  {
    if(!d.Xref.isInObject(objnum))
    {
      if(!Rex(fs.substr(startbyte, 20), "<<").has())
      {
        header = dictionary("<<>>");
        stream = fs.substr(startbyte, stopbyte - startbyte);
        stream = carveout(stream, " obj\\s+", "\\s+endobj");
      }
      else
      {
        header = dictionary(fs, startbyte);
        stream = getStreamContents(d, fs, startbyte);
        if(d.encrypted) stream = decryptStream(stream, fk, number, 0);
        if(isFlateDecode(fs, startbyte)) stream = FlateDecode(stream);
        has_stream = stream.length() > 0 ? true : false;
      }
    }
    else
    {
      int holdingobj = d.Xref.inObject(objnum);
      if(!d.Xref.objectExists(holdingobj))
      {
        stream = "";
        has_stream = false;
      }
      else
      {
        startbyte = d.Xref.getStart(holdingobj);
        std::string str = getStreamContents(d, fs, startbyte);
        if(d.encrypted) str = decryptStream(str, fk, holdingobj, 0);
        if(isFlateDecode(fs, startbyte)) str = FlateDecode(str);
        *this = object_class(d, str, objnum);
      }
    }
  }
  objectHasKids();
  objectHasContents();
  if(has_kids) findKids();
  if(has_contents) findContents();
}

/*---------------------------------------------------------------------------*/

object_class::object_class(document& d, std::string str, int objnum)
  : number(objnum)
{
  int startbyte = 0;
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

  if(!numarray.empty())
  {
    for(size_t i = 0; i < numarray.size(); i++)
    {
      if(i % 2 == 0) objnums.push_back(numarray[i]);
      if(i % 2 == 1)
      {
        bytenums.push_back(numarray[i]);
        if(i == numarray.size() - 1) bytelen.push_back(s.size() - numarray[i]);
        else bytelen.push_back(numarray[i+2] - numarray[i]);
      }
    }
    for(size_t i = 0; i < objnums.size(); i++)
    {
      if(objnums[i] == objnum)
      {
        std::string H = s.substr(bytenums[i], bytelen[i]);
        if(H[0] == '<')
        {
          header = dictionary(H);
          stream = "";
          has_stream = false;
        }
        else
        {
          header = dictionary();
          stream = H;
          has_stream = true;
          if(stream.size() < 15 && Rex(stream, "\\d+ R").get().size() == 1)
          {
            *this = object_class(d, getObjRefs(stream)[0]);
          }
        }
        objectHasKids();
        objectHasContents();
        if(has_kids) findKids();
        if(has_contents) findContents();
        break;
      }
    }
  }
}


/*---------------------------------------------------------------------------*/

void object_class::objectHasKids()
{
  has_kids = header.has("/Kids");
}

/*---------------------------------------------------------------------------*/

void object_class::findKids()
{
  std::vector <int> resvec;
  std::string rawkidsstring = carveout(header.get("/Kids"), "\\[", "\\]");
  std::vector<std::string> kidStrings = splitter(rawkidsstring, " \\d+ R( )*");
  for(auto i : kidStrings) if(stoi(i) != 0) resvec.push_back(stoi(i));
  Kids = resvec;
}


/*---------------------------------------------------------------------------*/

void object_class::objectHasContents()
{
  has_contents = header.has("/Contents");
}

/*---------------------------------------------------------------------------*/

void object_class::findContents()
{
  std::vector <int> resvec;
  std::string rawcontentsstring = carveout(header.get("/Contents"), "\\[", "\\]");
  std::vector<std::string> conStrings = splitter(rawcontentsstring, " \\d+ R( )*");
  for(auto i : conStrings) if(stoi(i) != 0) resvec.push_back(stoi(i));
  Contents = resvec;
}

/*---------------------------------------------------------------------------*/


dictionary object_class::getDict() { return header;}
std::vector<int> object_class::getKids() { return Kids;};
std::vector<int> object_class::getContents() { return Contents;};
std::string object_class::getStream(){ return stream;}

bool object_class::hasKids() { return has_kids;}
bool object_class::hasStream() { return has_stream;}
bool object_class::hasContents() { return has_contents;}

