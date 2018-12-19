//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR stringfunctions implementation file                                 //
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

#include "pdfr.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "debugtools.h"

using namespace std;

/*---------------------------------------------------------------------------*/
// Finds matches m in string s, returning a vector of (matches + 1) substrings
vector<string> splitter(const string& s, const string& m)
{
  Rex sl = Rex(s, m);
  vector<string> RC, res;
  if(!sl.has())
  {
    return sl.get();
  }
  size_t n = (size_t) sl.n();
  RC.push_back(s.substr(0, sl.pos().at(0)));
  if(n > 1)
    for(size_t i = 1; i < n; i++)
      RC.push_back(s.substr(sl.ends().at(i - 1),
                            sl.pos().at(i) - sl.ends().at(i - 1)));
  RC.push_back(s.substr(sl.ends().at(n - 1), s.size() - sl.ends().at(n - 1)));
  for(auto i : RC)
    if(!i.empty())
      res.push_back(i);
  return res;
}

/*---------------------------------------------------------------------------*/
// Return the first substring of s that lies between two regexes
string carveout(const string& s, const string& pre, const string& post)
{
  int firstpos  = 0;
  int secondpos = s.length();
  vector<int> FPV = Rex(s, pre).ends();
  vector<int> SPV = Rex(s, post).pos();
  int fpvs = FPV.size();
  int spvs = SPV.size();
  // Ensure the match lies between the first balanced matches
  if((fpvs == 0) && (spvs > 0)) secondpos = SPV.at(spvs - 1);
  if((fpvs > 0) && (spvs == 0)) firstpos = FPV.at(0);
  if((fpvs > 0) && (spvs > 0))
  {
    firstpos = FPV.at(0);
    for(auto i : SPV)
      if(i > firstpos)
      {
        secondpos = i;
        break;
      }
  }
  string res(s.begin() + firstpos, s.begin() + secondpos);
  return res;
}

/*---------------------------------------------------------------------------*/
// Decent approximation of whether a string contains binary data or not
bool IsAscii(const string& tempint)
{
  int mymin = *min_element(tempint.begin(), tempint.end());
  int mymax = *max_element(tempint.begin(), tempint.end());
  return (mymin > 7) && (mymax < 126);
}

