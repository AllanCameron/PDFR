//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR streams implementation file                                         //
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
#include "streams.h"
#include "document.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "../src/external/miniz.h"
#include "../src/external/miniz.c"
#include "debugtools.h"

/*---------------------------------------------------------------------------*/

bool isFlateDecode(const std::string& filestring, int startpos)
{
  dictionary dict = dictionary(filestring, startpos);
  return Rex(dict.get("/Filter"), "/FlateDecode").has();
}

/*---------------------------------------------------------------------------*/

std::string FlateDecode(const std::string& s)
{
  z_stream stream ;
  int factor = 20;

  while(true)
  {
    char * out = new char[s.length()*factor];
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = s.length();
    stream.next_in = (Bytef*)s.c_str();
    stream.avail_out = s.length()*factor;
    stream.next_out = (Bytef*)out;
    inflateInit(&stream);
    inflate(&stream, Z_FINISH);
    inflateEnd(&stream)       ;

    if(stream.total_out >= factor*s.length())
    {
      delete[] out;
      factor *= 2;
      continue;
    }

    std::string result;
    for(unsigned long i = 0; i < stream.total_out; i++) result += out[i];
    delete[] out;
    return result;
  }
}

/*---------------------------------------------------------------------------*/

std::string
getStreamContents(document& d, const std::string& filestring, int objstart)
{
  dictionary dict = dictionary(filestring, objstart);
  if(dict.has("stream"))
    if(dict.has("/Length"))
    {
      int streamlen;
      if(dict.hasRefs("/Length"))
      {
        int lengthob = dict.getRefs("/Length")[0];
        streamlen = std::stoi(d.getobject(lengthob).getStream());
      }
      else
        streamlen = std::stoi(dict.get("/Length"));
      int streamstart = std::stoi(dict.get("stream"));
      return filestring.substr(streamstart, streamlen);
    }
  return "";
}

/*---------------------------------------------------------------------------*/

bool objHasStream(const std::string& filestring, int objectstart)
{
  dictionary dict = dictionary(filestring, objectstart);
  return dict.has("stream");
}

/*---------------------------------------------------------------------------*/

bool isObject(const std::string& filestring, int objectstart)
{
  std::string ts(filestring.begin() + objectstart,
                 filestring.begin() + objectstart + 20);
  return  Rex(ts, "\\d+ \\d+ obj").has();
}

/*---------------------------------------------------------------------------*/

std::string objectPreStream(const std::string& filestring, int objectstart)
{
  std::vector<int> streamstart, nextobjstart, mn;
  std::string dic;
  int nchars = 0;
  while(mn.size() == 0 && nchars < 3000)
  {
    nchars += 1500;
    dic.assign(filestring.begin() + objectstart,
               filestring.begin() + objectstart + nchars);
    streamstart = Rex(dic, "stream").pos();
    nextobjstart = Rex(dic, "endobj").pos();
    if(!streamstart.empty()) mn.push_back(streamstart[0]);
    if(!nextobjstart.empty()) mn.push_back(nextobjstart[0]);
  }
  if(mn.size()>0)
    return dic.substr(0, *std::min_element(mn.begin(), mn.end()));

  throw std::runtime_error("No object found");
}

/*---------------------------------------------------------------------------*/

std::vector<std::vector<int> >
plainbytetable(std::vector<int> V, std::vector<int> ArrayWidths)
{
  std::vector<int> multarray;
  std::vector<std::vector<int> > rowarray;
  std::vector<int> fixedarray {1, 256, 256 * 256, 256 * 256 * 256};

  for(size_t i = 0; i < ArrayWidths.size(); i++)
  {
    int j = ArrayWidths[i];
    while (j > 0)
    {
      multarray.push_back(fixedarray[j - 1]);
      j--;
    }
  }
  size_t ncol = multarray.size();
  if(ncol == 0)
    throw std::runtime_error("No columns");
  int nrow = V.size() / ncol;
  for(int i = 0; i < (nrow); i++)
  {
    std::vector<int> tmprow;
    int cumval = 0;
    for(size_t j = 0; j < ncol; j++)
    {
      cumval += (V[i * ncol + j] * multarray[j]);
      if(multarray[j] == 1)
      {
        tmprow.push_back(cumval);
        cumval = 0;
      }
    }
    rowarray.push_back(tmprow);
  }
  std::vector<std::vector<int> > colarray;
  for(size_t i = 0; i < rowarray[0].size(); i++)
  {
    std::vector<int> newcol;
    for(size_t j = 0; j < rowarray.size(); j++)
      newcol.push_back(rowarray[j][i]);
    colarray.push_back(newcol);
  }
  std::vector<int> objectIndex;
  for(size_t i = 0; i < rowarray.size(); i++)
    objectIndex.push_back((int) i);
  colarray.push_back(objectIndex);
  return colarray;
}

/*---------------------------------------------------------------------------*/
