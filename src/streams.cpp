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

/* Streams are normally compressed in PDFs, and the majority appear to be
 * compressed in DEFLATE format. I have used inheritance here with the Stream
 * class playing the role of base class and the various types of compression
 * having their own dervied classes, so that the interface remains standardized
 * and new classes for each type of compression could be added as needed.
 *
 * The Stream Class itself is effectively an abstract class. Its constructor
 * is protected so it can only be called by the derived class constructors.
 */

#include<string>
#include<vector>
#include<iostream>
#include<stdexcept>
#include<map>
#include<algorithm>
#include "streams.h"

using namespace std;


Stream::Stream(const string* input) : input_(*input),
                                        output_(std::string()),
                                        input_position_(input_.begin()),
                                        output_position_(output_.begin()),
                                        unconsumed_bits_(0),
                                        unconsumed_bit_value_(0) {}

Stream::Stream(const CharString& input) : input_(input),
                                        output_(std::string()),
                                        input_position_(input_.begin()),
                                        output_position_(output_.begin()),
                                        unconsumed_bits_(0),
                                        unconsumed_bit_value_(0) {}

/*---------------------------------------------------------------------------*/

uint32_t Stream::GetByte()
{
  if (input_position_ == input_.end()) return 256;
  return (uint8_t) *input_position_++;
}

uint64_t Stream::GetEightBytes()
{
  auto distance_from_end = input_.end() - input_position_;
  if (distance_from_end > 8) distance_from_end = 8;
  uint64_t result = 0;
  while(distance_from_end > 0)
  {
    result += (uint64_t) *(input_position_++) << (8 * distance_from_end--);
  }
  return result;
}

/*---------------------------------------------------------------------------*/

uint32_t Stream::PeekByte()
{
  uint32_t result = GetByte();
  --input_position_;
  return result;
}

/*---------------------------------------------------------------------------*/

void Stream::Reset()
{
  input_position_ = input_.begin();
  output_.clear();
  output_position_ = output_.begin();
  unconsumed_bit_value_ = 0;
  unconsumed_bits_ = 0;
}

/*---------------------------------------------------------------------------*/

uint32_t Stream::GetBits(uint32_t n_bits)
{
  uint32_t value_read = unconsumed_bit_value_;
  uint8_t bits_read = unconsumed_bits_;

  while (bits_read < n_bits)
  {
    uint32_t new_byte = GetByte();
    if (new_byte == 256) throw runtime_error("Unexpected end of stream");
    value_read |= new_byte << bits_read;
    bits_read += 8;
  }

  uint32_t result = value_read & ((1 << n_bits) - 1);
  unconsumed_bit_value_ = value_read >> n_bits;
  bits_read -= n_bits;
  unconsumed_bits_ = bits_read;
  return result;
}

/*---------------------------------------------------------------------------*/

uint32_t Stream::BitFlip(uint32_t value, uint32_t n_bits)
{
  uint32_t result = 0;
  for(uint32_t i = 1; i <= n_bits; ++i)
  {
    result = (result << 1) | (value & 1);
    value  >>= 1;
  }
  return result;
}
