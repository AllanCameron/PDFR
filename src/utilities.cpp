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

using namespace std;

/*---------------------------------------------------------------------------*/
// A cheap way to get the value of Ascii - encoded bytes is to look up their
// value in an unordered map. This can be done arithmetically, but that is
// probably less efficient and definitely less transparent

static std::unordered_map<char, uint8_t> hexmap =
{
  {'0',  0}, {'1',  1}, {'2',  2}, {'3',  3}, {'4',  4}, {'5',  5}, {'6',  6},
  {'7',  7}, {'8',  8}, {'9',  9}, {'a', 10}, {'A', 10}, {'b', 11}, {'B', 11},
  {'c', 12}, {'C', 12}, {'d', 13}, {'D', 13}, {'e', 14}, {'E', 14}, {'f', 15},
  {'F', 15}
};

/*---------------------------------------------------------------------------*/
// A fixed static array to allow quick lookup of characters for use in the
// several lexers in this program. The position of each element represents an
// input char. The value is 'L' for any letter, 'D' for any digit, ' ' for any
// whitespace, and otherwise just the value of the char itself. This prevents
// having to go through a bunch of 'if' statements every time we look up a
// character in the lexer, since this may be done hundreds of thousands of times
// for each document.

static std::array<uint8_t, 256> sym_t =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x20, 0x0b, 0x0c, 0x20, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
  0x44, 0x44, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
  0x4c, 0x4c, 0x4c, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/*---------------------------------------------------------------------------*/
// Return the first substring of s that lies between two 'bookend' strings.
// This can be used e.g. to find the byte position that always sits between
// "startxref" and "%%EOF"

string carveout(const string& s, const string& pre, const string& post)
{
  int start =  s.find(pre);
  if (start == -1) start++; // if pre not found in s, start at beginning of s
  else start += pre.size(); // otherwise start at end of first pre
  string str = s.substr(start, s.size() - start); // trim start of s
  int stp = str.find(post);
  if(stp == -1) return str; // if post not found, finish at end of string
  return str.substr(0, stp);// discard end of string starting at end of post
}

/*---------------------------------------------------------------------------*/
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

vector<string> multicarve(const string& s, const string& a, const string& b)
{
  std::vector<std::string> res; // vector to store results
  // if any of the strings are length 0 then return an empty vector
  if(a.empty() || b.empty() || s.empty()) return res;
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
  if(s.empty()) return false;
  auto i = minmax_element(s.begin(), s.end()); // pair of iterators, minmax char
  return *(i.first) > 7 && *(i.second) < 127; // none outside the ascii range?
}

/*---------------------------------------------------------------------------*/
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

