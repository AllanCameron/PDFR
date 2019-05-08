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

void FlateDecode(string& t_message)
{
  // Creates a new z_stream object
  z_stream* stream = new z_stream;

  // Initialize the anticipated maximum decompression factor
  int factor = 20;
  size_t message_length = t_message.length();

  // This loop will repeat until it is explicitly exited.
  while (true)
  {
    // Set the maximum size for the array in this loop
    size_t array_size  = message_length * factor;
    char* output_array = new char[array_size];

    // Initialize stream members
    stream->zalloc    = 0;
    stream->zfree     = 0;
    stream->opaque    = 0;
    stream->avail_in  = message_length;
    stream->next_in   = (Bytef*) t_message.c_str();
    stream->avail_out = array_size;
    stream->next_out  = (Bytef*) output_array;

    // Inflate the stream
    inflateInit(stream);
    inflate(stream, 4);
    inflateEnd(stream);

    // If the result doesn't fit, try again with the array being twice as big
    if(stream->total_out >= array_size)
    {
      delete[] output_array;
      factor *= 2;
      continue;
    }

    // The result must have fit into the array if we got this far, so we will
    // write the result to a new string and swap it with t_string
    string result(output_array, stream->total_out);
    swap(result, t_message);

    // Before we go, make sure the created array is destroyed to avoid leak
    delete[] output_array;
    delete stream;
    break;
  }
}

/*---------------------------------------------------------------------------*/

