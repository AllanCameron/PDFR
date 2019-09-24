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

#include "streams.h"
#include<algorithm>

using namespace std;
typedef map<uint32_t, uint32_t> HuffmanMap;
typedef vector<uint32_t> LengthArray;

/*---------------------------------------------------------------------------*/
// The flatedecode interface is very simple. Provide it with a deflated string
// and it will replace it with the uncompressed version.

void FlateDecode(string* t_message)
{
  *t_message = Deflate(t_message).Output();
}

/*---------------------------------------------------------------------------*/
// In Deflate, some short messages are encoded with a fixed dictionary, since
// including a dictionary would make the stream longer instead of shorter.
// The decompressor needs to know this dictionary.
//
// This dictionary takes the form of a lookup table. The difficulty here is
// representing a variable-length bit sequence with a single key. Each of the
// 32-bit numbers making up the keys in this lookup table represent a number of
// bits, and the actual number encoded by those bits. The number of bits is
// stored in the 16 high order bits, and the value they represent is stored in
// the 16 low order bits. For example, the bit sequence "1101101" is 109 in
// binary, and is made of 7 bits. Therefore, it would be represented by
// 109 | (7 << 16) which is 458861 or 0x7006d. If I have the key 0x800cc, then I
// know it has 8 bits, since 0x800cc >> 16 == 8, and the value 0xcc or 204,
// since 0x800cc & 0x0ffff == 0xcc. I can therefore determine that my bit
// sequence has to be an 8-bit representation of 204, or 11001100. This could be
// accomplished by having a {bits, value} pair as the key, though you would
// have to do std::make_pair every time you wanted to look some bits up in
// the table, and I assume this is more costly than bitwise operations.
//
// One final note on this table: the bit sequences in all the code lookup tables
// in this implementation of Deflate are reversed. This allows direct reading
// of the codes from the stream in a standard LSB->MSB fashion rather than
// the reverse ordering employed by the packing of Huffman codes. This could
// have been done by reversing the bits every time a code was read, but it
// seemed more sensible to me to reverse the lookup tables, since that would
// require fewer bit-reversals in total.