/*---------------------------------------------------------------------------*/
//Casts a string to vector of unsigned ints
vector<uint16_t> strtoint(string x)
{
  vector<uint16_t> res;
  for(auto i : x)
  {
    uint16_t a = i;
    res.push_back(a % 255);
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// Casts a single unsigned int to a length 1 string
string intToString(uint16_t a)
{
  string b;
  if (a > 0x00ff)
    return "*";
  uint8_t d = (uint8_t) a;
  b += (char) d;
  return b;
}

/*---------------------------------------------------------------------------*/
// Generalizes stof to allow multiple floats from a single string
vector<float> getnums(const string& s)
{
  vector<float> res;
  string numstring = "(-)?(\\.)?\\d+(\\.)?\\d*"; // float regex
  vector<string> strs = Rex(s, numstring).get();
  for(auto i : strs)
    res.push_back(stof(i));
  return res;
}

/*---------------------------------------------------------------------------*/
// Generalizes stoi to allow multiple ints to be derived from a string
vector<int> getints(const string& s)
{
  vector<int> res;
  string numstring = "(-)?\\d+"; // int regex
  vector<string> strs = Rex(s, numstring).get();
  for(auto i : strs)
    res.push_back(stoi(i));
  return res;
}

/*---------------------------------------------------------------------------*/
// converts an int to a pseudo - octal
int dec2oct(int x)
{
  int a = (x / 64);
  int b = (x - a * 64) / 8;
  int c = (x - a * 64 - 8 * b);
  string res = to_string(a);
  res += to_string(b);
  res += to_string(c);
  return stoi(res);
}

/*---------------------------------------------------------------------------*/
// converts an octal (as captured by stoi) to intended decimal value
int oct2dec(int x)
{
  int res = 0;
  string str = to_string(x);
  int l = str.length();
  if(l == 0)
    return res;
  for (int i = 0; i < l; i++)
  {
    int e = stoi(str.substr(i,1));
    if(e > 7)
      throw std::runtime_error("Invalid octal");
    res += (e * pow(8, l - i - 1));
  }
  return res;
}

/*---------------------------------------------------------------------------*/
//Takes a string of bytes represented in ASCII and converts to actual bytes

vector<unsigned char> bytesFromArray(const string& s)
{
  if(s.empty())
    throw std::runtime_error("Zero-length string passed to bytesFromArray");
  vector<int> tmpvec, res;
  vector<unsigned char> resvec;
  for(auto a : s)
  {
    if(a > 47 && a < 58)  tmpvec.push_back(a - 48); //Digits 0-9
    if(a > 64 && a < 71)  tmpvec.push_back(a - 55); //Uppercase A-F
    if(a > 96 && a < 103) tmpvec.push_back(a - 87); //Lowercase a-f
  }
  size_t ts = tmpvec.size();
  if(ts == 0)
    throw std::runtime_error("arrayFromBytes not given a byte string");
  for(size_t i = 0; i < ts; i++)
    if(i % 2 == 0)
      tmpvec[i] = 16 * tmpvec[i];
  for(size_t i = 0; i < (ts - 1); i++)
    if(i % 2 == 0)
      res.push_back(tmpvec.at(i) + tmpvec.at(i + 1));
  if((ts - 1) % 2 == 0)
    res.push_back(tmpvec.at(ts - 1));
  for(auto i : res)
    resvec.push_back(i);
  return resvec;
}

/*---------------------------------------------------------------------------*/
// reinterprets string as vector of bytes
vector<uint8_t> stringtobytes(const string& s)
{
  vector<uint8_t> res;
  res.reserve(s.size());
  for(auto i : s)
    res.push_back(i);
  return res;
}

/*---------------------------------------------------------------------------*/
// reinterprets vector of bytes as a string
string bytestostring(const vector<uint8_t>& v)
{
  string res;
  res.reserve(v.size());
  for(auto i : v)
    res += i;
  return res;
}

/*---------------------------------------------------------------------------*/
// Matrix mulitplication on two 3 x 3 matrices
vector<float> matmul(vector<float> b, vector<float> a)
{
  if(a.size() != b.size())
    throw std::runtime_error("matmul: Vectors must have same size.");
  if(a.size() != 9)
    throw std::runtime_error("matmul: Vectors must be size 9.");
  vector<float> newmat;
  for(size_t i = 0; i < 9; i++) //clever use of indices to allow fill by loop
    newmat.push_back(a[i % 3 + 0] * b[3 * (i / 3) + 0] +
                     a[i % 3 + 3] * b[3 * (i / 3) + 1] +
                     a[i % 3 + 6] * b[3 * (i / 3) + 2] );
  return newmat;
}

/*---------------------------------------------------------------------------*/
// Converts a pdf style transformation sequence of 6 numbers to a 3x3 matrix
// simply by adding the "missing" invariant final column
vector<float> six2nine(vector<float> a)
{
  if(a.size() != 6)
    throw std::runtime_error("six2nine: Vectors must be size 6.");
  vector<float> newmat {a[0], a[1], 0, a[2], a[3], 0, a[4], a[5], 1};
  return newmat;
}

/*---------------------------------------------------------------------------*/
// Allows a length-6 vector of number strings to be converted to 3x3 matrix
vector<float> stringvectomat(vector<string> b)
{
  if(b.size() != 6)
    throw std::runtime_error("stringvectomat: Vectors must be size 6.");
  vector<float> a;
  for(auto i : b) a.push_back(stof(i));
  return six2nine(a);
}

/*---------------------------------------------------------------------------*/
// generalizes stof to vectors
vector<float> stringtofloat(vector<string> b)
{
  vector<float> r;
  for(auto i : b)
    r.push_back(stof(i));
  return r;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
string intToHexstring(int i)
{
  string hex = "0123456789ABCDEF";
  string res;
  int firstnum = i / (16 * 16 * 16);
  i -= firstnum * (16 * 16 * 16);
  int secondnum = i / (16 * 16);
  i -= secondnum * (16 * 16);
  int thirdnum = i / 16;
  i -= thirdnum * 16;
  res += hex[firstnum];
  res += hex[secondnum];
  res += hex[thirdnum];
  res += hex[i];
  while(res.length() < 4)
    res = "0" + res;
  transform(res.begin(), res.end(), res.begin(),
                 ptr_fun<int, int>(toupper));
  return res;
}

/*---------------------------------------------------------------------------*/
//Split a string into length-4 elements
vector<string> splitfours(string s)
{
  vector<string> res;
  if(s.empty())
    return res;
  while(s.size() % 4 != 0)
    s += '0';
  for(unsigned i = 0; i < s.length()/4; i++)
    res.push_back(s.substr(i * 4, 4));
  return res;
}

/*--------------------------------------------------------------------------*/
//Split a string into length-2 elements
vector<string> splittwos(string s)
{
  vector<string> res;
  if(s.empty())
    return res;
  while(s.size() % 2 != 0)
    s += '0';
  for(unsigned i = 0; i < s.length()/2; i++)
    res.push_back(s.substr(i * 2, 2));
  return res;
}

/*--------------------------------------------------------------------------*/
//Converts an ASCII encoded string to a (char-based) string
string byteStringToString(const string& s)
{
  vector<string> sv = splitfours(s);
  vector<unsigned int> uv;
  string res;
  for(auto i : sv)
    uv.push_back((unsigned) stoul("0x" + i, nullptr, 0));
  for(auto i : uv)
  {
    if(i > 255)
      i = 255;
    res += (char) i;
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// Extracts pdf object references from string
vector<int> getObjRefs(string ds)
{
  vector<int> res;
  for (auto i : Rex(ds, "\\d+ 0 R").get())
    res.push_back(stoi(splitter(i, " ")[0]));
  return res;
}

/*--------------------------------------------------------------------------*/
//test of whether string s contains a dictionary
bool isDictString(const string& s)
{
  return Rex(s, "<<").has();
}

/*---------------------------------------------------------------------------*/
//helper function to classify tokens in lexers
char symbol_type(const char c)
{
  if(c > 0x40 && c < 0x5b) return 'L'; //UPPERCASE
  if(c > 0x60 && c < 0x7b) return 'L'; //lowercase
  if(c > 0x2f && c < 0x3a) return 'D'; //digits
  if(c == ' ' || c == 0x0d || c == 0x0a ) return ' '; //whitespace
  return c;
}


/*--------------------------------------------------------------------------*/
// Removes whitespace from right of a string
void trimRight(string& s)
{
  if(s.length() == 0)
    return;
  for(int i = s.length() - 1; i >= 0; i--)
    if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')
      s.resize(i);
    else
      break;
}
