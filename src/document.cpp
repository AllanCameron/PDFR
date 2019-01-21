//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR document implementation file                                        //
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

#include<fstream>
#include "document.h"

#include<iostream>
using namespace std;

/*---------------------------------------------------------------------------*/

document::document(const string& filename) : file(filename)
{
  get_file();
  buildDoc();
}

/*---------------------------------------------------------------------------*/

document::document(const vector<uint8_t>& bytevector)
{
  filestring = bytestostring(bytevector);
  filesize = filestring.size();
  buildDoc();
}

/*---------------------------------------------------------------------------*/

void document::buildDoc()
{
  Xref = xref(filestring);
  trailer = Xref.trailer();
  getCatalogue();
  getPageDir();
  isLinearized();
  getPageHeaders();
}

/*---------------------------------------------------------------------------*/

void document::get_file()
{
  ifstream in(file.c_str(), ios::in | ios::binary);
  fileCon = &in;
  fileCon->seekg(0, ios::end);
  filesize = fileCon->tellg();
  filestring.resize(filesize);
  fileCon->seekg(0, ios::beg);
  fileCon->read(&filestring[0], filestring.size());
  fileCon->seekg(0, ios::beg);
  fileCon->close();
}

/*---------------------------------------------------------------------------*/

object_class document::getobject(int n)
{
  if(this->objects.find(n) == this->objects.end())
    objects[n] = object_class(&(this->Xref), n);
  return objects[n];
}

/*---------------------------------------------------------------------------*/

void document::getCatalogue()
{
  vector<int> rootnums = trailer.getRefs("/Root");
  if (rootnums.empty())
    throw runtime_error("Couldn't find catalogue from trailer");
  catalogue = getobject(rootnums.at(0)).getDict();
}

/*---------------------------------------------------------------------------*/

void document::getPageDir()
{
  if(!catalogue.hasRefs("/Pages"))
    throw runtime_error("No valid /Pages entry");
  int pagenum = catalogue.getRefs("/Pages").at(0);
  pagedir = getobject(pagenum);
}

/*---------------------------------------------------------------------------*/

void document::isLinearized()
{
  linearized = filestring.substr(0, 100).find("<</Linearized") != string::npos;
}

/*---------------------------------------------------------------------------*/

vector <int> document::expandKids(vector<int> objnums)
{
  if(objnums.empty())
    throw runtime_error("No pages found");
  size_t i = 0;
  vector<int> res;
  while (i < objnums.size())
  {
    object_class&& o = getobject(objnums[i]);
    if (o.getDict().hasRefs("/Kids"))
      concat(objnums, o.getDict().getRefs("/Kids"));
    else
      res.push_back(objnums[i]);
    i++;
  }
  return res;
}

/*---------------------------------------------------------------------------*/

vector <int> document::expandContents(vector<int> objnums)
{
  if(objnums.empty())
    return objnums;
  size_t i = 0;
  vector<int> res;
  while (i < objnums.size())
  {
    object_class o = getobject(objnums[i]);
    if (o.getDict().hasRefs("/Contents"))
      concat(objnums, o.getDict().getRefs("/Contents"));
    else
      res.push_back(objnums[i]);
    i++;
  }
  return objnums;
}

/*---------------------------------------------------------------------------*/

void document::getPageHeaders()
{
  if (pagedir.getDict().hasRefs("/Kids"))
  {
    std::vector<int> kids = expandKids(pagedir.getDict().getRefs("/Kids"));
    pageheaders.reserve(kids.size());
    for (auto i : kids)
      pageheaders.emplace_back(objects.at(i).getDict());
  }
}


/*---------------------------------------------------------------------------*/

