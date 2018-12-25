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
  std::string partial = d->filestring.substr(d->filesize - 50, 50);
  std::string xrefstring =  carveout(partial, "startxref", "%%EOF");
  Xreflocations.push_back(stoi(xrefstring));
  if(Xreflocations.empty())
    throw runtime_error("No xref entry found");
  TrailerDictionary = dictionary(d->filestring, Xreflocations[0]);
  dictionary tempdict = TrailerDictionary;
  while (true)
    if(tempdict.hasInts("/Prev"))
    {
      Xreflocations.push_back(tempdict.getInts("/Prev")[0]);
      tempdict = dictionary(d->filestring, Xreflocations.back());
    }
    else break;
}

/*---------------------------------------------------------------------------*/

void xref::xrefstrings()
{
  std::vector<std::string> res;
  for(auto i : Xreflocations)
  {
    size_t nchars = 0;
    std::vector<int> ts;
    while (ts.empty() && ( (size_t)(i + nchars) < d->filesize))
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
      throw std::runtime_error("No object found at location");
  }
  Xrefstrings = res;
}

/*---------------------------------------------------------------------------*/

void xref::xrefIsstream()
{
  for(auto i : Xrefstrings)
    XrefsAreStreams.push_back(Rex(i.substr(0, 15), "<<").has());
}

/*---------------------------------------------------------------------------*/

void xref::xrefFromStream(int xrefloc)
{
  XRtab xreftable;
  try
  {
    xreftable = xrefstream(*d, xrefloc).table();
  }
  catch(...)
  {
    throw std::runtime_error("couldn't decode string");
  }
  if(xreftable.empty())
    throw std::runtime_error("xreftable empty");
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

void xrefstream::getIndex()
{
  if(dict.hasInts("/Index"))
    indexEntries = dict.getInts("/Index");
  if(indexEntries.empty())
    objectNumbers = {0};
  else
    for(size_t i = 0; i < indexEntries.size(); i++)
      if(i % 2 == 0)
        firstObject = indexEntries[i];
      else
        for(auto j = 0; j < indexEntries[i]; j++)
          objectNumbers.push_back(firstObject + j);
}

/*---------------------------------------------------------------------------*/

void xrefstream::getParms()
{
  if(dict.has("/DecodeParms"))
  {
    if(subdict.hasInts("/Columns"))
      ncols = subdict.getInts("/Columns")[0];
    if(subdict.hasInts("/Predictor"))
      predictor = subdict.getInts("/Predictor")[0];
  }
}

/*---------------------------------------------------------------------------*/

void xrefstream::getRawMatrix()
{
  std::string SS = getStreamContents(*d, d->filestring, objstart);
  if(isFlateDecode(d->filestring, objstart))
    SS = FlateDecode(SS);
  std::vector<unsigned char> rawarray(SS.begin(), SS.end());
  std::vector<int> intstrm(rawarray.begin(), rawarray.end());

  std::vector<int> tmparraywidths = dict.getInts("/W");
  for(auto i : tmparraywidths)
  {
    if(i > 0)
      arrayWidths.push_back(i);
  }
  if(ncols == 0)
    for(auto i : arrayWidths)
      ncols += i;
  if(predictor > 9) ncols++;
  if(ncols == 0)
    throw std::runtime_error("divide by zero error");
  int nrows = intstrm.size() / ncols;
  for(int i = 0; i < nrows; i++)
  {
    std::vector<int>
    tmprow(intstrm.begin() + ncols * i, intstrm.begin() + ncols * (i + 1));
    rawMatrix.push_back(tmprow);
  }
}

/*---------------------------------------------------------------------------*/

void xrefstream::diffup()
{
  for(size_t i = 0; i < rawMatrix.size(); i++ )
  if(i > 0)
    for(size_t j = 0; j < rawMatrix.at(i).size(); j++)
      rawMatrix.at(i).at(j) += rawMatrix.at(i - 1).at(j);
}

/*---------------------------------------------------------------------------*/

void xrefstream::modulotranspose()
{
  for(size_t i = 0; i < rawMatrix.at(0).size(); i++)
  {
    std::vector<int> tempcol;
    for(size_t j = 0; j < rawMatrix.size(); j++)
      tempcol.push_back(rawMatrix.at(j).at(i) % 256);
    if(predictor < 10 || i > 0)
      finalArray.push_back(tempcol);
  }
}

/*---------------------------------------------------------------------------*/

void xrefstream::expandbytes()
{
  std::vector<int> byteVals {16777216, 65536, 256, 1};
  std::vector<int> columnConst;
  for(auto i: arrayWidths)
    columnConst.insert(columnConst.end(), byteVals.end() - i, byteVals.end());
  for(size_t i = 0; i < finalArray.size(); i++)
    for(auto &j : finalArray.at(i))
      j *= columnConst.at(i);
}

/*---------------------------------------------------------------------------*/

void xrefstream::mergecolumns()
{
  int cumsum = 0;
  for(auto i : arrayWidths)
  {
    std::vector<int> newcolumn = finalArray.at(cumsum);
    if(i > 1)
      for(int j = 1; j < i; j++)
        for(size_t k = 0; k < finalArray.at(cumsum + j).size(); k++)
          newcolumn.at(k) += finalArray.at(cumsum + j).at(k);
    result.push_back(newcolumn);
    cumsum += i;
  }
  if(result.size() == 2)
  {
    std::vector<int> zeroArray(result.at(0).size(), 0);
    result.push_back(zeroArray);
  }
}

/*---------------------------------------------------------------------------*/

void xrefstream::numberRows()
{
  if(indexEntries.empty())
  {
    for(size_t i = 0; i < result.at(0).size(); i++)
      objectIndex.push_back(i + objectNumbers.at(0));
    result.push_back(objectIndex);
  }
  else
    result.push_back(objectNumbers);
}

/*---------------------------------------------------------------------------*/

std::vector<std::vector<int>> xrefstream::table()
{
  return result;
}

/*---------------------------------------------------------------------------*/

xrefstream::xrefstream(document& doc, int starts) : ncols(0), firstObject(0),
predictor(0), objstart(starts)
{
  d = &doc;
  dict = dictionary(d->filestring, objstart);
  subdict = dictionary(dict.get("/DecodeParms"));
  getIndex();
  getParms();
  if(!dict.hasInts("/W")) throw std::runtime_error("Malformed xref stream");
  getRawMatrix();
  if(predictor == 12) diffup();
  modulotranspose();
  expandbytes();
  mergecolumns();
  numberRows();
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
    throw std::runtime_error("Couldn't get xref locations");
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
  xrefstrings();
  xrefIsstream();
  buildXRtable();
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
    throw std::runtime_error("Object does not exist");
}

/*---------------------------------------------------------------------------*/

size_t xref::getEnd(int objnum)
{
  if(objectExists(objnum))
    return (size_t) xreftab.at(objnum).stopbyte;
  else
    throw std::runtime_error("Object does not exist");
}

/*---------------------------------------------------------------------------*/

bool xref::isInObject(int objnum)
{
  if(objectExists(objnum))
    return xreftab.at(objnum).in_object != 0;
  else
    throw std::runtime_error("Object does not exist");
};

/*---------------------------------------------------------------------------*/

size_t xref::inObject(int objnum)
{
  if(objectExists(objnum))
    return (size_t) xreftab.at(objnum).in_object;
  else
    throw std::runtime_error("Object does not exist");
}

/*---------------------------------------------------------------------------*/

std::vector<int> xref::getObjects()
{
  return objenum;
}

/*---------------------------------------------------------------------------*/
