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
#include<algorithm>

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

const std::vector<uint32_t> Deflate::fixed_literal_codes_ {
  0x80030, 0x80031, 0x80032, 0x80033, 0x80034, 0x80035, 0x80036,
  0x80037, 0x80038, 0x80039, 0x8003a, 0x8003b, 0x8003c, 0x8003d,
  0x8003e, 0x8003f, 0x80040, 0x80041, 0x80042, 0x80043, 0x80044,
  0x80045, 0x80046, 0x80047, 0x80048, 0x80049, 0x8004a, 0x8004b,
  0x8004c, 0x8004d, 0x8004e, 0x8004f, 0x80050, 0x80051, 0x80052,
  0x80053, 0x80054, 0x80055, 0x80056, 0x80057, 0x80058, 0x80059,
  0x8005a, 0x8005b, 0x8005c, 0x8005d, 0x8005e, 0x8005f, 0x80060,
  0x80061, 0x80062, 0x80063, 0x80064, 0x80065, 0x80066, 0x80067,
  0x80068, 0x80069, 0x8006a, 0x8006b, 0x8006c, 0x8006d, 0x8006e,
  0x8006f, 0x80070, 0x80071, 0x80072, 0x80073, 0x80074, 0x80075,
  0x80076, 0x80077, 0x80078, 0x80079, 0x8007a, 0x8007b, 0x8007c,
  0x8007d, 0x8007e, 0x8007f, 0x80080, 0x80081, 0x80082, 0x80083,
  0x80084, 0x80085, 0x80086, 0x80087, 0x80088, 0x80089, 0x8008a,
  0x8008b, 0x8008c, 0x8008d, 0x8008e, 0x8008f, 0x80090, 0x80091,
  0x80092, 0x80093, 0x80094, 0x80095, 0x80096, 0x80097, 0x80098,
  0x80099, 0x8009a, 0x8009b, 0x8009c, 0x8009d, 0x8009e, 0x8009f,
  0x800a0, 0x800a1, 0x800a2, 0x800a3, 0x800a4, 0x800a5, 0x800a6,
  0x800a7, 0x800a8, 0x800a9, 0x800aa, 0x800ab, 0x800ac, 0x800ad,
  0x800ae, 0x800af, 0x800b0, 0x800b1, 0x800b2, 0x800b3, 0x800b4,
  0x800b5, 0x800b6, 0x800b7, 0x800b8, 0x800b9, 0x800ba, 0x800bb,
  0x800bc, 0x800bd, 0x800be, 0x800bf, 0x90190, 0x90191, 0x90192,
  0x90193, 0x90194, 0x90195, 0x90196, 0x90197, 0x90198, 0x90199,
  0x9019a, 0x9019b, 0x9019c, 0x9019d, 0x9019e, 0x9019f, 0x901a0,
  0x901a1, 0x901a2, 0x901a3, 0x901a4, 0x901a5, 0x901a6, 0x901a7,
  0x901a8, 0x901a9, 0x901aa, 0x901ab, 0x901ac, 0x901ad, 0x901ae,
  0x901af, 0x901b0, 0x901b1, 0x901b2, 0x901b3, 0x901b4, 0x901b5,
  0x901b6, 0x901b7, 0x901b8, 0x901b9, 0x901ba, 0x901bb, 0x901bc,
  0x901bd, 0x901be, 0x901bf, 0x901c0, 0x901c1, 0x901c2, 0x901c3,
  0x901c4, 0x901c5, 0x901c6, 0x901c7, 0x901c8, 0x901c9, 0x901ca,
  0x901cb, 0x901cc, 0x901cd, 0x901ce, 0x901cf, 0x901d0, 0x901d1,
  0x901d2, 0x901d3, 0x901d4, 0x901d5, 0x901d6, 0x901d7, 0x901d8,
  0x901d9, 0x901da, 0x901db, 0x901dc, 0x901dd, 0x901de, 0x901df,
  0x901e0, 0x901e1, 0x901e2, 0x901e3, 0x901e4, 0x901e5, 0x901e6,
  0x901e7, 0x901e8, 0x901e9, 0x901ea, 0x901eb, 0x901ec, 0x901ed,
  0x901ee, 0x901ef, 0x901f0, 0x901f1, 0x901f2, 0x901f3, 0x901f4,
  0x901f5, 0x901f6, 0x901f7, 0x901f8, 0x901f9, 0x901fa, 0x901fb,
  0x901fc, 0x901fd, 0x901fe, 0x901ff, 0x70000, 0x70001, 0x70002,
  0x70003, 0x70004, 0x70005, 0x70006, 0x70007, 0x70008, 0x70009,
  0x7000a, 0x7000b, 0x7000c, 0x7000d, 0x7000e, 0x7000f, 0x70010,
  0x70011, 0x70012, 0x70013, 0x70014, 0x70015, 0x70016, 0x70017,
  0x800c0, 0x800c1, 0x800c2, 0x800c3, 0x800c4, 0x800c5, 0x800c6,
  0x800c7};

