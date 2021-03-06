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

#include<map>
#include "streams.h"

std::string FlateDecode(std::string* message);
std::string FlateDecode(const CharString& message);

//---------------------------------------------------------------------------//
// This class reinvents the wheel in an attempt to free the library from
// dependencies. It is a full implementation of Deflate decompression. It uses
// std::map for storing and looking up Huffman trees and inherits from Stream
// to give it an easy interface to the underlying stream. Only the constructor
// is public.

class Deflate : public Stream
{
 public:
  // String and byte-vector constructors. The latter converts to a string.
  Deflate(const std::string*);
  Deflate(const CharString&);

 private:
  bool is_last_block_;    // Flag so decompressor knows when to stop

  // The fixed literal and distance maps are used if compression used a
  // fixed dictionary. Usually this only happens with short messages.
  static const std::unordered_map<uint32_t, uint32_t> fixed_literal_map_;
  static const std::unordered_map<uint32_t, uint32_t> fixed_distance_map_;

  // If we come across a length code or a distance code, we need to know
  // how many extra bytes to read. This is looked up in these tables.
  static const std::vector<uint32_t> length_table_;
  static const std::vector<uint32_t> distance_table_;

  // Whether its fixed or dynamic compression, we want to end up with a literal
  // and distance map that we can look up.
  std::unordered_map<uint32_t, uint32_t> literal_map_;
  std::unordered_map<uint32_t, uint32_t> distance_map_;

  void CheckHeader_();             // Read first two bytes to ensure valid
  void ReadBlock_();               // Co-ordinates reading of a single block
  void BuildDynamicCodeTable_();   // Builds lookup tables for each block
  void ReadCodes_();               // Actual reading of compressed data
  void HandlePointer_(uint32_t);   // Deals with length & distance pointers

  // Finds the next code in the input stream using given lookup table
  uint32_t ReadCode_(std::unordered_map<uint32_t, uint32_t>&);

  // Creates a Huffman tree from a vector of bit lengths.
  std::unordered_map<uint32_t, uint32_t>
    Huffmanize_(const std::vector<uint32_t>&);
};


#endif
