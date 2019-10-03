//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities header file                                               //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_UTILTIES

//---------------------------------------------------------------------------//

#define PDFR_UTILTIES

/* This file is the first point in a daisy-chain of header files that
 * constitute the program. Since every other header file
 * in the program ultimately #includes utilities.h, it acts as a single point
 * from which to propagate global definitions and #includes.
 *
 * Since everything here is in the global workspace, it is best to be selective
 * about adding new functions - if possible, new functions should be added
 * to "downstream" classes.
 *
 * There are also a small number of templates defined here which are used to
 * reduce boilerplate code in the rest of the program.
 */

#include<numeric>
#include<string>
#include<vector>
#include<unordered_map>
#include<algorithm>

//---------------------------------------------------------------------------//
/* The characters in pdf strings are most portably interpreted as uint16_t.
 * They need to be translated to Unicode for rendition to the intended
 * characters. Since Unicode is best handled as uint16_t too, it is easy
 * to get confused about pre-translation to Unicode and post-translation.
 * There are thus two typedefs for uint16_t to allow easy tracking of which
 * is which.
 */

typedef uint16_t RawChar;
typedef uint16_t Unicode;

//---------------------------------------------------------------------------//
// Template returning a vector of a std::map's keys. This can be used to
// enumerate the keys for iterating through the map or as a method of printing
// the map to the console

template< typename K, typename V > // K, V stand for key, value
std::vector<K> GetKeys(const std::unordered_map<K, V>& p_map)
{
  // vector to store results
  std::vector<K> result;

  // Ensure it is big enough
  result.reserve(p_map.size());

  // Declare map iterator according to its type
  typename std::unordered_map<K, V>::const_iterator it;

  // The following loop iterates through the map, gets the key and stores it
  for (it = p_map.begin(); it != p_map.end(); ++it) result.push_back(it->first);

  return result;
}

//---------------------------------------------------------------------------//
// Simple template to shorten boilerplate of sticking vectors together
// Given two vectors of the same type, add B's contents to the end of A
// This modifies A

template <typename T>
void Concatenate(std::vector<T>& p_start, const std::vector<T>& p_append)
{
  p_start.insert(p_start.end(), p_append.begin(), p_append.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); returns the indices which if applied would sort the
// vector from lowest to highest

template <typename T>
std::vector<int> Order(std::vector<T> p_data)
{
  std::vector<int> index(p_data.size(), 0); // a new int vector to store results
  for (size_t i = 0; i < p_data.size(); ++i)
  {
    index[i] = std::count_if(p_data.begin(), p_data.end(),
               [&](T other) { return other < p_data[i]; });
  }

  return index;
}

//---------------------------------------------------------------------------//
// Sort one vector by another's order. Modifies supplied vector

template <typename Ta, typename Tb>
std::vector<Ta> SortBy(std::vector<Ta> p_sortee,
                       const std::vector<Tb>& p_sorter)
{
  // Nothing to do!
  if (p_sortee.empty()) return p_sortee;

  // Throw error if lengths don't match
  if (p_sortee.size() != p_sorter.size())
  {
    throw std::runtime_error("SortBy requires equal-lengthed vectors");
  }

  // Vector to store results
  std::vector<Ta> result;

  // Use Order(p_sorter) as defined above to sort sortee
  for (auto i : Order(p_sorter)) result.emplace_back(p_sortee[i]);

  return result;
}

//---------------------------------------------------------------------------//
//                                                                           //
//                      global function declarations                         //
//                                                                           //
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Return the first substring of s that lies between two strings.
// This can be used e.g. to find the byte position that always sits between
// "startxref" and "%%EOF"

std::string CarveOut(const std::string& string_to_be_carved,
                     const std::string& left_delimiter,
                     const std::string& right_delimiter);

//---------------------------------------------------------------------------//
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string> MultiCarve(const std::string& string_to_be_carved,
                                    const std::string& left_delimiter,
                                    const std::string& right_delimiter);

//---------------------------------------------------------------------------//
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const std::string& string_to_be_tested);

//---------------------------------------------------------------------------//
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

std::vector<uint8_t> ConvertHexToBytes(const std::string& hex_encoded_string);

//---------------------------------------------------------------------------//
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

std::string ConvertIntToHex(int int_to_be_converted);

//---------------------------------------------------------------------------//
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char GetSymbolType(const char input_char);

//---------------------------------------------------------------------------//
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

std::vector<RawChar> ConvertHexToRawChar(std::string& hex_encoded_string);

//---------------------------------------------------------------------------//
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

std::vector<RawChar> ConvertStringToRawChar(const std::string& input_string);

//---------------------------------------------------------------------------//
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

std::vector<int> ParseReferences(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> ParseInts(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// consume and convert integers but not hex, octal or scientific notation

std::vector<float> ParseFloats(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// Loads a file's contents into a single std::string using <fstream>

std::string GetFile(const std::string& path_to_file);

#endif
