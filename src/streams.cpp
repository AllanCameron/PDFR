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
/* This is the only file in the program that requires external libraries other
 * than those of the standard library. It uses miniz, which is a small portable
 * library consisting of a single pair of header / implementation files. It
 * does not therefore need to be precompiled and is included in the source
 * files in the "external" directory
 */
#include "streams.h"
#include "../src/external/miniz.h"
#include "../src/external/miniz.c"

using namespace std;

/*---------------------------------------------------------------------------*/
// The flatedecode algorithm. This uses the API of the miniz library to recreate
// the original version of a compressed stream.

string FlateDecode(const string& s)
{
  z_stream stream ; // create a new z_stream object
  int factor = 20;  // initialize the anticipated maximum decompression factor
  size_t len = s.length();  // get input string length
  while(true) //  loop will continue until explicitly exited
  {
    char * out = new char[len * factor];  // create array big enough for result
    stream.zalloc = 0;    //----//
    stream.zfree = 0;           // Initialize stream members
    stream.opaque = 0;    //----//
    stream.avail_in = len; // tell z_stream how big the compressed string is
    stream.next_in = (Bytef*) s.c_str(); // read s into stream
    stream.avail_out = len * factor; // tell z_stream how big 'out' is
    stream.next_out = (Bytef*) out; // create pointer to array for output
    inflateInit(&stream); // initialize the inflation algorithm
    inflate(&stream, 4); // inflation algorithm
    inflateEnd(&stream); // end the inflation algorithm
    if(stream.total_out >= len * factor) // if the result doesn't fit...
    {
      delete[] out;     //
      factor *= 2;      //  ... try again with the array being twice as big
      continue;         //
    }
    string result;
    // copy the resultant array over to a string
    for(size_t i = 0; i < stream.total_out; i++) result += out[i];
    delete[] out; // make sure the created array is destroyed to avoid leak
    return result;
  }
}

/*---------------------------------------------------------------------------*/