const HuffmanMap Deflate::fixed_literal_map_ {
{0x8000c,   0}, {0x8008c,   1}, {0x8004c,   2}, {0x800cc,   3}, {0x8002c,   4},
{0x800ac,   5}, {0x8006c,   6}, {0x800ec,   7}, {0x8001c,   8}, {0x8009c,   9},
{0x8005c,  10}, {0x800dc,  11}, {0x8003c,  12}, {0x800bc,  13}, {0x8007c,  14},
{0x800fc,  15}, {0x80002,  16}, {0x80082,  17}, {0x80042,  18}, {0x800c2,  19},
{0x80022,  20}, {0x800a2,  21}, {0x80062,  22}, {0x800e2,  23}, {0x80012,  24},
{0x80092,  25}, {0x80052,  26}, {0x800d2,  27}, {0x80032,  28}, {0x800b2,  29},
{0x80072,  30}, {0x800f2,  31}, {0x8000a,  32}, {0x8008a,  33}, {0x8004a,  34},
{0x800ca,  35}, {0x8002a,  36}, {0x800aa,  37}, {0x8006a,  38}, {0x800ea,  39},
{0x8001a,  40}, {0x8009a,  41}, {0x8005a,  42}, {0x800da,  43}, {0x8003a,  44},
{0x800ba,  45}, {0x8007a,  46}, {0x800fa,  47}, {0x80006,  48}, {0x80086,  49},
{0x80046,  50}, {0x800c6,  51}, {0x80026,  52}, {0x800a6,  53}, {0x80066,  54},
{0x800e6,  55}, {0x80016,  56}, {0x80096,  57}, {0x80056,  58}, {0x800d6,  59},
{0x80036,  60}, {0x800b6,  61}, {0x80076,  62}, {0x800f6,  63}, {0x8000e,  64},
{0x8008e,  65}, {0x8004e,  66}, {0x800ce,  67}, {0x8002e,  68}, {0x800ae,  69},
{0x8006e,  70}, {0x800ee,  71}, {0x8001e,  72}, {0x8009e,  73}, {0x8005e,  74},
{0x800de,  75}, {0x8003e,  76}, {0x800be,  77}, {0x8007e,  78}, {0x800fe,  79},
{0x80001,  80}, {0x80081,  81}, {0x80041,  82}, {0x800c1,  83}, {0x80021,  84},
{0x800a1,  85}, {0x80061,  86}, {0x800e1,  87}, {0x80011,  88}, {0x80091,  89},
{0x80051,  90}, {0x800d1,  91}, {0x80031,  92}, {0x800b1,  93}, {0x80071,  94},
{0x800f1,  95}, {0x80009,  96}, {0x80089,  97}, {0x80049,  98}, {0x800c9,  99},
{0x80029, 100}, {0x800a9, 101}, {0x80069, 102}, {0x800e9, 103}, {0x80019, 104},
{0x80099, 105}, {0x80059, 106}, {0x800d9, 107}, {0x80039, 108}, {0x800b9, 109},
{0x80079, 110}, {0x800f9, 111}, {0x80005, 112}, {0x80085, 113}, {0x80045, 114},
{0x800c5, 115}, {0x80025, 116}, {0x800a5, 117}, {0x80065, 118}, {0x800e5, 119},
{0x80015, 120}, {0x80095, 121}, {0x80055, 122}, {0x800d5, 123}, {0x80035, 124},
{0x800b5, 125}, {0x80075, 126}, {0x800f5, 127}, {0x8000d, 128}, {0x8008d, 129},
{0x8004d, 130}, {0x800cd, 131}, {0x8002d, 132}, {0x800ad, 133}, {0x8006d, 134},
{0x800ed, 135}, {0x8001d, 136}, {0x8009d, 137}, {0x8005d, 138}, {0x800dd, 139},
{0x8003d, 140}, {0x800bd, 141}, {0x8007d, 142}, {0x800fd, 143}, {0x90013, 144},
{0x90113, 145}, {0x90093, 146}, {0x90193, 147}, {0x90053, 148}, {0x90153, 149},
{0x900d3, 150}, {0x901d3, 151}, {0x90033, 152}, {0x90133, 153}, {0x900b3, 154},
{0x901b3, 155}, {0x90073, 156}, {0x90173, 157}, {0x900f3, 158}, {0x901f3, 159},
{0x9000b, 160}, {0x9010b, 161}, {0x9008b, 162}, {0x9018b, 163}, {0x9004b, 164},
{0x9014b, 165}, {0x900cb, 166}, {0x901cb, 167}, {0x9002b, 168}, {0x9012b, 169},
{0x900ab, 170}, {0x901ab, 171}, {0x9006b, 172}, {0x9016b, 173}, {0x900eb, 174},
{0x901eb, 175}, {0x9001b, 176}, {0x9011b, 177}, {0x9009b, 178}, {0x9019b, 179},
{0x9005b, 180}, {0x9015b, 181}, {0x900db, 182}, {0x901db, 183}, {0x9003b, 184},
{0x9013b, 185}, {0x900bb, 186}, {0x901bb, 187}, {0x9007b, 188}, {0x9017b, 189},
{0x900fb, 190}, {0x901fb, 191}, {0x90007, 192}, {0x90107, 193}, {0x90087, 194},
{0x90187, 195}, {0x90047, 196}, {0x90147, 197}, {0x900c7, 198}, {0x901c7, 199},
{0x90027, 200}, {0x90127, 201}, {0x900a7, 202}, {0x901a7, 203}, {0x90067, 204},
{0x90167, 205}, {0x900e7, 206}, {0x901e7, 207}, {0x90017, 208}, {0x90117, 209},
{0x90097, 210}, {0x90197, 211}, {0x90057, 212}, {0x90157, 213}, {0x900d7, 214},
{0x901d7, 215}, {0x90037, 216}, {0x90137, 217}, {0x900b7, 218}, {0x901b7, 219},
{0x90077, 220}, {0x90177, 221}, {0x900f7, 222}, {0x901f7, 223}, {0x9000f, 224},
{0x9010f, 225}, {0x9008f, 226}, {0x9018f, 227}, {0x9004f, 228}, {0x9014f, 229},
{0x900cf, 230}, {0x901cf, 231}, {0x9002f, 232}, {0x9012f, 233}, {0x900af, 234},
{0x901af, 235}, {0x9006f, 236}, {0x9016f, 237}, {0x900ef, 238}, {0x901ef, 239},
{0x9001f, 240}, {0x9011f, 241}, {0x9009f, 242}, {0x9019f, 243}, {0x9005f, 244},
{0x9015f, 245}, {0x900df, 246}, {0x901df, 247}, {0x9003f, 248}, {0x9013f, 249},
{0x900bf, 250}, {0x901bf, 251}, {0x9007f, 252}, {0x9017f, 253}, {0x900ff, 254},
{0x901ff, 255}, {0x70000, 256}, {0x70040, 257}, {0x70020, 258}, {0x70060, 259},
{0x70010, 260}, {0x70050, 261}, {0x70030, 262}, {0x70070, 263}, {0x70008, 264},
{0x70048, 265}, {0x70028, 266}, {0x70068, 267}, {0x70018, 268}, {0x70058, 269},
{0x70038, 270}, {0x70078, 271}, {0x70004, 272}, {0x70044, 273}, {0x70024, 274},
{0x70064, 275}, {0x70014, 276}, {0x70054, 277}, {0x70034, 278}, {0x70074, 279},
{0x80003, 280}, {0x80083, 281}, {0x80043, 282}, {0x800c3, 283}, {0x80023, 284},
{0x800a3, 285}, {0x80063, 286}, {0x800e3, 287}, {0xfffff,   9}, {0x00000,   7}
};

