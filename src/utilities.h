//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities header file                                               //
//                                                                           //
//  Copyright (C) 2018 by Allan Cameron                                      //
//                                                                           //
//  Permission is hereby granted, free of charge, to any person obtaining    //
//  a copy of this software and associated documentation files               //
//  (the "Software"), to deal in the Software without restriction, including //
//  without limitation the rights to use, copy, modify, merge, publish,      //
//  distribute, sublicense, and/or sell copies of the Software, and to       //
//  permit persons to whom the Software is furnished to do so, subject to    //
//  the following conditions:                                                //
//                                                                           //
//  The above copyright notice and this permission notice shall be included  //
//  in all copies or substantial portions of the Software.                   //
//                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  //
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               //
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   //
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY     //
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,     //
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        //
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_UTILTIES

//---------------------------------------------------------------------------//

#define PDFR_UTILTIES

/* This file is the first point in a daisy-chain of header files that
 * constitute the program. Although it #includes "debugtools.h", it can be
 * compiled without that file for production. Since every other header file
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

#include<string>
#include<vector>
#include<unordered_map>

//Uncomment the following line if debug functions required in the program:
//#include "debugtools.h"

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

template< typename Mt, typename T >
std::vector<Mt> getKeys(std::unordered_map<Mt, T>& Map)
{
  std::vector<Mt> keyvec; // vector to store results
  keyvec.reserve(Map.size()); // Ensure it is big enough
  // the following loop iterates through the map, gets the key and stores it
  for(typename std::unordered_map<Mt, T>::iterator i = Map.begin();
      i != Map.end(); i++)
    keyvec.push_back(i->first);
  return keyvec;
}

//---------------------------------------------------------------------------//
// Simple template to shorten boilerplate of sticking vectors together
// Given two vectors of the same type, add B's contents to the end of A
// This modifies A

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); returns the indices which if applied would sort the
// vector from lowest to highest

template <typename T>
std::vector<int> order(const std::vector<T>& data)
{
  std::vector<int> index(data.size(), 0); // a new int vector to store results
  int i = 0;
  for (auto &j : index) j = i++; // fills the new vector with 0,1,2,3,..etc
  // Use a lambda function to sort 'index' based on the order of 'data'
  sort(index.begin(), index.end(), [&](const T& a, const T& b)
  {
    return (data[a] < data[b]);
  });
  return index;
}

//---------------------------------------------------------------------------//
// Sort one vector by another's order. Modified supplied vector

template <typename Ta, typename Tb>
void sortby(std::vector<Ta>& vec, const std::vector<Tb>& data)
{
  if(vec.size() == 0) return; // Nothing to do!

  if(vec.size() != data.size()) // throw error if vector lengths don't match
    throw std::runtime_error("sortby requires equal-lengthed vectors");
  std::vector<Ta> res; // vector to store results
  // Use order(data) as defined above to sort vec
  for(auto i : order(data))
    res.emplace_back(vec[i]);
  vec = res; // replace vec by the stored results
  return;
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

std::string carveout(const std::string&,
                     const std::string&,
                     const std::string&);

//---------------------------------------------------------------------------//
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string> multicarve(const std::string&,
                                    const std::string&,
                                    const std::string&);

//---------------------------------------------------------------------------//
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const std::string&);

//---------------------------------------------------------------------------//
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

std::vector<unsigned char> bytesFromArray(const std::string&);

//---------------------------------------------------------------------------//
// reinterprets a vector of bytes as a string

std::string bytestostring(const std::vector<uint8_t>&);

//---------------------------------------------------------------------------//
// Transforms a vector of strings to a vector of floats
// (vectorised version of stof)

std::vector<float> stringtofloat(std::vector<std::string>);

//---------------------------------------------------------------------------//
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

std::string intToHexstring(int);

//---------------------------------------------------------------------------//
// Splits a string into a vector of length-4 elements. Useful for Ascii-
// encoded strings e.g. "00FF00AA1234" -> {"00FF", "00AA", "1234"}

std::vector<std::string> splitfours(std::string);

//---------------------------------------------------------------------------//
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char symbol_type(const char);

//---------------------------------------------------------------------------//
// Removes whitespace from right of a string

void trimRight(std::string&);

//---------------------------------------------------------------------------//
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

std::vector<RawChar> HexstringToRawChar(std::string&);

//---------------------------------------------------------------------------//
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

std::vector<RawChar> StringToRawChar(std::string&);

//---------------------------------------------------------------------------//
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

std::vector<int> getObjRefs(const std::string&);

//---------------------------------------------------------------------------//
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> getints(const std::string&);

//---------------------------------------------------------------------------//
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// include ints but not hex, octal or scientific notation (eg 10e5)

std::vector<float> getnums(const std::string&);

//---------------------------------------------------------------------------//
// Loads a file's contents into a single std::string using <fstream>

std::string get_file(const std::string&);

//---------------------------------------------------------------------------//


#endif
