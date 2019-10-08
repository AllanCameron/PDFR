//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Deflate implementation file                                         //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include<string>
#include<vector>
#include<iostream>
#include<stdexcept>
#include<map>
#include<algorithm>
#include "streams.h"
#include "deflate.h"


using namespace std;

// I will define a HuffmanMap for brevity and clarity
typedef map<uint32_t, uint32_t> HuffmanMap;

// Further typedef for brevity and clarity
typedef vector<uint32_t> LengthArray;

/*---------------------------------------------------------------------------*/
// The flatedecode interface is very simple. Provide it with a pointer to a
// compressed string and it will replace it with a pointer to the uncompressed
// version.

void FlateDecode(string* p_message)
{
  *p_message = Deflate(p_message).Output();
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
// The Deflate constructor calls the stream constructor and then runs the
// decompression without further prompting.

Deflate::Deflate(const string* p_input) : Stream(p_input),
                                          is_last_block_(false)
{
  ExpectExpansionFactor(6);

  // This will abort further reading if the two header bytes aren't right.
  CheckHeader_();

  // Reads each available block sequentially
  while (!is_last_block_) ReadBlock_();

  ShrinkToFit();
}


/*---------------------------------------------------------------------------*/
// The Huffmanize function reconstructs a Huffman tree from a vector of lengths.
// It assumes that the position of each length in the vector is the number to
// be associated with the Huffman code. This means that symbols which don't
// need a code have to have a zero associated with them. For example, if
// the vector (3, 0, 2, 3, 0, 4) is passed, the equivalent mapping would be
// [ 010 -> 0 ], [ 00 -> 2], [ 011 -> 3], [ 1000 -> 5].
//
// Because the bit sequences are variable length, they are stored in such a way
// as to allow both the number of bits and the bit sequence to be combined into
// a single key. We reverse the bit sequence for easy lookup and store it in
// the low order bits, then store the number of bits in the high order bits.

HuffmanMap Deflate::Huffmanize_(const LengthArray& p_lengths)
{
  HuffmanMap huffman_table;

  // The maximum length is stored in the main table as it is needed for reading
  // the table later.
  uint32_t max_length = *max_element(p_lengths.begin(), p_lengths.end());

  // The minimum length needs to be found in the loop.
  uint32_t min_length = 15, code = 0;

  for(uint32_t i = 1; i <= max_length; ++i) // For each possible length
  {
    for(size_t j = 0; j < p_lengths.size(); ++j) // check each acutal length
    {
      if(p_lengths[j] == i) // If it matches, add it to the table.
      {
        // Creates the lookup value from the code value and number of bits
        uint32_t&& lookup = (p_lengths[j] << 16)|BitFlip(code++, p_lengths[j]);

        // Stores the result
        huffman_table[lookup] = j;

        // Update min_length if length is smaller the current smallest
        if (i < min_length) min_length = i;
      }
    }
    // Bitshift code to the left so that there won't be any prefix clashes with
    // higher bit-length sequences
    code <<= 1;
  }

  // Store the max and min lengths for easy lookup
  huffman_table[0xfffff] = max_length;
  huffman_table[0x00000] = min_length;

  return huffman_table;
}

/*---------------------------------------------------------------------------*/
// Mostly helpful as a debugger, this takes a representation of a bit-sequence
// as it is given in the Huffman tree and returns a std::string of ones and
// zeros to show the bits represented. It is a non-exported function and does
// not appear in the header file.

std::string PrintBits(uint32_t p_entry)
{
  std::string result = "";

  // Read the value and number of bits from the uint32
  uint32_t n_bits = p_entry >> 16, value = p_entry & 0xffff;

  // Set an initial mask to give only the number of bits needed for the value
  uint32_t mask = 1 << (n_bits - 1);

  // Now loops through from MSB to LSB to add a 1 or 0 to the string so that
  // the string is presented in MSB -> LSB order
  while(n_bits > 0)
  {
    if (mask & value) result = result + '1'; else result = result + '0';
    mask = mask >> 1;
    n_bits--;
  }
  return result;
}

/*---------------------------------------------------------------------------*/
// This is the function that actually reads the bit stream to find the next
// code, matching it against the appropriate HuffmanMap. The map is passed
// by reference.

uint32_t Deflate::ReadCode_(HuffmanMap& p_map)
{
  // The maximum and minimum number of bits that may be required for a match
  // in a given Huffman table is stored in the table itself and retrieved here.
  uint32_t read_bits = p_map[0x00000], maxbits = p_map[0xfffff];

  // Start by reading in the minimum number of bits that could match
  uint32_t read_value = GetBits(read_bits);

  while(true) // The loop will run until explicitly exited by return or throw
  {
    // Create a lookup key from the number of read bits and their value
    auto lookup = p_map.find(read_value | (read_bits << 16));

    // If this isn't found, get the next bit and repeat the loop
    if (lookup == p_map.end())
    {
      read_value = read_value + (GetBits(1) << read_bits++);

      // If we can't find a match in our lookup even with max_bits, something
      // has gone wrong and we need to throw an error.
      if (read_bits > maxbits) throw runtime_error("Couldn't find code");
    }
    // Otherwise, a match has been found so we return the looked-up value.
    else return lookup->second;
  }
}

/*---------------------------------------------------------------------------*/
// Every deflate stream begins with two header bytes (CMF and FLG). This checks
// they are valid before attempting to decompress. It will only allow the
// compression method DEFLATE (CMF & 0x0f == 8), it will halt if the fixed
// dictionary flag is set, and it will also throw if the checksum fails.

void Deflate::CheckHeader_()
{
  uint8_t cmf = GetByte(); // Gets first byte
  uint8_t flg = GetByte(); // Gets second byte

  // Check compression method
  if ((cmf & 0x0f) != 8) throw runtime_error("Invalid compression method.");

  // Ensure checksum is modulo 31
  if ((((cmf << 8) + flg) % 31)) throw runtime_error("Invalid check flag");

  // Throw if FDCIT is set
  if ((flg & 32) != 0) throw runtime_error("FDICT bit set in stream header");
}

/*---------------------------------------------------------------------------*/
// A deflate stream comes in a series of blocks. Very often in pdf, there is
// only a single block, since a block can hold up to 32K bytes, and most
// compressed objects in pdf streams will be smaller than this. Each block is
// effectively self-contained, containing all the information it requires to
// decompress itself.
//
// The ReadBlock() function co-ordinates the steps needed to decompress each
// block, and is called recurrently by the constructor until the last block has
// been read.

void Deflate::ReadBlock_()
{
  // The first bit of any block is a flag announcing whether this is the last
  // block "1", or whether there are other blocks to be read after this one.
  // The next two bits tell us the compression type : "00" means no compression,
  // "01" indicates the standard default Huffman dictionary will be used, and
  // "10" means that a custom dictionary is going to be used. "11" signals an
  // error.

  // Read the three header bits in one gulp
  uint32_t three_bit_header = GetBits(3);

  // Interrogate the is_last_block_ bit and store it
  if (three_bit_header & 1) is_last_block_ = true;

  // Now we can read the compression type by ditching the first bit and reading
  // the next two
  three_bit_header >>= 1;

  // I'm not sure how often mode 0 is used, since there seems little point in
  // having uncompressed data in a deflate stream. However, I have implemented
  // it anyway, albeit in an inefficient manner whereby a Huffman tree is
  // created for 8-bit numbers to return themselves via a Huffman Map.
  if ( three_bit_header == 0)
  {
    HuffmanMap uncompressed_literal_codes;
    for(uint32_t i = 0; i < 286; ++i)
    {
      auto flipped_bit = BitFlip(i, 8);
      uncompressed_literal_codes[flipped_bit + 0x80000] = i;
    }
    // Remember the maximum and minimum bit lengths will both be 8.
    uncompressed_literal_codes[0x00000] = 8;
    uncompressed_literal_codes[0xfffff] = 8;
    literal_map_ = uncompressed_literal_codes;
  }

  // If the default dictionary is used, this is easy. We have this stored as
  // a static object, and just use it as our literal and distance maps/
  if (three_bit_header == 1)
  {
    literal_map_ = fixed_literal_map_;
    distance_map_ = fixed_distance_map_;
  }

  // Most deflate streams will have their own dictionary. This is complex to
  // construct and requires its own long function to do so
  if (three_bit_header == 2) BuildDynamicCodeTable_();

  // This safety check ensures we don't read garbage if there's an error
  if (three_bit_header == 3) throw runtime_error("Invalid dictionary type.");

  // Now we should be in a position to read our actual compressed data.
  ReadCodes_();
};

/*---------------------------------------------------------------------------*/

void Deflate::BuildDynamicCodeTable_()
{
  // Read the number of literal / length codes
  uint32_t literal_code_count = GetBits(5) + 257;

  // Effectively reads the number of distance codes
  uint32_t total_code_count = GetBits(5) + 1 + literal_code_count;

  // Read the number of entries in the code length code length [sic] table
  uint32_t number_of_length_codes = GetBits(4) + 4;

  // The entries in the code length code lengths will represent these numbers
  // in order. If there are less than 19 entries, the remaining numbers get
  // an associated length of zero
  vector<uint8_t> length_code_order {16, 17, 18, 0, 8,  7,  9, 6, 10, 5,
                                     11, 4,  12, 3, 13, 2, 14, 1, 15};

  // Read the code lengths code table using the above numbers as indices. Each
  // code length is given as three bits (lengths can be 0 to 7)
  LengthArray code_length_lengths(19);
  for (uint32_t i = 0; i < number_of_length_codes; ++i)
  {
    uint8_t triplet = GetBits(3);
    code_length_lengths[length_code_order[i]] = triplet;
  }

  // Pass these lengths to our Huffman map reconstructor
  auto code_length_table = Huffmanize_(code_length_lengths);

  // Create an empty array for our literal / distance code lengths
  LengthArray code_lengths(total_code_count);

  // We need to keep track of our current position and look out for the special
  // cases of codes 16, 17, 18, which are run-length encodings rather than
  // individual lengths to be written to the array.
  size_t write_head = 0;
  uint32_t code = 0;

  // Now we fill our length array until the write head reaches the end.
  while(write_head < total_code_count)
  {
    // Find the next matching code in the Huffman table
    code = ReadCode_(code_length_table);

    // Handle run-length encoding
    if(code > 15)
    {
      // Codes 17 and 18 write a sequence of zeros - make this default
      uint32_t repeat_this = 0;

      // Code 16 repeats the last written entry instead
      if (code == 16) repeat_this = code_lengths[write_head - 1];

      // The number of repeats is given by a simple formula derived from a table
      // in RFC 1951
      uint32_t num_repeats = GetBits( 3 * (code / 18) + code - 14);
      num_repeats = num_repeats + 3 + (8 * (code / 18));

      // Now write the desired element the desired number of times
      for (size_t i = 0; i < num_repeats; ++i)
      {
        if(write_head > code_lengths.size()) break; // This shouldn't happen
        code_lengths[write_head++] = repeat_this;
      }
    }
    else
    {
      // If not run length encoding, write the length and move on
      code_lengths[write_head++] = code;
    }
  }

  // Set iterators that divide the resultant length array into two: the literal
  // table and the distance table
  auto&& literal_start = code_lengths.begin();
  auto&& literal_end = literal_start + literal_code_count;

  // Now build our Huffman tree from the given length arrays
  literal_map_ = Huffmanize_(LengthArray(literal_start, literal_end));
  distance_map_ = Huffmanize_(LengthArray(literal_end, code_lengths.end()));
}

/*---------------------------------------------------------------------------*/
// This function simply calls ReadCode() while there are still codes to be read
// within a block. It stops when it encounters 256, which is the stop code. If
// it reads a number higher than 256, it knows it is dealing with a length code
// and switches to pointer mode.

void Deflate::ReadCodes_()
{
  uint32_t code = 0;

  while(code != 256) // continues indefinitely until explicitly exited.
  {
    code = ReadCode_(literal_map_);               // Read the next code
    if (code < 256) WriteOutput((uint8_t) code);  // If it's a literal, write it
    if (code > 256) HandlePointer_(code);         // If it's a length, handle it
  }
}

/*---------------------------------------------------------------------------*/
// If ReadCodes() comes across a number greater than 256, it knows it must be
// dealing with a length code which will in turn be followed by a distance code.
// This single function handles that situation.

void Deflate::HandlePointer_(uint32_t p_code)
{
  // Initialize the variables we will use to store the length and distance
  uint32_t length_value = 0, distance_value = 0, extrabits = 0;

  // Length codes of 257 -> 264 represent lengths 1 -> 8
  if (p_code < 265) length_value = p_code - 254;

  // length code 285 represents the maximum length of 258
  else if (p_code == 285) length_value = 258;

  // The other length codes (265 - 284) are less straightforward. Each of them
  // requires a specific number of extra bits to be read to determine the actual
  // length. The number of bits is easily calculated by (p_code - 261) / 4,
  // but the actual base lengths are just looked up in a table.
  else
  {
    // How many extra bits should be looked up is dependent on the code itself
    extrabits = (p_code - 261) / 4;

    // We now read these extra bits and add them to the base length value
    // looked up in the length_table_ member
    uint32_t read_value = GetBits(extrabits);
    length_value = read_value + length_table_[p_code - 265];
  }

  // Now we have our length, we need the distance. The distance codes are stored
  // in their own table, and we now search for a match in the next few bits in
  // the stream using the ReadCode() function
  uint32_t distance_code = ReadCode_(distance_map_);

  // The first four distance codes are 1-4.
  if(distance_code < 4) distance_value = distance_code + 1;

  // The other distance codes (up to 31) can represent many thousands of
  // distances. Again, this depends on reading a specified number of extra
  // bits and adding them to the base distance stored against that code number
  // in the distance_table_ member.
  else
  {
    uint32_t read_value = GetBits((distance_code / 2) - 1);
    distance_value = read_value + distance_table_[distance_code - 4];
  }

  // Now that we have our distance and length values, we look back in the
  // output at [output_.end() - distance] and start copying characters to the
  // and of output_ until we have copied [length] characters.
  AppendPrevious(distance_value, length_value);
}