vector<uint8_t> bytesFromArray(const string& s)
{
  vector<uint8_t> resvec, tmpvec; // vectors to store results
  if(s.empty()) return resvec; // if string is empty, return empty vector;
  for(auto a : s) // convert hex characters to numerical values using hexmap
  {
    auto& b = hexmap.find(a);
    if(b != hexmap.end())
      tmpvec.push_back(b->second);
  }
  if(tmpvec.empty()) return resvec; // No hex characters - return empty vector;
  if(tmpvec.size() % 2 == 1) tmpvec.push_back(0); // No odd-length strings!
  for(size_t i = 0; i < tmpvec.size(); i += 2)
    resvec.emplace_back( (0xf0 & (tmpvec[i] << 4)) |(0x0f & tmpvec[i + 1]));
  return resvec;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

string intToHexstring(int i)
{
  if(i < 0 || i > 0xffff) return "FFFF"; // returns max if out of range
  string hex {"0123456789ABCDEF"};
  hex += hex[(i & 0xf000) >> 12]; // gets index of hex from first 4 bits
  hex += hex[(i & 0x0f00) >>  8]; // gets index of hex from second 4 bits
  hex += hex[(i & 0x00f0) >>  4]; // gets index of hex from third 4 bits
  hex += hex[(i & 0x000f) >>  0]; // gets index of hex from last 4 bits
  return string {hex, 16, 4};
}

/*---------------------------------------------------------------------------*/
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char symbol_type(const char c)
{
  return sym_t[(uint8_t) c]; // if none of the above, return the char itself;
}

/*--------------------------------------------------------------------------*/
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

vector<RawChar> HexstringToRawChar(string& s)
{
  while(s.size() % 4 != 0) s = '0' + s;
  vector<RawChar> raw_vector; // vector to store results
  raw_vector.reserve(s.size() / 4);
  for(size_t i = 0; i < (s.size() - 3); i += 4)
  {
    raw_vector.emplace_back(((hexmap[s[i]] & 0x000f) << 12)      |
                            ((hexmap[s[i + 1]] & 0x000f) << 8)  |
                            ((hexmap[s[i + 2]] & 0x000f) << 4)  |
                             (hexmap[s[i + 3]] & 0x000f));
  }
  return raw_vector;
}

/*--------------------------------------------------------------------------*/
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

vector<RawChar> StringToRawChar(const string& s)
{
  vector<RawChar> result; // vector to hold results
  result.reserve(s.size());
  if(!s.empty())
    for(auto i : s)
      result.emplace_back(0x00ff & ((uint8_t) i)); // convert to uint16
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
  for(const auto& i : s)
  {
    char m = symbol_type(i); // finds out if current char is digit, letter, ws
    switch(state)
    {
      case WAIT_FOR_START:  if(m == 'D'){ buf += i; state = IN_FIRST_INT;}
                            break;
      case IN_FIRST_INT:    switch(m)
                            {
                              case 'D' : buf += i;              break;
                              case ' ' : state = WAIT_FOR_GEN;  break;
                              default  : buf.clear();
                                         state = WAIT_FOR_START;
                            }
                            break;
      case WAIT_FOR_GEN:    if(m == 'D') state = IN_GEN;
                            else         state = WAIT_FOR_START;
                            break;
      case IN_GEN:          switch(m)
                            {
                              case 'D' :                        break;
                              case ' ' : state = WAIT_FOR_R;    break;
                              default  : buf.clear();
                                         state = WAIT_FOR_START;
                            }
                            break;
      case WAIT_FOR_R:      if(i == 'R') res.push_back(stoi(buf));
                            buf.clear(); state = WAIT_FOR_START;
                            break;
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
    WAITING,// waiting for digit or minus sign
    NEG,    // found a minus sign, looking to see if this starts a negative int
    INT,    // found a digit, now recording successive digits to buffer
    IGNORE  // ignoring any digits between decimal point and next non-number
  };
  IntState state = WAITING; // current state of fsm.
  for(auto i : s)
  {
    char m = symbol_type(i);
    switch(state)
    {
      case WAITING: if(m == 'D')
                    {
                      if(buf.length() > 10) state = IGNORE;
                      else {buf += i;state = INT;}
                    }
                    else if(i == '-'){ buf += i; state = NEG;}
                    break;
      case NEG    : if(m == 'D'){buf += i; state = INT;}
                    else { buf.clear(); state = WAITING;}
                    break;
      case INT    : if(m == 'D') buf += i;
                    else
                    {
                      if(buf != "-") res.push_back(stoi(buf));
                      buf.clear();
                      if(i == '.') state = IGNORE;
                      else         state = WAITING;
                    }
                    break;
      case IGNORE : if(m != 'D') state = WAITING; break;
    }
  }
  if(state == INT && !buf.empty()) res.push_back(stoi(buf));
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
    WAITING,// awaiting the first character that might represent a number
    NEG,    // found a minus sign. Could be start of negative number
    PRE,    // Now reading an integer, waiting for whitespace or decimal point
    POST    // Have read integer and found point, now reading fractional number
  };
  FloatState state = WAITING; // current state of fsm
  for(const auto& i : s)
  {
    char m = symbol_type(i);
    switch(state)
    {
    case WAITING: if(m == 'D'){ buf += i; state = PRE;}
                  else if(i == '-'){ buf += i; state = NEG;}
                  else if(i == '.'){ buf += i; state = POST;}
                  break;
    case NEG:     if(m == 'D'){ buf += i; state = PRE;}
                  else if (i == '.'){ buf = "-0."; state = POST;}
                  else {buf.clear(); state = WAITING;}
                  break;
    case PRE:     if(m == 'D') buf += i;
                  else if (i == '.'){ buf += i; state = POST;}
                  else
                  {
                    if(buf != "-") res.push_back(stof(buf));
                    buf.clear(); state = WAITING;
                  }
                  break;
    case POST:    if(m == 'D') buf += i;
                  else{ res.push_back(stof(buf)); state = WAITING; buf.clear();}
                  break;
    }
  }
  if(state == PRE && !buf.empty()) res.push_back(stof(buf));
  if(state == POST && buf != "-0.") res.push_back(stof(buf));
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
// converts (16-bit) Unicode code points to multibyte utf-8 encoding.

std::string utf(const std::vector<uint16_t>& Unicode_points)
{
  std::string result_string = ""; // empty string for results
  for(auto& point : Unicode_points) // for each uint16_t in the input vector...
  {
    // values less than 128 are just single-byte ASCII
    if(point < 0x0080)
    {
      result_string.push_back(point & 0x007f);
      continue;
    }
    // values of 128 - 2047 are two bytes. The first byte starts 110xxxxx
    // and the second starts 10xxxxxx. The remaining 11 x's are filled with the
    // 11 bits representing a number between 128 and 2047. e.g. Unicode point
    // U+061f (decimal 1567) is 11000011111 in 11 bits of binary, which we split
    // into length-5 and length-6 pieces 11000 and 011111. These are appended on
    // to 110 and 10 respectively to give the 16-bit number 110 11000 10 011111,
    // which as two bytes is 11011000 10011111 or d8 9f. Thus the UTF-8
    // encoding for character U+061f is the two-byte sequence d8 9f.
    if(point > 0x007f && point < 0x0800)
    {
      // construct byte with bits 110 and first 5 bits of unicode point number
      result_string.push_back((0x00c0 | ((point >> 6) & 0x001f)));
      // construct byte with bits 10 and final 6 bits of unicode point number
      result_string.push_back(0x0080 | (point & 0x003f));
      continue;
    }

    // Unicode values between 2048 (0x0800) and the maximum uint16_t value
    // (65535 or 0xffff) are given by 16 bits split over three bytes in the
    // following format: 1110xxxx 10xxxxxx 10xxxxxx. Each x here takes one of
    // the 16 bits representing 2048 - 65535.
    if(point > 0x07ff)
    {
      // convert ligatures
      if(point == 0xFB00) {result_string += "ff"; continue;}
      if(point == 0xFB01) {result_string += "fi"; continue;}
      if(point == 0xFB02) {result_string += "fl"; continue;}
      if(point == 0xFB03) {result_string += "ffi"; continue;}
      if(point == 0xFB04) {result_string += "ffl"; continue;}
      // construct byte with 1110 and first 4 bits of unicode point number
      result_string.push_back(0x00e0 | ((point >> 12) & 0x000f));
      // construct byte with 10 and bits 5-10 of unicode point number
      result_string.push_back(0x0080 | ((point >> 6) & 0x003f));
      // construct byte with bits 10 and final 6 bits of unicode point number
      result_string.push_back(0x0080 | ((point) & 0x003f));
    }
    // Although higher Unicode points are defined and can be encoded in utf8,
    // the hex-strings in pdf seem to be two bytes wide at most. These are
    // therefore not supported at present.
  }
  return result_string;
}
