//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Streams implementation file                                         //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

/* This is the only file in the program that requires external libraries other
 * than those of the standard library. It uses miniz, which is a small portable
 * library consisting of a single pair of header / implementation files. It
 * does not therefore need to be precompiled and is included in the source
 * files in the "external" directory
 */

#include "streams.h"
#include "miniz.h"

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
    if (stream->total_out >= array_size)
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

Stream::Stream(const std::string& input_t) :  input_(input_t),
                                              input_position_(0),
                                              output_position_(0),
                                              unconsumed_bits_(0),
                                              unconsumed_bit_value_(0) {}

std::string Stream::Output() {return output_;}

int Stream::GetByte()
{
  if (input_.size() > input_position_) return input_[input_position_++];
  return -1;
}

int Stream::PeekByte()
{
  ++input_position_;
  int result = GetByte();
  --input_position_;
  return result;
}

void Stream::Reset()
{
  input_position_ = 0;
  output_position_ = 0;
  unconsumed_bit_value_ = 0;
  unconsumed_bits_ = 0;
  output_.clear();
}

int Stream::GetBits(int n_bits_t)
{
  int value_read = unconsumed_bit_value_;
  int bits_read = unconsumed_bits_;

  while (bits_read < n_bits_t)
  {
    int new_byte = GetByte();
    if (new_byte == -1) throw std::runtime_error("Unexpected end of stream");
    value_read |= new_byte << bits_read;
    bits_read += 8;
  }
  int result = value_read & ((1 << n_bits_t) - 1);
  unconsumed_bit_value_ = value_read >> n_bits_t;
  bits_read -= n_bits_t;
  unconsumed_bits_ = bits_read;
  return result;
}
