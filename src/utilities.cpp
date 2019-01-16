//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities implementation file                                       //
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

#include "utilities.h"
#include <fstream>   // for get_file - uses ifstream
#include <algorithm> // for min_element
#include <cmath>     // for pow()

using namespace std;

/*---------------------------------------------------------------------------*/
// Return the first substring of s that lies between two strings.
// This can be used e.g. to find the byte position that always sits between
// "startxref" and "%%EOF"

string carveout(const string& s, const string& pre, const string& post)
{
  int start = s.find(pre);
  if(start == -1) start++; // if pre not found in s, start at beginning of s
  else start += pre.size(); // otherwise start at end of first pre
  string str = s.substr(start, s.size() - start); // trim start of s
  int stop = str.find(post);
  if(stop == -1) return str; // if post not found, finish at end of string
  return str.substr(0, stop); // discard end of string starting at end of post
}

/*---------------------------------------------------------------------------*/
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string>
  multicarve(const std::string& s, const std::string& a, const std::string& b)
{
  std::vector<std::string> res;
  if(a.size() == 0 || b.size() == 0 || s.size() == 0) return res;
  std::string str = s; // makes a copy to allow const correctness
  while(true)
  {
    // this loop progressively matches and removes from str until it's empty
    int start = str.find(a);
    if(start == -1) break; // no more matched pairs so halt
    // chop start off string up to end of first match of a
    str = str.substr(start + a.size(), str.size() - (start + a.size()));
    int stop = str.find(b);
    if(stop == -1) break; // no more matched pairs so halt
    res.push_back(str.substr(0, stop)); // target found - push to result
    // Now discard the target plus the following instance of b
    str = str.substr(stop + b.size(), str.size() - (stop + b.size()));
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const string& tempint)
{
  int mymin = *min_element(tempint.begin(), tempint.end());
  int mymax = *max_element(tempint.begin(), tempint.end());
  // if at least one character is outside the ASCII range, return false
  return (mymin > 7) && (mymax < 126);

}

/*---------------------------------------------------------------------------*/
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

vector<uint8_t> bytesFromArray(const string& s)
{
  vector<uint8_t> resvec;
  if(s.empty()) return resvec; // if string is empty, return empty vector;
  vector<int> tmpvec;
  for(auto a : s) // convert hex characters to numerical values
  {
    if(a > 47 && a < 58)  tmpvec.emplace_back(a - 48); //Digits 0-9
    if(a > 64 && a < 71)  tmpvec.emplace_back(a - 55); //Uppercase A-F
    if(a > 96 && a < 103) tmpvec.emplace_back(a - 87); //Lowercase a-f
  }
  size_t ts = tmpvec.size();
  if(ts == 0) return resvec; // if no hex characters, return empty vector;
  if(ts % 2 == 1)
    tmpvec.push_back(0); // add an extra zero to the end of odd-length strings
  for(size_t i = 0; i < ts; i += 2)
    resvec.emplace_back((uint8_t) (16 * tmpvec.at(i) + tmpvec.at(i + 1)));
  return resvec;
}

/*---------------------------------------------------------------------------*/
// reinterprets a vector of bytes as a string

string bytestostring(const vector<uint8_t>& v)
{
  string res(v.begin(), v.end());
  return res;
}

/*---------------------------------------------------------------------------*/
// Transforms a vector of strings to a fector of floats
// (vectorised version of stof)

vector<float> stringtofloat(vector<string> b)
{
  vector<float> r;
  for(auto i : b) r.push_back(stof(i));
  return r;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> A1

string intToHexstring(int i)
{
  string hex = "0123456789ABCDEF";
  string res;
  int firstnum = i / 4096; // 16^3
  i -= firstnum * 4096;
  int secondnum = i / 256; // 16^2
  i -= secondnum * 256;
  int thirdnum = i / 16;
  i -= thirdnum * 16;
  res += hex[firstnum];
  res += hex[secondnum];
  res += hex[thirdnum];
  res += hex[i];
  while(res.length() < 4)
    res = "0" + res; // sanity clause; adds null digits if length < 4
  return res;
}

/*---------------------------------------------------------------------------*/
// Splits a string into a vector of length-4 elements. Useful for Ascii-
// encoded strings e.g. "00FF00AA1234" -> {"00FF", "00AA", "1234"}

vector<string> splitfours(string s)
{
  vector<string> res;
  // if string empty, return empty vector
  if(s.empty()) return res;
  // if length of s not divisible by 4, prepend zeros until it is
  while(s.size() % 4 != 0) s = '0' + s;
  // push back sequential substrings of length 4
  for(unsigned i = 0; i < s.length()/4; i++)
    res.emplace_back(s.substr(i * 4, 4));
  return res;
}

/*--------------------------------------------------------------------------*/
// test of whether string s contains a dictionary by looking for double
// angle brackets

bool isDictString(const string& s)
{
  return s.find("<<", 0) < s.length();
}

/*---------------------------------------------------------------------------*/
// Classify characters for use in lexers

char symbol_type(const char c)
{
  if(c > 0x40 && c < 0x5b) return 'L'; // character is UPPERCASE
  if(c > 0x60 && c < 0x7b) return 'L'; // character is lowercase
  if(c > 0x2f && c < 0x3a) return 'D'; // character is a digit
  if(c == ' ' || c == 0x0d || c == 0x0a ) return ' '; // character is whitespace
  return c; // if none of the above, return the char itself;
}

/*--------------------------------------------------------------------------*/
// Removes whitespace from right of a string

void trimRight(string& s)
{
  if(s.length() == 0)
    return;
  for(int i = s.length() - 1; i >= 0; i--)
    if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') //whitespace
      s.resize(i);
    else
      break;
}

/*--------------------------------------------------------------------------*/
// Similar to string.find()

size_t firstmatch(std::string& s, std::string m, int startpos)
{
  size_t ssize = s.size();
  size_t msize = m.size();
  if(startpos < 0 || startpos > (int) ssize || msize > ssize) return -1;
  size_t state = 0;
  size_t j = startpos;
  while(j < ssize)
  {
    if(s[j] == m[state])
      state++;
    else
      state = 0;
    if (state == msize)
      break;
    if (ssize - j == 1) return -1;
    j++;
  }
  return j + 1 - msize;
}

/*--------------------------------------------------------------------------*/
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

vector<RawChar> HexstringToRawChar(string& s)
{
  vector<string>&& sv = splitfours(s);
  vector<RawChar> uv;
  for(auto& i : sv)
  {
    if(i.length() < 4) while(i.length() < 4) i = "0" + i;
    i = "0x" + i;
    uv.push_back((RawChar) stoul(i, nullptr, 0));
  }
  return uv;
}

/*--------------------------------------------------------------------------*/
// Converts normal string to a vector of 2-byte width numbers (RawChar)

vector<RawChar> StringToRawChar(string& s)
{
  vector<RawChar> res;
  if(s.size() == 0) return res;
  for(size_t i = 0; i < s.size(); i++)
  {
    uint8_t a = (uint8_t) s[i];
    res.push_back((uint16_t) a);
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character.

vector<int> getObjRefs(const string& s)
{
  std::vector<int> res;
  std::string buf;
  std::string state = "waiting";
  for(auto i : s)
  {
    char m = symbol_type(i);
    if(state == "waiting")
    {
      if(m == 'D')
      {
        buf += i; state = "infirstint";
      }
      continue;
    }
    if(state == "infirstint")
    {
      switch(m)
      {
        case 'D' : buf += i; break;
        case ' ' : state = "wait2"; break;
        default:   buf.clear(); state = "waiting";
      }
      continue;
    }
    if(state == "wait2")
    {
      switch(m)
      {
        case 'D' : state = "insecondint"; break;
        default:   state = "waiting"; break;
      }
      continue;
    }
    if(state == "insecondint")
    {
      switch(m)
      {
        case 'D' : break;
        case ' ' : state = "wait3"; break;
        default:   buf.clear(); state = "waiting"; break;
      }
      continue;
    }
    if(state == "wait3")
    {
      switch(m)
      {
        case 'L' : if(i == 'R') res.push_back(stoi(buf));
                   buf.clear(); state = "waiting"; break;
        default:   buf.clear(); state = "waiting";
      }
      continue;
    }
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.

std::vector<int> getints(const std::string& s)
{
  std::vector<int> res;
  std::string buf;
  enum IntState {WAITING, NEG, INT, IGNORE};
  IntState state = WAITING;
  for(auto i : s)
  {
    char m = symbol_type(i);
    if(state == WAITING)
    {
      if(m == 'D')
      {
        if(buf.length() > 10)
          state = IGNORE;
        else
        {
          buf += i;
          state = INT;
        }
      }
      else if(i == '-')
      {
        buf += i;
        state = NEG;
      }
      continue;
    }
    if(state == NEG)
    {
      if(m == 'D')
      {
        buf += i;
        state = INT;
      }
      else
      {
        buf.clear();
        state = WAITING;
      }
      continue;
    }
    if(state == INT)
    {
      if(m == 'D') buf += i;
      else
      {
        if(buf != "-")
          res.push_back(stoi(buf));
        buf.clear();
        if(i == '.')
          state = IGNORE;
        else
          state = WAITING;
      }
      continue;
    }
    if(state == IGNORE)
      if(m != 'D')
        state = WAITING;
  }
  if(state == INT && !buf.empty())
    res.push_back(stoi(buf));
  return res;
}

/*--------------------------------------------------------------------------*/
// This lexer retrieves floats from a string

std::vector<float> getnums(const std::string& s)
{
  std::vector<float> res;
  std::string buf;
  enum FloatState {WAITING, NEG, PRE, POST};
  FloatState state = WAITING;
  for(auto i : s)
  {
    char m = symbol_type(i);
    if(state == WAITING)
    {
      if(m == 'D')
      {
        buf += i;
        state = PRE;
      }
      else if(i == '-')
      {
        buf += i;
        state = NEG;
      }
      else if(i == '.')
      {
        buf += i;
        state = POST;
      }
      continue;
    }
    if(state == NEG)
    {
      if(m == 'D')
      {
        buf += i;
        state = PRE;
      }
      else if (i == '.')
      {
        buf = "-0.";
        state = POST;
      }
      else
      {
        buf.clear();
        state = WAITING;
      }
      continue;
    }
    if(state == PRE)
    {
      if(m == 'D') buf += i;
      else if (i == '.')
      {
        buf += i;
        state = POST;
      }
      else
      {
        if(buf != "-")
          res.push_back(stof(buf));
        buf.clear();
        state = WAITING;
      }
      continue;
    }
    if(state == POST)
    {
      if(m != 'D')
      {
        res.push_back(stof(buf));
        state = WAITING;
        buf.clear();
      }
      else
        buf += i;
    }
  }
  if(state == PRE && !buf.empty())
    res.push_back(stof(buf));
  if(state == POST && buf != "-0.")
    res.push_back(stof(buf));
  return res;
}

/*--------------------------------------------------------------------------*/
// Loads a file's contents into a single string

std::string get_file(const std::string& file)
{
  string filestring;
  ifstream in(file.c_str(), ios::in | ios::binary); // open connection to file
  auto fileCon = &in;
  fileCon->seekg(0, ios::end); // move to end of file
  size_t filesize = fileCon->tellg(); // reads position
  filestring.resize(filesize); // ensure string is big enough
  fileCon->seekg(0, ios::beg); // move to start of file
  fileCon->read(&filestring[0], filestring.size()); // copy contents
  fileCon->seekg(0, ios::beg); // move back to start of file
  fileCon->close();
  return filestring;
}