/*---------------------------------------------------------------------------*/
// The fixed distance table, like the fixed literal table, is only used when
// a fixed compression dictionary is employed. It uses the same scheme as above
// to convert from a 32-bit unsigned int to a variable-length bit sequence.

const HuffmanMap Deflate::fixed_distance_map_ {
  {0x50000,  0}, {0x50010,  1}, {0x50008,  2}, {0x50018,  3}, {0x50004,  4},
  {0x50014,  5}, {0x5000c,  6}, {0x5001c,  7}, {0x50002,  8}, {0x50012,  9},
  {0x5000a, 10}, {0x5001a, 11}, {0x50006, 12}, {0x50016, 13}, {0x5000e, 14},
  {0x5001e, 15}, {0x50001, 16}, {0x50011, 17}, {0x50009, 18}, {0x50019, 19},
  {0x50005, 20}, {0x50015, 21}, {0x5000d, 22}, {0x5001d, 23}, {0x50003, 24},
  {0x50013, 25}, {0x5000b, 26}, {0x5001b, 27}, {0x50007, 28}, {0x50017, 29},
  {0x5000f, 30}, {0x5001f, 31}, {0xfffff,  5}, {0x00000,  5}
  };

/*---------------------------------------------------------------------------*/
// Look up what actual lengths the length codes represent

const vector<uint32_t> Deflate::length_table_ {
  0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x17, 0x1b, 0x1f, 0x23, 0x2b,
  0x33, 0x3b, 0x43, 0x53, 0x63, 0x73, 0x83, 0xa3, 0xc3, 0xe3};

/*---------------------------------------------------------------------------*/
// Look up what actual distances the distance codes represent

const vector<uint32_t> Deflate::distance_table_{
  0x0005, 0x0007, 0x0009, 0x000d, 0x0011, 0x0019, 0x0021, 0x0031, 0x0041,
  0x0061, 0x0081, 0x00c1, 0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601,
  0x0801, 0x0c01, 0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001};

/*---------------------------------------------------------------------------*/

Stream::Stream(const string* input_t) : input_(input_t),
                                        input_position_(0),
                                        output_position_(0),
                                        unconsumed_bits_(0),
                                        unconsumed_bit_value_(0) {}

/*---------------------------------------------------------------------------*/

Stream::Stream(const vector<uint8_t>* input_t)
{
  string raw_string(input_t->begin(), input_t->end());
  *this = Stream(&raw_string);
}


/*---------------------------------------------------------------------------*/