const std::vector<uint32_t> Deflate::fixed_distance_codes_ {
  0x50000, 0x50001, 0x50002, 0x50003, 0x50004, 0x50005, 0x50006,
  0x50007, 0x50008, 0x50009, 0x5000a, 0x5000b, 0x5000c, 0x5000d,
  0x5000e, 0x5000f, 0x50010, 0x50011, 0x50012, 0x50013, 0x50014,
  0x50015, 0x50016, 0x50017, 0x50018, 0x50019, 0x5001a, 0x5001b,
  0x5001c, 0x5001d, 0x5001e, 0x5001f};

/*---------------------------------------------------------------------------*/

Stream::Stream(const std::string& input_t) :  input_(input_t),
                                              input_position_(0),
                                              output_position_(0),
                                              unconsumed_bits_(0),
                                              unconsumed_bit_value_(0) {}

Stream::Stream(const std::vector<uint8_t>& input_t)
{
  std::string raw_string(input_t.begin(), input_t.end());
  *this = Stream(raw_string);
}

/*---------------------------------------------------------------------------*/

std::string Stream::Output() {return output_;}

/*---------------------------------------------------------------------------*/

uint32_t Stream::GetByte()
{
  if (input_position_ >= input_.size()) return 256;
  uint8_t next_byte = input_[input_position_++];
  return next_byte;
  ;
}

/*---------------------------------------------------------------------------*/

uint32_t Stream::PeekByte()
{
  ++input_position_;
  uint32_t result = GetByte();
  --input_position_;
  return result;
}

/*---------------------------------------------------------------------------*/

void Stream::Reset()
{
  input_position_ = 0;
  output_position_ = 0;
  unconsumed_bit_value_ = 0;
  unconsumed_bits_ = 0;
  output_.clear();
}

/*---------------------------------------------------------------------------*/

uint32_t Stream::GetBits(uint32_t n_bits_t)
{
  uint32_t value_read = unconsumed_bit_value_;
  uint8_t bits_read = unconsumed_bits_;

  while (bits_read < n_bits_t)
  {
    uint32_t new_byte = GetByte();
    if (new_byte == 256) throw std::runtime_error("Unexpected end of stream");
    value_read |= new_byte << bits_read;
    bits_read += 8;
  }
  uint32_t result = value_read & ((1 << n_bits_t) - 1);
  unconsumed_bit_value_ = value_read >> n_bits_t;
  bits_read -= n_bits_t;
  unconsumed_bits_ = bits_read;
  return result;
}

/*---------------------------------------------------------------------------*/

Deflate::Deflate(const std::string& input_t) : Stream(input_t),
                                               is_last_block_(false)
{
  CheckHeader();
}

/*---------------------------------------------------------------------------*/

Deflate::Deflate(const std::vector<uint8_t>& input_t) : Stream(input_t),
                                                        is_last_block_(false)
{
  CheckHeader();
}

/*---------------------------------------------------------------------------*/

std::vector<uint32_t> Deflate::Huffmanize(const std::vector<uint32_t>& lengths)
{
  std::vector<uint32_t> huffman_table(lengths.size());
  uint32_t max_length = 0;
  for (auto& i : lengths) if(i > max_length) max_length = i;

  uint32_t current_code = 0;

  for(uint32_t i = 0; i <= max_length; ++i)
  {
    bool code_added = false;
    for(size_t j = 0; j < lengths.size(); ++j)
    {
      if(lengths[j] == i && i != 0)
      {
        code_added = true;
        huffman_table[j] = (lengths[j] << 16) |
                            BitFlip(current_code++, lengths[j]);
      }
    }
    if(code_added) current_code <<= 1;
  }
  return huffman_table;
}

/*---------------------------------------------------------------------------*/

void Deflate::CheckHeader()
{
  uint8_t cmf = GetByte();
  uint8_t flg = GetByte();
  if ((cmf & 0x0f) != 8)
  {
    throw std::runtime_error("Invalid compression method.");
  }
  if ((((cmf << 8) + flg) % 31) != 0)
  {
    throw std::runtime_error("Invalid check flag");
  }
  if ((flg & 32) != 0)
  {
    throw std::runtime_error("FDICT bit set in stream header");
  }
}


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

/*---------------------------------------------------------------------------*/

void Deflate::ReadBlock()
{
  uint32_t three_bit_header = GetBits(3);
  if (three_bit_header & 1) is_last_block_ = true;
  three_bit_header >>= 1;

  if ( three_bit_header == 0)
  {
    std::vector<uint32_t> uncompressed_literal_codes(288);
    for(uint32_t i = 0; i < uncompressed_literal_codes.size(); ++i)
    {
      uncompressed_literal_codes[i] = i + 0x80000;
    }
    literal_codes_ = uncompressed_literal_codes;
  }

  if (three_bit_header == 1)
  {
    literal_codes_ = fixed_literal_codes_;
    distance_codes_ = fixed_distance_codes_;
  }

  if (three_bit_header == 2) BuildDynamicCodeTable();

  ReadCodes();
};

