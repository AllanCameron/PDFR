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

/*---------------------------------------------------------------------------*/

document::document(const std::string& filename) : file(filename)
{
  get_file();
  Xref = xref(*this);
  trailer = Xref.trailer();
  filekey = get_cryptkey();
  getCatalogue();
  getPageDir();
  isLinearized();
  getPageHeaders();
}

/*---------------------------------------------------------------------------*/

document::document(const std::vector<uint8_t>& bytevector)
{
  filestring = bytestostring(bytevector);
  filesize = filestring.size();
  Xref = xref(*this);
  trailer = Xref.trailer();
  filekey = get_cryptkey();
  getCatalogue();
  getPageDir();
  isLinearized();
  getPageHeaders();
}

/*---------------------------------------------------------------------------*/

void document::get_file()
{
  std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
  fileCon = &in;
  fileCon->seekg(0, std::ios::end);
  filesize = fileCon->tellg();
  filestring.resize(filesize);
  fileCon->seekg(0, std::ios::beg);
  fileCon->read(&filestring[0], filestring.size());
  fileCon->seekg(0, std::ios::beg);
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
  std::vector<int> rootnums = trailer.getRefs("/Root");
  if (rootnums.empty())
    throw std::runtime_error("Couldn't find catalogue from trailer");
  catalogue = getobject(rootnums.at(0)).getDict();
}

/*---------------------------------------------------------------------------*/

void document::getPageDir()
{
  if(!catalogue.hasRefs("/Pages"))
    throw std::runtime_error("No valid /Pages entry");
  int pagenum = catalogue.getRefs("/Pages").at(0);
  pagedir = getobject(pagenum);
}

/*---------------------------------------------------------------------------*/

void document::isLinearized()
{
  linearized = Rex(subfile(0, 100), "<</Linearized").has();
}

/*---------------------------------------------------------------------------*/

std::vector <int> document::expandKids(std::vector<int> objnums)
{
  if(objnums.size() == 0)
    throw std::runtime_error("No pages found");
  std::vector<bool> ispage(objnums.size(), true);
  size_t i = 0;
  while (i < objnums.size())
  {
    object_class o = getobject(objnums[i]);
    if (o.hasKids())
    {
      std::vector<int> tmpvec = o.getKids();
      objnums.insert( objnums.end(), tmpvec.begin(), tmpvec.end() );
      while(ispage.size() < objnums.size()) ispage.push_back(true);
      ispage[i] = false;
    }
    i++;
  }
  std::vector<int> res;
  for(i = 0; i < objnums.size(); i++)
    if(ispage[i])
      res.push_back(objnums[i]);
  return res;
}

/*---------------------------------------------------------------------------*/

std::vector <int> document::expandContents(std::vector<int> objnums)
{
  if(objnums.size() == 0)
    return objnums;
  size_t i = 0;
  while (i < objnums.size())
  {
    object_class o = getobject(objnums[i]);
    if (o.hasContents())
    {
      std::vector<int> tmpvec = o.getContents();
      objnums.erase(objnums.begin() + i);
      objnums.insert(objnums.begin() + i, tmpvec.begin(), tmpvec.end());
      i = 0;
    }
    else i++;
  }
  return objnums;
}

/*---------------------------------------------------------------------------*/

void document::getPageHeaders()
{
  if (pagedir.hasKids())
  {
    std::vector<int> finalkids = expandKids(pagedir.getKids());
    for (auto i : finalkids)
      pageheaders.push_back(getobject(i).getDict());
  }
}

/*---------------------------------------------------------------------------*/

page document::getPage(int pagenum)
{
  return page(*this, pagenum);
}

/*---------------------------------------------------------------------------*/

std::string document::subfile(int startbyte, int len)
{
  return filestring.substr(startbyte, len);
}

/*---------------------------------------------------------------------------*/

std::vector<uint8_t> document::get_cryptkey()
{
  std::vector<uint8_t> blank;
  if(trailer.has("/Encrypt"))
  {
    this->encrypted = true;
    int encnum = trailer.getRefs("/Encrypt").at(0);
    dictionary encdict;
    if(this->Xref.objectExists(encnum))
      encdict = getobject(encnum).getDict();
    std::vector<uint8_t> pbytes = perm(encdict.get("/P"));
    int rnum = 2;
    if(encdict.hasInts("/R"))
      rnum = encdict.getInts("/R").at(0);
    size_t cryptlen = 5;
    if(encdict.hasInts("/Length"))
      cryptlen = encdict.getInts("/Length").at(0) / 8;
    std::string ostarts = encdict.get("/O");
    std::string ustarts = encdict.get("/U");
    std::vector<uint8_t> obytes;
    if(ostarts.size() > 32)
      for(auto j : ostarts.substr(1, 32))
        obytes.push_back(j);
    std::vector<uint8_t> ubytes;
    if(ustarts.size() > 32)
      for(auto j : ustarts.substr(1, 32))
        ubytes.push_back(j);
    std::vector<uint8_t> idbytes;
    if(trailer.has("/ID"))
      idbytes = bytesFromArray(trailer.get("/ID"));
    idbytes.resize(16);

    std::vector<uint8_t> Fstring = UPW;
    concat(Fstring, obytes);
    concat(Fstring, pbytes);
    concat(Fstring, idbytes);
    std::vector<uint8_t> filekey = md5(Fstring);
    filekey.resize(cryptlen);
    if(rnum > 2)
    {
      for(int i = 0; i < 50; i++)
      {
        filekey = md5(filekey);
        filekey.resize(cryptlen);
      }
    }
    std::vector<uint8_t> checkans;
    if(rnum == 2)
    {
      checkans = rc4(UPW, filekey);
      if(checkans.size() == 32)
      {
        int m = 0;
        for(int l = 0; l < 32; l++)
        {
          if(checkans[l] != ubytes[l]) break;
          m++;
        }
        if(m == 32) return filekey;
        else return blank;
      }
    }
    if(rnum > 2)
    {
      std::vector<uint8_t> buf = UPW;
      buf.insert(buf.end(), idbytes.begin(), idbytes.end());
      checkans = md5(buf);
      checkans = rc4(checkans, filekey);
      for (int i = 19; i >= 0; i--)
      {
        std::vector<uint8_t> tmpkey;
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
      return filekey;
    }
  }
  this->encrypted = false;
  return blank;
}

