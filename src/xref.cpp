//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR xref implementation file                                            //
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
#include "xref.h"
#include "debugtools.h"

/*---------------------------------------------------------------------------*/

void xref::locateXrefs()
{
  std::vector<int> res;
  std::string partial = d->filestring.substr(d->filesize - 50, 50);
  std::string xrefstring =  carveout(partial, "startxref", "%%EOF");
  res.push_back(stoi(xrefstring));
  if(res.empty()) Xreflocations = res;
  else
  {
    bool noprevious = false;
    while ((res.back() != 0) && (noprevious == false))
    {
      int XRS = res.back();
      std::string x = d->filestring.substr(XRS, 2000);
      std::vector<int> prevstart = Rex(x, "/Prev *").ends();
      noprevious = prevstart.size() == 0;
      if(!noprevious)
      {
        x = x.substr(prevstart[0], 10);
        Rex stst = Rex(x, "\\d+");
        if(stst.pos().empty() || stst.ends().empty())
          noprevious = true;
        else
          res.push_back(stoi(x.substr(stst.pos()[0], stst.ends()[0])));
      }
    }
    Xreflocations = res;
    getTrailer();
  }
}

/*---------------------------------------------------------------------------*/

void xref::xrefstrings()
{
  std::vector<std::string> res;
  for(auto i : Xreflocations)
  {
    size_t nchars = 0;
    std::vector<int> ts;
    while (ts.size() == 0 && ( (size_t)(i + nchars) < d->filesize))
    {
      nchars += 1000;
      if(i + nchars > d->filesize) nchars = d->filesize - i;
      std::string hunter = d->filestring.substr(i, nchars);
      ts = Rex(hunter, "startxref").pos();
    }
    if (!ts.empty())
    {
      int minloc = *min_element(ts.begin(), ts.end());
      res.push_back(d->filestring.substr(i + 5, minloc + 4));
    }
    else
      throw "No object found at location";
  }
  printvec(res);
  Xrefstrings = res;
}

/*---------------------------------------------------------------------------*/

void xref::xrefIsstream()
{
  for(auto i : Xrefstrings)
    XrefsAreStreams.push_back(Rex(i.substr(0, 15), "<<").has());
}

/*---------------------------------------------------------------------------*/

void xref::getTrailer()
{
  TrailerDictionary = dictionary(this->d->filestring, Xreflocations[0]);
  if(!TrailerDictionary.has("/Root"))
    throw "Didn't find trailer";
}

/*---------------------------------------------------------------------------*/

void xref::xrefFromStream(int xrefloc)
{
  XRtab xreftable;
  try
  {
    xreftable = decodeString(*d, d->filestring, xrefloc);
  }
  catch(...)
  {
    throw "couldn't decode string";
  }
  if(xreftable.empty())
    throw "xreftable empty";
  for (size_t j = 0; j < xreftable[0].size(); j++)
  {
    xrefrow txr;
    txr.object = xreftable[3][j];
    txr.startbyte = txr.in_object = xreftable[1][j];
    if (xreftable[0][j] == 1) txr.in_object = 0;
    if (xreftable[0][j] == 2) txr.startbyte = 0;
    xreftab[txr.object] = txr;
    objenum.push_back(txr.object);
  }
}

/*---------------------------------------------------------------------------*/


void xref::xrefFromString(std::string& xstr)
{
  std::vector<int> inuse, byteloc, objnumber;
  if(!Rex(xstr, "\\d+").has())
    return;
  int startingobj = std::stoi(Rex(xstr, "\\d+").get().at(0));
  std::vector<std::string> bytestrings  = Rex(xstr, "\\d{10}").get();
  std::vector<std::string> inusestrings = Rex(xstr, "( )\\d{5} ").get();
  if(!bytestrings.empty() && !inusestrings.empty())
  {
    for(unsigned int i = 0; i < bytestrings.size(); i++)
    {
      inuse.push_back(stoi(inusestrings[i]));
      objnumber.push_back(i + startingobj);
      byteloc.push_back(stoi(bytestrings[i]));
    }
    XRtab xreftable = {inuse, objnumber, byteloc};

    for (unsigned int j = 0; j < xreftable[0].size(); j++)
    {
      if (xreftable[0][j] < 65535)
      {
        xrefrow txr;
        txr.object = xreftable[1][j];
        txr.startbyte = xreftable[2][j];
        txr.stopbyte = 0;
        txr.in_object = 0;
        xreftab[txr.object] = txr;
        objenum.push_back(txr.object);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void xref::buildXRtable()
{
  if (Xreflocations.empty())
    throw "Couldn't get xref locations";
  for(size_t i = 0; i < Xreflocations.size(); i++)
  {
    if(XrefsAreStreams[i])
      xrefFromStream(Xreflocations[i]);
    else
      xrefFromString(Xrefstrings[i]);
  }
}

/*---------------------------------------------------------------------------*/

void xref::findEnds()
{
  for(size_t i = 0; i < objenum.size(); i++)
    if((xreftab[objenum[i]].startbyte > 0))
    {
      int initend = 0;
      if(i < objenum.size() - 1)
        initend = xreftab[objenum[i + 1]].startbyte - 1;
      else
      {
        int last = 500;
        std::vector<int> foundend;
        while(foundend.empty())
        {
          std::string endfile = d->filestring.substr(d->filesize - last, last);
          foundend = Rex(endfile, "endobj").ends();
          if(!foundend.empty())
            initend = d->filesize - last + foundend.back();
          else
            last += 500;
        }
      }
      xreftab[objenum[i]].stopbyte = initend;
    }
    else
      xreftab[objenum[i]].stopbyte = 0;
}

/*---------------------------------------------------------------------------*/

xref::xref(document& d) : d(&d)
{
  locateXrefs();
  std::cout << "Located xrefs" << std::endl;
  xrefstrings();
    std::cout << "Got xref strings" << std::endl;
  xrefIsstream();
    std::cout << "Established streaminess" << std::endl;
  buildXRtable();
    std::cout << "Built table" << std::endl;
  findEnds();
}

/*---------------------------------------------------------------------------*/

bool xref::objectExists(int objnum)
{
  return xreftab.find(objnum) != xreftab.end();
}

/*---------------------------------------------------------------------------*/

size_t xref::getStart(int objnum)
{
  if(objectExists(objnum))
    return (size_t) xreftab.at(objnum).startbyte;
  else
    throw "Object does not exist";
}

/*---------------------------------------------------------------------------*/

size_t xref::getEnd(int objnum)
{
  if(objectExists(objnum))
    return (size_t) xreftab.at(objnum).stopbyte;
  else
    throw "Object does not exist";
}

/*---------------------------------------------------------------------------*/

bool xref::isInObject(int objnum)
{
  if(objectExists(objnum))
    return xreftab.at(objnum).in_object != 0;
  else
    throw "Object does not exist";
};

/*---------------------------------------------------------------------------*/

size_t xref::inObject(int objnum)
{
  if(objectExists(objnum))
    return (size_t) xreftab.at(objnum).in_object;
  else
    throw "Object does not exist";
}

/*---------------------------------------------------------------------------*/

std::vector<int> xref::getObjects()
{
  return objenum;
}

/*---------------------------------------------------------------------------*/