/*---------------------------------------------------------------------------*/

void Deflate::BuildDynamicCodeTable()
{
  uint32_t number_literal_codes = GetBits(5) + 257;
  uint32_t number_distance_codes = GetBits(5) + 1;
  uint32_t total_number_of_codes = number_distance_codes + number_literal_codes;
  uint32_t number_of_length_codes = GetBits(4) + 4;
  std::vector<uint32_t> length_code_order {16, 17, 18, 0, 8,  7,  9, 6, 10, 5,
                                           11, 4,  12, 3, 13, 2, 14, 1, 15};

  uint8_t maxbits = 0x00;
  uint8_t minbits = 0xff;

  // build the code lengths code table
  std::vector<uint32_t> code_length_lengths(19);
  for (uint32_t i = 0; i < number_of_length_codes; ++i)
  {
    uint8_t triplet = GetBits(3);
    code_length_lengths[length_code_order[i]] = triplet;
    if(triplet > maxbits) maxbits = triplet;
    if(triplet < minbits && triplet > 0) minbits = triplet;
  }

  auto code_length_table = Huffmanize(code_length_lengths);
  std::vector<uint32_t> code_lengths(total_number_of_codes);
  size_t write_head = 0;
  uint32_t code = 0;

  while(write_head < total_number_of_codes)
  {
    bool found = false;
    uint32_t read_bits = minbits;
    uint32_t read_value = GetBits(read_bits);
    while(!found)
    {
     uint32_t candidate = read_value | (read_bits << 16);
     for(size_t it = 0; it < code_length_table.size(); ++it)
      {
        if (candidate == code_length_table[it])
        {
          code = it;
          found = true;
          continue;
        }

        if (it == code_length_table.size() - 1 && found == false)
        {
          read_value = read_value + (GetBits(1) << read_bits++);
          if (read_bits > maxbits)
          {
            throw runtime_error("Couldn't find code");
          }
        }
      }
    }
    if(code > 15)
    {
      uint32_t repeat_this = 0;
      uint32_t num_bits = 3 * (code / 18) + code - 14;
      uint32_t num_repeats = GetBits(num_bits);
      num_repeats = num_repeats + 3 + (8 * (code / 18));
      if (code == 16) repeat_this = code_lengths[write_head - 1];
      for (size_t i = 0; i < num_repeats; ++i)
      {
        if(write_head > code_lengths.size()) break;
        code_lengths[write_head++] = repeat_this;
      }
    }
    else
    {
      code_lengths[write_head++] = code;
    }
  }

  auto literal_start = code_lengths.begin();
  auto distance_start = literal_start + number_literal_codes;
  vector<uint32_t> literal_lengths(literal_start, distance_start);
  vector<uint32_t> distance_lengths(distance_start, code_lengths.end());

  literal_codes_ = Huffmanize(literal_lengths);
  distance_codes_ = Huffmanize(distance_lengths);
}

/*---------------------------------------------------------------------------*/

void Deflate::ReadCodes()
{
  uint32_t code = 0;

  uint32_t min_literal = 15;
  uint32_t max_literal = 0;
  uint32_t min_distance = 15;
  uint32_t max_distance = 0;
  for (size_t i = 0; i < literal_codes_.size(); ++i)
  {
    uint32_t code_size = literal_codes_[i] >> 16;
    if (code_size > max_literal) max_literal = code_size;
    if (code_size < min_literal && code_size != 0) min_literal = code_size;
  }
  for (size_t i = 0; i < distance_codes_.size(); ++i)
  {
    uint32_t code_size = distance_codes_[i] >> 16;
    if (code_size > max_distance) max_distance = code_size;
    if (code_size < min_distance && code_size != 0) min_distance = code_size;
  }

  while(code != 256)
  {
    bool found = false;
    uint32_t read_bits = min_literal;
    uint32_t read_value = GetBits(read_bits);

    while(!found)
    {
      uint32_t candidate = read_value | (read_bits << 16);
      for(size_t it = 0; it < literal_codes_.size(); ++it)
      {
        if (candidate == literal_codes_[it])
        {
          code = it;
          found = true;
          continue;
        }

        if (it == literal_codes_.size() - 1 && found == false)
        {
          read_value = read_value + (GetBits(1) << read_bits++);
          if (read_bits > max_literal)
          {
            throw runtime_error("Couldn't find code");
          }
        }
      }
    }
    if(code < 128){ cout << (char) code;}
  }
  std::cout << endl;
}
