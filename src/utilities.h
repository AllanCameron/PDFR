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
 * There are a small number of templates defined here which are used to reduce
 * boilerplate code in the rest of the program.
 *
 */

#include<string>
#include<vector>
#include<unordered_map>
#include "debugtools.h"

/* The characters in pdf strings are most portably interpreted as uint16_t.
 * They need to be translated to Unicode for rendition to the intended
 * characters. Since Unicode is best handled as uint16_t too, it is easy
 * to get confused about pre-translation to Unicode and post-translation.
 * There are thus two typedefs for uint16_t to allow easy tracking of which
 * is which.
 */

typedef uint16_t RawChar;
typedef uint16_t Unicode;


typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef std::unordered_map<RawChar, std::pair<Unicode, int>> GlyphMap;

//---------------------------------------------------------------------------//
// Returns vector of a std::map's keys.

template< typename Mt, typename T >
std::vector<Mt> getKeys(std::unordered_map<Mt, T>& Map)
{
  std::vector<Mt> keyvec;
  keyvec.reserve(Map.size());
  for(typename std::unordered_map<Mt, T>::iterator i = Map.begin();
      i != Map.end(); i++)
    keyvec.push_back(i->first);
  return keyvec;
}

//---------------------------------------------------------------------------//
//Simple template to shorten boilerplate of sticking vectors together

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); allows sorting of one vector by another's order

template <typename T>
std::vector<int> order(const std::vector<T>& data)
{
  std::vector<int> index(data.size(), 0);
  int i = 0;
  for (auto &j : index) j = i++;

  sort(index.begin(), index.end(), [&](const T& a, const T& b)
  {
    return (data[a] < data[b]);
  });
  return index;
}

/*---------------------------------------------------------------------------*/
// Sort one vector by another's order
template <typename T>
void sortby(std::vector<T>& vec, const std::vector<T>& data)
{
  if(vec.size() == 0) return;
  if(vec.size() != data.size())
    throw std::runtime_error("sortby requires equal-lengthed vectors");
  std::vector<T> res(vec.size(), 0);
  for(auto i : order(data))
    res.emplace_back(vec[i]);
  vec = res;
  return;
}

/*---------------------------------------------------------------------------*/
// global function declarations - see utilities.cpp for definitions
std::string carveout(const std::string& subject, const std::string& pre,
                     const std::string& post);
bool IsAscii(const std::string& tempint);
std::vector<float> getnums(const std::string& s);
std::vector<int> getints(const std::string& s);
int oct2dec(int x);
std::vector<unsigned char> bytesFromArray(const std::string& s);
std::string bytestostring(const std::vector<uint8_t>& v);
std::vector<float> stringtofloat(std::vector<std::string> b);
std::string intToHexstring(int i);
std::vector<std::string> splitfours(std::string s);
std::vector<int> getObjRefs(std::string& ds);
bool isDictString(const std::string& s);
char symbol_type(const char c);
void trimRight(std::string& s);
size_t firstmatch(std::string& s, std::string m, int startpos);
std::vector<RawChar> HexstringToRawChar(std::string& s);
std::vector<RawChar> StringToRawChar(std::string& s);
std::vector<int> refFinder(const std::string& s);
std::vector<std::string> multicarve(const std::string& s,
                                    const std::string& a,
                                    const std::string& b);
std::string get_file(const std::string& file);

//---------------------------------------------------------------------------//

#endif
