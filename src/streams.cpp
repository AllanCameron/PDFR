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
using namespace std;

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

vector<size_t> getStreamLoc(document* d, const string& fs, int objstart)
{
  dictionary dict = dictionary(fs, objstart);
  if(dict.has("stream"))
    if(dict.has("/Length"))
    {
      int streamlen;
      if(dict.hasRefs("/Length"))
      {
        int lengthob = dict.getRefs("/Length")[0];
        streamlen = std::stoi(d->getobject(lengthob).getStream());
      }
      else
        streamlen = dict.getInts("/Length")[0];
      int streamstart = dict.getInts("stream")[0];
      std::vector<size_t> res = {(size_t) streamstart,
                                 (size_t) streamstart + streamlen};
      return res;
    }
  std::vector<size_t> res = {0,0};
  return res;
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
