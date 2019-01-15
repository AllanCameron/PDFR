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

#include "streams.h"
#include "../src/external/miniz.h"
#include "../src/external/miniz.c"
using namespace std;

/*---------------------------------------------------------------------------*/

string FlateDecode(const string& s)
{
  z_stream stream ;
  int factor = 20;
  size_t len = s.length();
  while(true)
  {
    char * out = new char[len * factor];
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;
    stream.avail_in = len;
    stream.next_in = (Bytef*) s.c_str();
    stream.avail_out = len * factor;
    stream.next_out = (Bytef*) out;
    inflateInit(&stream);
    inflate(&stream, 4);
    inflateEnd(&stream);
    if(stream.total_out >= len * factor)
    {
      delete[] out;
      factor *= 2;
      continue;
    }
    string result;
    for(size_t i = 0; i < stream.total_out; i++) result += out[i];
    delete[] out;
    return result;
  }
}

/*---------------------------------------------------------------------------*/

