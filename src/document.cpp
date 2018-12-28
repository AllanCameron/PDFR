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

#include "pdfr.h"
#include "document.h"
#include "streams.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "crypto.h"
#include "debugtools.h"

using namespace std;

/*---------------------------------------------------------------------------*/

document::document(const string& filename) : file(filename), encrypted(false)
{
  get_file();
  buildDoc();
}

/*---------------------------------------------------------------------------*/

document::document(const vector<uint8_t>& bytevector) : encrypted(false)
{
  filestring = bytestostring(bytevector);
  filesize = filestring.size();
  buildDoc();
}

/*---------------------------------------------------------------------------*/

void document::buildDoc()
{
  Xref = xref(*this);
  trailer = Xref.trailer();
  get_cryptkey();
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
    objects[n] = object_class(*this, n);
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
  linearized = Rex(subfile(0, 100), "<</Linearized").has();
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
    if (o.hasKids())
      concat(objnums, o.getKids() );
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
    if (o.hasContents())
      concat(objnums, o.getContents());
    else
      res.push_back(objnums[i]);
    i++;
  }
  return objnums;
}

/*---------------------------------------------------------------------------*/

void document::getPageHeaders()
{
  if (pagedir.hasKids())
  {
    std::vector<int> kids = expandKids(pagedir.getKids());
    pageheaders.reserve(kids.size());
    for (auto i : kids)
      pageheaders.emplace_back(objects.at(i).getDict());
  }
}

/*---------------------------------------------------------------------------*/

page document::getPage(int pagenum)
{
  return page(*this, pagenum);
}

/*---------------------------------------------------------------------------*/

string document::subfile(int startbyte, int len)
{
  return filestring.substr(startbyte, len);
}

/*---------------------------------------------------------------------------*/

vector<uint8_t> document::getPassword(const string& key, dictionary& encdict)
{
  string ostarts = encdict.get(key);
  vector<uint8_t> obytes;
  if(ostarts.size() > 32)
    for(auto j : ostarts.substr(1, 32))
      obytes.push_back(j);
  return obytes;
}

/*---------------------------------------------------------------------------*/

void document::getFilekey(dictionary& encdict)
{
  vector<uint8_t> Fstring = UPW;
  concat(Fstring, getPassword("/O", encdict));
  concat(Fstring, perm(encdict.get("/P")));
  vector<uint8_t> idbytes = bytesFromArray(trailer.get("/ID"));
  idbytes.resize(16);
  concat(Fstring, idbytes);
  filekey = md5(Fstring);
  size_t cryptlen = 5;
  if(encdict.hasInts("/Length"))
    cryptlen = encdict.getInts("/Length").at(0) / 8;
  filekey.resize(cryptlen);
}

/*---------------------------------------------------------------------------*/

void document::checkKeyR2(dictionary& encdict)
{
  vector<uint8_t> ubytes = getPassword("/U", encdict);
  vector<uint8_t> checkans = rc4(UPW, filekey);
  if(checkans.size() == 32)
  {
    int m = 0;
    for(int l = 0; l < 32; l++)
    {
      if(checkans[l] != ubytes[l]) break;
      m++;
    }
    if(m == 32)
      return;
  }
  throw runtime_error("Incorrect cryptkey");
}

/*---------------------------------------------------------------------------*/

void document::checkKeyR3(dictionary& encdict)
{
  vector<uint8_t> ubytes = getPassword("/U", encdict);
  vector<uint8_t> buf = UPW;
  concat(buf, bytesFromArray(trailer.get("/ID")));
  buf.resize(48);
  vector<uint8_t> checkans = rc4(md5(buf), filekey);
  for (int i = 19; i >= 0; i--)
  {
    vector<uint8_t> tmpkey;
    for (auto j : filekey)
      tmpkey.push_back(j ^ ((uint8_t) i));
    checkans = rc4(checkans, tmpkey);
  }
  int m = 0;
  for(int l = 0; l < 16; l++)
  {
    if(checkans[l] != ubytes[l]) break;
    m++;
  }
  if(m != 16) std::cout << "cryptkey doesn't match";
}

/*---------------------------------------------------------------------------*/

void document::get_cryptkey()
{
  if(!trailer.has("/Encrypt")) return;
  int encnum = trailer.getRefs("/Encrypt").at(0);
  if(!Xref.objectExists(encnum)) return;
  encrypted = true;
  dictionary encdict = getobject(encnum).getDict();
  int rnum = 2;
  if(encdict.hasInts("/R")) rnum = encdict.getInts("/R").at(0);
  getFilekey(encdict);
  if(rnum == 2)
    checkKeyR2(encdict);
  else
  {
    size_t cryptlen = filekey.size();
    for(int i = 0; i < 50; i++)
    {
      filekey = md5(filekey);
      filekey.resize(cryptlen);
    }
    checkKeyR3(encdict);
  }
}

