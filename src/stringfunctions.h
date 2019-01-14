//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR stringfunctions header file                                         //
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


#ifndef PDFR_STRINGFUNCTIONS
#define PDFR_STRINGFUNCTIONS

#include<Rcpp.h>
#include<string>
#include<vector>
#include<unordered_map>

typedef uint16_t RawChar;
typedef uint16_t Unicode;
typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef uint16_t Unicode;
typedef std::unordered_map<RawChar, std::pair<Unicode, int>> GlyphMap;


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

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//

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

std::string carveout(const std::string& subject, const std::string& pre,
                     const std::string& post);
bool IsAscii(const std::string& tempint);
std::vector<float> getnums(const std::string& s);
std::vector<int> getints(const std::string& s);
int oct2dec(int x);
std::vector<unsigned char> bytesFromArray(const std::string& s);
std::string bytestostring(const std::vector<uint8_t>& v);
std::vector<float> matmul(std::vector<float> b, std::vector<float> a);
std::vector<float> stringvectomat(std::vector<std::string> b);
std::vector<float> stringtofloat(std::vector<std::string> b);
std::string intToHexstring(int i);
std::vector<std::string> splitfours(std::string s);
std::vector<int> getObjRefs(std::string& ds);
bool isDictString(const std::string& s);
char symbol_type(const char c);
void trimRight(std::string& s);
size_t firstmatch(std::string& s, std::string m, int startpos);
void upperCase(std::string& s);
std::vector<RawChar> HexstringToRawChar(std::string& s);
std::vector<RawChar> StringToRawChar(std::string& s);
std::vector<int> refFinder(const std::string& s);
std::vector<std::string> multicarve(const std::string& s,
                                    const std::string& a,
                                    const std::string& b);

#endif
