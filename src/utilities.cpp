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

using namespace std;

static std::unordered_map<uint8_t, uint8_t> hexmap = {
  {'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5}, {'6', 6},
  {'7', 7}, {'8', 8}, {'9', 9}, {'a', 10}, {'A', 10}, {'b', 11}, {'B', 11},
  {'c', 12}, {'C', 12}, {'d', 13}, {'D', 13}, {'e', 14}, {'E', 14},
  {'f', 15}, {'F', 15}
};

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

vector<string> multicarve(const string& s, const string& a, const string& b)
{
  std::vector<std::string> res; // vector to store results
  // if any of the strings are length 0 then return an empty vector
  if(a.size() == 0 || b.size() == 0 || s.size() == 0) return res;
  std::string str = s; // makes a copy to allow const correctness
  while(true)
  {
    // this loop progressively finds matches in s and removes from the start
    // of the string all characters up to the end of matched b.
    // It does this until there are no paired matches left in the string
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

bool IsAscii(const string& s)
{
  if(s.length() == 0) return false;
  char minchar = *min_element(s.begin(), s.end()); // minimum char in s
  char maxchar = *max_element(s.begin(), s.end()); // maximum char in s
  // if at least one character is outside the ASCII range, return false
  return (minchar > 7) && (maxchar < 127);

}

/*---------------------------------------------------------------------------*/
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

vector<uint8_t> bytesFromArray(const string& s)
{
  vector<uint8_t> resvec; // vector to store results
  if(s.empty()) return resvec; // if string is empty, return empty vector;
  vector<int> tmpvec; // temporary vector to hold results
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
// Transforms a vector of strings to a vector of floats
// (vectorised version of stof)

vector<float> stringtofloat(vector<string> b)
{
  vector<float> r;
  for(auto i : b) r.push_back(stof(i));
  return r;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

string intToHexstring(int i)
{
  if(i < 0 || i > 0xffff) return "FFFF"; // returns max if out of range
  string hex = "0123456789ABCDEF";
  string res; // string to hold results
  res += hex[(i & 0xf000) >> 12]; // gets index of hex from first 4 bits
  res += hex[(i & 0x0f00) >> 8];  // gets index of hex from second 4 bits
  res += hex[(i & 0x00f0) >> 4];  // gets index of hex from third 4 bits
  res += hex[i & 0x000f];         // gets index of hex from last 4 bits
  return res;
}

/*---------------------------------------------------------------------------*/
// Splits a string into a vector of length-4 elements. Useful for Ascii-
// encoded strings e.g. "00FF00AA1234" -> {"00FF", "00AA", "1234"}

vector<string> splitfours(string s)
{
  vector<string> res; // vector to hold results
  // if string empty, return empty vector
  if(s.empty()) return res;
  // if length of s not divisible by 4, prepend zeros until it is
  while(s.size() % 4 != 0) s = '0' + s;
  // push back sequential substrings of length 4
  for(unsigned i = 0; i < s.length()/4; i++)
    res.emplace_back(s.substr(i * 4, 4));
  return res;
}

/*---------------------------------------------------------------------------*/
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

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
  if(s.length() == 0) return; // nothing to do if string is empty
  for(int i = s.length() - 1; i >= 0; i--) // reverse iterator
    if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') //whitespace
      s.resize(i); // shrink string to before whitespace character
    else
      break;
}

/*--------------------------------------------------------------------------*/
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

vector<RawChar> HexstringToRawChar(string& s)
{
  while(s.size() % 4 != 0) s += '0';
  vector<RawChar> raw_vector; // vector to store results
  raw_vector.reserve(s.size() / 4);
  for(size_t i = 0; i < (s.size() - 3); i += 4)
  {
    raw_vector.emplace_back(hexmap[(uint8_t)(s[i])] * 4096 +
                            hexmap[(uint8_t)(s[i + 1])] * 256 +
                            hexmap[(uint8_t)(s[i + 2])] * 16 +
                            hexmap[(uint8_t)(s[i + 3])]);
  }
  return raw_vector;
}

/*--------------------------------------------------------------------------*/
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

vector<RawChar> StringToRawChar(string& s)
{
  vector<RawChar> result; // vector to hold results
  result.reserve(s.size());
  if(s.size() == 0) return result; // string s is empty - nothing to do.
  for(auto i : s)
    result.emplace_back(0x00ff & ((uint8_t) i)); // convert uint8 to uint16 and store result

  return result;
}

/*--------------------------------------------------------------------------*/
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

vector<int> getObjRefs(const string& s)
{
  enum RefState // defines the possible states of the finite state machine (fsm)
  {
    WAIT_FOR_START, // Waiting for an integer
    IN_FIRST_INT,   // In an integer
    WAIT_FOR_GEN,   // Waiting for a generation number
    IN_GEN,         // In a second integer
    WAIT_FOR_R      // Wait to see if next char is R to confirm this is a ref
  };
  std::vector<int> res; // vector to hold result
  std::string buf; // a buffer to hold characters until stored or discarded
  RefState state = WAIT_FOR_START; // the current state of the fsm
  for(auto i : s)
  {
    char m = symbol_type(i); // finds out if current char is digit, letter, ws
    if(state == WAIT_FOR_START)
    {
      if(m == 'D')
      {
        buf += i;
        state = IN_FIRST_INT;
      }
      continue; // restarts next iteration of loop.
    }
    if(state == IN_FIRST_INT)
    {
      switch(m)
      {
        case 'D' : buf += i; break;
        case ' ' : state = WAIT_FOR_GEN; break;
        default:   buf.clear(); state = WAIT_FOR_START;
      }
      continue;
    }
    if(state == WAIT_FOR_GEN)
    {
      switch(m)
      {
        case 'D' : state = IN_GEN; break;
        default:   state = WAIT_FOR_START; break;
      }
      continue;
    }
    if(state == IN_GEN)
    {
      switch(m)
      {
        case 'D' : break;
        case ' ' : state = WAIT_FOR_R; break;
        default:   buf.clear(); state = WAIT_FOR_START; break;
      }
      continue;
    }
    if(state == WAIT_FOR_R)
    {
      switch(m)
      {
        case 'L' : if(i == 'R') res.push_back(stoi(buf));
                   buf.clear(); state = WAIT_FOR_START; break;
        default:   buf.clear(); state = WAIT_FOR_START;
      }
      continue;
    }
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> getints(const std::string& s)
{
  std::vector<int> res; // vector to store results
  std::string buf;  // string buffer to hold characters which may be ints
  enum IntState
  {
    WAITING, // waiting for first digit or minus sign
    NEG,    // found a minus sign, looking to see if this starts a negative int
    INT,    // found a digit, now recording successive digits to buffer
    IGNORE  // ignoring any digits between decimal point and next non-number
  };
  IntState state = WAITING; // current state of fsm.
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
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// include ints but not hex, octal or scientific notation (eg 10e5)

std::vector<float> getnums(const std::string& s)
{
  std::vector<float> res; // vector to store and return results
  std::string buf; // a buffer to hold characters until stored or discarded
  enum FloatState // The possible states of the fsm
  {
    WAITING, // awaiting the first character that might represent a number
    NEG,    // found a minus sign. Could be start of negative number
    PRE,    // Now reading an integer, waiting for whitespace or decimal point
    POST    // Have read integer and found point, now reading fractional number
  };
  FloatState state = WAITING; // current state of fsm
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
// Loads a file's contents into a single std::string using <fstream>

std::string get_file(const std::string& file)
{
  string filestring; // a new string in which to store file contents.
  ifstream in(file.c_str(), ios::in | ios::binary); // open connection to file
  auto fileCon = &in;
  fileCon->seekg(0, ios::end); // move to end of file
  size_t filesize = fileCon->tellg(); // reads position
  filestring.resize(filesize); // ensure string is big enough
  fileCon->seekg(0, ios::beg); // move to start of file
  fileCon->read(&filestring[0], filestring.size()); // copy contents
  fileCon->seekg(0, ios::beg); // move back to start of file
  fileCon->close(); // Ensure the connection is closed before proceeding
  return filestring;
}

/*--------------------------------------------------------------------------*/
// converts unicode points to multibyte utf-8 encoding

std::string utf(std::vector<Unicode> u)
{
  std::vector<uint8_t> res;
  for(auto x : u)
  {
    if(x < 0x0080) res.push_back(x & 0x007f);
    if(x > 0x007f && x < 0x0800)
    {
      res.push_back((0x00c0 | ((x >> 6) & 0x001f)));
      res.push_back(0x0080 | (x & 0x003f));
    }
    if(x > 2047)
    {
      res.push_back(0x00e0 | ((x >> 12) & 0x000f));
      res.push_back(0x0080 | ((x >> 6) & 0x003f));
      res.push_back(0x0080 | ((x) & 0x003f));
    }
  }
  std::string resstring(res.begin(), res.end());
  return resstring;
}