uint32_t Stream::GetByte()
{
  if (input_position_ >= input_->size()) return 256;
  uint8_t next_byte = (*input_)[input_position_++];
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

/*---------------------------------------------------------------------------*/

Deflate::Deflate(const string* input_t) : Stream(input_t),
                                               is_last_block_(false)
{
  CheckHeader();
  while (!is_last_block_) ReadBlock();
}

/*---------------------------------------------------------------------------*/

Deflate::Deflate(const vector<uint8_t>* input_t) : Stream(input_t),
                                                        is_last_block_(false)
{
  CheckHeader();
  while (!is_last_block_) ReadBlock();
}

/*---------------------------------------------------------------------------*/

HuffmanMap Deflate::Huffmanize(const LengthArray& lengths)
{
  HuffmanMap huffman_table;
  uint32_t max_length = *max_element(lengths.begin(), lengths.end());
  uint32_t min_length = 15, code = 0;

  for(uint32_t i = 1; i <= max_length; ++i)
  {
    for(size_t j = 0; j < lengths.size(); ++j)
    {
      if(lengths[j] == i)
      {
        uint32_t lookup = (lengths[j] << 16) | BitFlip(code++, lengths[j]);
        huffman_table[lookup] = j;
        if (i < min_length) min_length = i;
      }
    }
    code <<= 1;
  }
  huffman_table[0xfffff] = max_length;
  huffman_table[0x00000] = min_length;

  return huffman_table;
}


std::string PrintBits(uint32_t entry)
{
  std::string result = "";
  uint32_t n_bits = entry >> 16;
  uint32_t mask = 1 << (n_bits - 1);
  uint32_t value = entry & 0xffff;
  while(n_bits > 0)
  {
    if (mask & value) result = result + '1'; else result = result + '0';
    mask = mask >> 1;
    n_bits--;
  }
  return result;
}

/*---------------------------------------------------------------------------*/

uint32_t Deflate::ReadCode(HuffmanMap& map_t)
{
  uint32_t code = 0;
  uint32_t read_bits = map_t[0x00000], maxbits = map_t[0xfffff];
  uint32_t read_value = GetBits(read_bits);

  while(true)
  {
    auto lookup = map_t.find(read_value | (read_bits << 16));
    if (lookup != map_t.end())
    {
      code = lookup->second;
      break;
    }
    else
    {
      read_value = read_value + (GetBits(1) << read_bits++);
      if (read_bits > maxbits) throw runtime_error("Couldn't find code");
    }
  }

  return code;
}

/*---------------------------------------------------------------------------*/

void Deflate::CheckHeader()
{
  uint8_t cmf = GetByte();
  uint8_t flg = GetByte();
  if ((cmf & 0x0f) != 8) throw runtime_error("Invalid compression method.");
  if ((((cmf << 8) + flg) % 31)) throw runtime_error("Invalid check flag");
  if ((flg & 32) != 0) throw runtime_error("FDICT bit set in stream header");
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

/*---------------------------------------------------------------------------*/

void Deflate::ReadBlock()
{
  uint32_t three_bit_header = GetBits(3);
  if (three_bit_header & 1) is_last_block_ = true;
  three_bit_header >>= 1;

  if ( three_bit_header == 0)
  {
    HuffmanMap uncompressed_literal_codes;
    for(uint32_t i = 0; i < 286; ++i)
    {
      auto flipped_bit = BitFlip(i, 8);
      uncompressed_literal_codes[flipped_bit + 0x80000] = i;
    }
    literal_map_ = uncompressed_literal_codes;
  }

  if (three_bit_header == 1)
  {
    literal_map_ = fixed_literal_map_;
    distance_map_ = fixed_distance_map_;
  }

  if (three_bit_header == 2) BuildDynamicCodeTable();

  ReadCodes();
};

/*---------------------------------------------------------------------------*/

void Deflate::BuildDynamicCodeTable()
{
  uint32_t literal_code_count = GetBits(5) + 257;
  uint32_t total_code_count = GetBits(5) + 1 + literal_code_count;
  uint32_t number_of_length_codes = GetBits(4) + 4;
  vector<uint8_t> length_code_order {16, 17, 18, 0, 8,  7,  9, 6, 10, 5,
                                      11, 4,  12, 3, 13, 2, 14, 1, 15};

  // build the code lengths code table
  LengthArray code_length_lengths(19);
  for (uint32_t i = 0; i < number_of_length_codes; ++i)
  {
    uint8_t triplet = GetBits(3);
    code_length_lengths[length_code_order[i]] = triplet;
  }

  auto code_length_table = Huffmanize(code_length_lengths);

  LengthArray code_lengths(total_code_count);
  size_t write_head = 0;
  uint32_t code = 0;

  while(write_head < total_code_count)
  {
    code = ReadCode(code_length_table);
    if(code > 15)
    {
      uint32_t repeat_this = 0;
      uint32_t num_repeats = GetBits( 3 * (code / 18) + code - 14);
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

  auto&& literal_start = code_lengths.begin();
  auto&& literal_end = literal_start + literal_code_count;

  literal_map_ = Huffmanize(LengthArray(literal_start, literal_end));
  distance_map_ = Huffmanize(LengthArray(literal_end, code_lengths.end()));
}

/*---------------------------------------------------------------------------*/

void Deflate::ReadCodes()
{
  uint32_t code = 0;

  while(code != 256)
  {
    code = ReadCode(literal_map_);
    if (code < 256) WriteOutput((uint8_t) code);
    if (code > 256) HandlePointer(code);
  }
}

/*---------------------------------------------------------------------------*/

void Deflate::HandlePointer(uint32_t code_t)
{
  uint32_t length_value = 0, distance_value = 0, extrabits = 0;
  if (code_t < 265) length_value = code_t - 254;
  else if (code_t == 285) length_value = 258;
  else
  {
    extrabits = (code_t - 261) / 4;
    uint32_t read_value = GetBits(extrabits);
    length_value = read_value + length_table_[code_t - 265];
  }
  uint32_t distance_code = ReadCode(distance_map_);

  if(distance_code < 4)
  {
    distance_value = distance_code + 1;
  }
  else
  {
    uint32_t read_value = GetBits((distance_code / 2) - 1);
    distance_value = read_value + distance_table_[distance_code - 4];
  }
  AppendPrevious(distance_value, length_value);
}
