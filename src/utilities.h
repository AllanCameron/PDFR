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
// #include "debugtools.h"


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

/*---------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------*/
// global function declarations - see utilities.cpp for descriptions

std::string carveout(const std::string&,
                     const std::string&,
                     const std::string&);

bool IsAscii(const std::string&);

std::vector<float> getnums(const std::string&);

std::vector<int> getints(const std::string&);

std::vector<unsigned char> bytesFromArray(const std::string&);

std::string bytestostring(const std::vector<uint8_t>&);

std::vector<float> stringtofloat(std::vector<std::string>);

std::string intToHexstring(int);

std::vector<std::string> splitfours(std::string);

std::vector<int> getObjRefs(const std::string&);

bool isDictString(const std::string&);

char symbol_type(const char);

void trimRight(std::string&);

size_t firstmatch(std::string&, std::string, int);

std::vector<RawChar> HexstringToRawChar(std::string&);

std::vector<RawChar> StringToRawChar(std::string&);

std::vector<std::string> multicarve(const std::string&,
                                    const std::string&,
                                    const std::string&);

std::string get_file(const std::string&);

//---------------------------------------------------------------------------//

#endif
