//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Deflate header file                                                 //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_DEFLATE

//---------------------------------------------------------------------------//

#define PDFR_DEFLATE

#include "streams.h"
#include<map>

//---------------------------------------------------------------------------//
// Stand-alone function to inflate a deflate stream

void FlateDecode(std::string* deflated_string);

//---------------------------------------------------------------------------//
// This class reinvents the wheel in an attempt to free the library from
// dependencies. It is a full implementation of Deflate decompression. It uses
// std::map for storing and looking up Huffman trees and inherits from Stream
// to give it an easy interface to the underlying stream.

class Deflate : public Stream
{
public:
  // String and byte-vector constructors. The latter converts to a string.
  Deflate(const std::string*);

private:
  bool is_last_block_;    // Flag so decompressor knows when to stop
  void CheckHeader();     // Read first two bytes to ensure valid Deflate

  // The fixed literal and distance maps are used if compression used a
  // fixed dictionary. Usually this only happens with short messages.
  static const std::map<uint32_t, uint32_t> fixed_literal_map_;
  static const std::map<uint32_t, uint32_t> fixed_distance_map_;

  // If we come across a length code or a distance code, we need to know
  // how many extra bytes to read. This is looked up in these tables.
  static const std::vector<uint32_t> length_table_;
  static const std::vector<uint32_t> distance_table_;

  // Whether its fixed or dynamic compression, we want to end up with a literal
  // and distance map that we can look up.
  std::map<uint32_t, uint32_t> literal_map_;
  std::map<uint32_t, uint32_t> distance_map_;

  void ReadBlock();               // Co-ordinates reading of a single block
  void BuildDynamicCodeTable();   // Builds lookup tables for each block
  void ReadCodes();               // Actual reading of compressed data
  void HandlePointer(uint32_t);   // Deals with length & distance pointers

  // Finds the next code in the input stream using given lookup table
  uint32_t ReadCode(std::map<uint32_t, uint32_t>&);

  // Creates a Huffman tree from a vector of bit lengths.
  std::map<uint32_t, uint32_t> Huffmanize(const std::vector<uint32_t>&);
};


#endif
