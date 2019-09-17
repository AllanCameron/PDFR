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

#ifndef PDFR_STREAMS

//---------------------------------------------------------------------------//

#define PDFR_STREAMS

/* Streams in pdf files are usually made up of a sequence of non-ascii bytes
 * intended to represent raw data. When they occur they are always part of a pdf
 * object, which will always start with a <<dictionary>>. At the end of the
 * dictionary, after the closing brackets, comes the keyword 'stream', usually
 * (?always) followed by two whitespace bytes: \r and \n. The data then begins.
 * The end of the stream is declared by the sequence (\r\nendstream).
 *
 * The data can represent many different things including pictures, fonts,
 * annotations and postscript-type page descriptions. For the purposes of text
 * extraction, it is mainly the latter we are interested in.
 *
 * The raw data in the stream is almost always compressed, so needs to be
 * decompressed before being processed. That is the purpose of the stream class.
 *
 * At present, only the flatedecode decompression algorithm is implemented. That
 * is simply because it is the most common algorithm used in pdfs. Ultimately,
 * we will need to turn "streams" into its own class, which implements stream
 * decompression for a variety of algorithms on request. I have yet to find a
 * pdf file that uses anything else for page description to allow testing
 *
 * This header is required by the xref class, as it needs to be able to deflate
 * xrefstreams.
 */
#include<string>
#include<vector>
#include<iostream>
#include<stdexcept>

//---------------------------------------------------------------------------//
// Inflates the given string

void FlateDecode(std::string& deflated_string);

//---------------------------------------------------------------------------//


class Stream
{
public:
/*  Possible stream types are:

    Ascii85Stream,
    AsciiHexStream,
    DecodeStream,
    FlateStream,
    NullStream,
    PredictorStream,
    RunLengthStream,
    StreamsSequenceStream,
    StringStream,
    LZWStream,
*/

  Stream(const std::string&);
  Stream(const std::vector<uint8_t>&);

  std::string Output();
  uint32_t GetByte();
  uint32_t PeekByte();
  void Reset();
  uint32_t GetBits(uint32_t);
  uint32_t BitFlip(uint32_t value, uint32_t n_bits);

  private:
  std::string input_;
  std::string output_;
  size_t input_position_;
  size_t output_position_;
  uint8_t unconsumed_bits_;
  uint32_t unconsumed_bit_value_;

};

class Deflate : public Stream
{
public:
  Deflate(const std::string&);
  Deflate(const std::vector<uint8_t>&);
  void ReadBlock();
  void BuildDynamicCodeTable();
  void ReadCodes();
  uint32_t ReadCode(const std::vector<uint32_t>&);
  std::vector<uint32_t> Huffmanize(const std::vector<uint32_t>&);

private:
  void CheckHeader();
  bool is_last_block_;
  static const std::vector<uint32_t> fixed_literal_codes_;
  static const std::vector<uint32_t> fixed_distance_codes_;
  std::vector<uint32_t> literal_codes_;
  std::vector<uint32_t> distance_codes_;
};

#endif

