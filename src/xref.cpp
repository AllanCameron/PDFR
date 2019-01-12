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

#include<Rcpp.h>
#include<string>
#include<vector>
#include<unordered_map>
#include "pdfr.h"
#include "stringfunctions.h"
#include "dictionary.h"
#include "object_class.h"
#include "xref.h"
#include "document.h"
#include "streams.h"

/*---------------------------------------------------------------------------*/

void xref::locateXrefs()
{
  std::string&& partial = d->filestring.substr(d->filesize - 50, 50);
  std::string&& xrefstring =  carveout(partial, "startxref", "%%EOF");
  Xreflocations.emplace_back(stoi(xrefstring));
  if(Xreflocations.empty()) throw runtime_error("No xref entry found");
  TrailerDictionary = dictionary(&(d->filestring), Xreflocations[0]);
  dictionary tempdict = TrailerDictionary;
  while (true)
    if(tempdict.hasInts("/Prev"))
    {
      Xreflocations.emplace_back(tempdict.getInts("/Prev")[0]);
      tempdict = dictionary(&(d->filestring), Xreflocations.back());
    }
    else break;
}

/*---------------------------------------------------------------------------*/

void xref::xrefstrings()
{
  std::vector<std::string> res;
  for(auto i : Xreflocations)
  {
    std::string startxref = "startxref";
    int minloc = firstmatch(d->filestring, startxref, i) - 9;
    if (minloc > 0)
    {
      string firstpass = d->filestring.substr(i + 5, minloc - i);
      firstpass = carveout(firstpass, "xref", "trailer");
      res.push_back(firstpass);
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
    XrefsAreStreams.emplace_back(i.substr(0, 15).find("<<", 0) != string::npos);
}

/*---------------------------------------------------------------------------*/

void xref::xrefFromStream(int xrefloc)
{
  XRtab xreftable;
  try
  {
    xreftable = xrefstream(d, xrefloc).table();
  }
  catch(...)
  {
    throw std::runtime_error("couldn't decode string");
  }
  if(xreftable.empty())
    throw std::runtime_error("xreftable empty");
  size_t xrts = xreftable[0].size();
  for (size_t j = 0; j < xrts; j++)
  {
    xrefrow txr;
    txr.object = xreftable[3][j];
    txr.startbyte = txr.in_object = xreftable[1][j];
    if (xreftable[0][j] == 1)
      txr.in_object = 0;
    if (xreftable[0][j] == 2)
      txr.startbyte = 0;
    xreftab[txr.object] = txr;
    objenum.emplace_back(txr.object);
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
  {
    size_t ies = indexEntries.size();
    for(size_t i = 0; i < ies; i++)
      if(i % 2 == 0)
        firstObject = indexEntries[i];
      else
        for(auto j = 0; j < indexEntries[i]; j++)
          objectNumbers.emplace_back(firstObject + j);
  }
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
  std::vector<size_t> sl = getStreamLoc(d, d->filestring, objstart);
  std::string SS = d->filestring.substr(sl[0], sl[1] - sl[0]);
  dictionary dict = dictionary(&(d->filestring), objstart);
  if(dict.get("/Filter").find("/FlateDecode", 0) != string::npos)
    SS = FlateDecode(SS);
  std::vector<unsigned char> rawarray(SS.begin(), SS.end());
  std::vector<int> intstrm(rawarray.begin(), rawarray.end());
  std::vector<int>&& tmparraywidths = dict.getInts("/W");
  for(auto i : tmparraywidths) if(i > 0) arrayWidths.push_back(i);
  if(ncols == 0) for(auto i : arrayWidths) ncols += i;
  if(predictor > 9) ncols++;
  if(ncols == 0) throw std::runtime_error("divide by zero error");
  int nrows = intstrm.size() / ncols;
  for(int i = 0; i < nrows; i++)
    rawMatrix.emplace_back(intstrm.begin() + ncols * i,
                           intstrm.begin() + ncols * (i + 1));
}

/*---------------------------------------------------------------------------*/

void xrefstream::diffup()
{
  size_t rms = rawMatrix.size();
  for(size_t i = 1; i < rms; i++ )
  {
    size_t rmis = rawMatrix.at(i).size();
    for(size_t j = 0; j < rmis; j++)
      rawMatrix.at(i).at(j) += rawMatrix.at(i - 1).at(j);
  }
}

/*---------------------------------------------------------------------------*/

void xrefstream::modulotranspose()
{
  size_t rms = rawMatrix.size();
  size_t rmzs = rawMatrix.at(0).size();
  for(size_t i = 0; i < rmzs; i++)
  {
    std::vector<int> tempcol;
    for(size_t j = 0; j < rms; j++)
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
      {
        size_t fas = finalArray.at(cumsum + j).size();
        for(size_t k = 0; k < fas; k++)
          newcolumn.at(k) += finalArray.at(cumsum + j).at(k);
      }
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

xrefstream::xrefstream(document* doc, int starts) : ncols(0), firstObject(0),
predictor(0), objstart(starts)
{
  d = doc;
  dict = dictionary(&(d->filestring), objstart);
  string decodestring = dict.get("/DecodeParms");
  subdict = dictionary(&decodestring);
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
  std::vector<int> inuse, byteloc, objnumber, allints;
  allints = getints(xstr);
  if(allints.size() < 3) return;
  int startingobj = allints[0];
  if(allints.size() % 2) throw runtime_error("Malformed xref");
  for(size_t i = 2; i < allints.size(); i++)
  {
    if(i % 2 == 0) byteloc.emplace_back(allints[i]);
    else
    {
      inuse.emplace_back(allints[i]);
      objnumber.emplace_back(startingobj + (i / 2) - 1);
    }
  }
  XRtab xreftable = {inuse, objnumber, byteloc};
  for (unsigned int j = 0; j < xreftable[0].size(); j++)
  {
    if (xreftable[0][j] < 65535)
    {
      xrefrow txr;
      txr.object = xreftable[1][j];
      txr.startbyte = xreftable[2][j];
      txr.in_object = 0;
      xreftab[txr.object] = txr;
      objenum.push_back(txr.object);
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

xref::xref(document& d) : d(&d)
{
  locateXrefs();
  xrefstrings();
  xrefIsstream();
  buildXRtable();
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
  if(!objectExists(objnum))
    throw std::runtime_error("Object does not exist");
  size_t i = xreftab[objnum].startbyte;
  if((i > 0))
  {
    return (int) firstmatch(d->filestring, "endobj", i);
  }
  else
    return 0;
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
