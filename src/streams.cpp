//----------------------------------------------------------------------------//
//                                                                            //
//  PDFR Streams implementation file                                          //
//                                                                            //
//  Copyright (C) 2018 - 2019 by Allan Cameron                                //
//                                                                            //
//  Licensed under the MIT license - see https://mit-license.org              //
//  or the LICENSE file in the project root directory                         //
//                                                                            //
//----------------------------------------------------------------------------//

/* Streams are normally compressed in PDFs, and the majority appear to be
 * compressed in DEFLATE format. I have used inheritance here with the Stream
 * class playing the role of base class and the various types of compression
 * having their own dervied classes, so that the interface remains standardized
 * and new classes for each type of compression could be added as needed.
 *
 * The Stream Class itself is effectively an abstract class. Its constructor
 * is protected so it can only be called by the derived class constructors.
 */

#include<stdexcept>
#include "streams.h"

using namespace std;

//----------------------------------------------------------------------------//
// The constructor takes a string pointer which it uses as stream input. It
// creates a new string as output and sets iterators to the appropriate places.

Stream::Stream(const string* input_t) : input_(input_t),
                                        output_(std::string()),
                                        input_position_(input_->begin()),
                                        output_position_(output_.begin()),
                                        unconsumed_bits_(0),
                                        unconsumed_bit_value_(0) {}

<<<<<<< HEAD
/*----------------------------------------------------------------------------*/
// Consume the next byte in the stream. Returned as a uint32_t as we need
// to be able to return a 256 to signal end of stream.
||||||| b604107... Finished commenting deflate
/*---------------------------------------------------------------------------*/

Stream::Stream(const vector<uint8_t>* input_t)
{
  string raw_string(input_t->begin(), input_t->end());
  *this = Stream(&raw_string);
}


/*---------------------------------------------------------------------------*/
// Consume the next byte in the stream
=======
/*---------------------------------------------------------------------------*/

Stream::Stream(const vector<uint8_t>* input_t)
{
  string raw_string(input_t->begin(), input_t->end());
  *this = Stream(&raw_string);
}


/*---------------------------------------------------------------------------*/
>>>>>>> parent of b604107... Finished commenting deflate

uint32_t Stream::GetByte()
{
  if (input_position_ == input_->end()) return 256;
  return (uint8_t) *input_position_++;
}

<<<<<<< HEAD
/*----------------------------------------------------------------------------*/
// Look ahead at the next byte
||||||| b604107... Finished commenting deflate
/*---------------------------------------------------------------------------*/
// Look ahead at the next byte
=======
/*---------------------------------------------------------------------------*/
>>>>>>> parent of b604107... Finished commenting deflate

uint32_t Stream::PeekByte()
{
  uint32_t result = GetByte();
  --input_position_;
  return result;
}

<<<<<<< HEAD
/*----------------------------------------------------------------------------*/
// Sets all the counters in the stream back to zero
||||||| b604107... Finished commenting deflate
/*---------------------------------------------------------------------------*/
// Sets all the counters back to zero
=======
/*---------------------------------------------------------------------------*/
>>>>>>> parent of b604107... Finished commenting deflate

void Stream::Reset()
{
  input_position_ = input_->begin();
  output_.clear();
  output_position_ = output_.begin();
  unconsumed_bit_value_ = 0;
  unconsumed_bits_ = 0;
}

/*----------------------------------------------------------------------------*/

uint32_t Stream::GetBits(uint32_t n_bits_t)
{
  uint32_t value_read = unconsumed_bit_value_;
  uint8_t bits_read = unconsumed_bits_;

  while (bits_read < n_bits_t)
  {
    uint32_t new_byte = GetByte();
    if (new_byte == 256) throw runtime_error("Unexpected end of stream");
    value_read |= new_byte << bits_read;
    bits_read += 8;
  }

  uint32_t result = value_read & ((1 << n_bits_t) - 1);
  unconsumed_bit_value_ = value_read >> n_bits_t;
  bits_read -= n_bits_t;
  unconsumed_bits_ = bits_read;
  return result;
}

<<<<<<< HEAD
/*----------------------------------------------------------------------------*/
// An important little helper function to reverse Huffman codes before writing
// to their Huffman tables.
||||||| b604107... Finished commenting deflate
/*---------------------------------------------------------------------------*/
// An important little helper function to reverse Huffman codes before writing
// to their Huffman tables
=======
/*---------------------------------------------------------------------------*/
>>>>>>> parent of b604107... Finished commenting deflate

uint32_t Stream::BitFlip(uint32_t value, uint32_t n_bits)
{
  uint32_t result = 0;
  for(uint32_t i = 1; i <= n_bits; ++i)
  {
    // read value from LSB to MSB, write results MSB to LSB
    result = (result << 1) | (value & 1);
    value  >>= 1;
  }
  return result;
}
