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

static unordered_map<char, uint8_t> s_hexmap =
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

static array<uint8_t, 256> s_symbol_type =
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

string carve_out(const string& t_string,
                 const string& t_left,
                 const string& t_right)
{
  // Find the starting point of first 'pre'
  int start =  t_string.find(t_left);

  // If pre not found in s, start at s[0], otherwise start at end of first pre
  if (start) start += t_left.size();

  else start = 0;

  // Trim the start of string
  string trimmed(t_string.substr(start, t_string.size() - start));

  // Now find the starting point of 'post'
  int stop = trimmed.find(t_right);

  // If post found, discard the end of the string starting at start of post
  if (stop) return trimmed.substr(0, stop);

  // if post not found, finish at the end of the string
  return trimmed;
}

/*---------------------------------------------------------------------------*/
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

vector<string> multicarve(const string& t_string,
                          const string& t_left,
                          const string& t_right)
{
  // vector to store results
  vector<string> result;

  // if any of the strings are length 0 then return an empty vector
  if (t_string.empty() || t_left.empty() || t_right.empty()) return result;

  // makes a copy to allow const correctness
  string trimmed(t_string);

  // This loop progressively finds matches in s and removes from the start
  // of the string all characters up to the end of matched b.
  // It does this until there are no paired matches left in the string
  while(true)
  {
    int start = trimmed.find(t_left);
    if(start == -1) break;

    // chop start off string up to end of first match of a
    int end_of_match     = start + t_left.length();
    int remaining_length = trimmed.length() - end_of_match;
    trimmed              = trimmed.substr(end_of_match, remaining_length);

    int stop = trimmed.find(t_right);
    if(stop == -1) break;

    // Target found - push to result
    result.push_back(trimmed.substr(0, stop));

    // Now discard the target plus the following instance of b
    end_of_match     = stop + t_right.length();
    remaining_length = trimmed.length() - end_of_match;
    trimmed          = trimmed.substr(end_of_match, remaining_length);
  }

  return result;
}

/*---------------------------------------------------------------------------*/
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool is_ascii(const string& t_string)
{
  if(t_string.empty()) return false;

  // Use minmax to get a pair of iterators pointing to min & max char values
  auto minmax_ptrs = minmax_element(t_string.begin(), t_string.end());

  // If any are outside the ascii range return false, otherwise return true
  return *(minmax_ptrs.first) > 7 && *(minmax_ptrs.second) < 127;
}

/*---------------------------------------------------------------------------*/
// Converts an Ascii-encoded string of bytes to a vector of bytes

vector<uint8_t> convert_hex_to_bytes(const string& t_hexstring)
{
   vector<uint8_t> byte_vector{};

  // If hexstring is empty, return an empty vector;
  if(t_hexstring.empty()) return byte_vector;

  for(auto hexchar : t_hexstring)
  {
    const auto& found = s_hexmap.find(hexchar);
    if(found != s_hexmap.end()) byte_vector.push_back(found->second);
  }

  // We cannot allow odd-length vectors;
  if(byte_vector.size() | 0x01) byte_vector.push_back(0);

  // Now take each pair of four-bit bytes, left shift the first by four bits
  // and add them together using a bitwise OR, overwriting the source as we go
  for(size_t i = 0; i < byte_vector.size(); i += 2)
  {
    byte_vector[i / 2] = (0xf0 & (byte_vector[i] << 4)) |
                         (0x0f &  byte_vector[i + 1]);
  }

  // Remove the non-overwritten part of the vector.
  byte_vector.resize(byte_vector.size()/2);
  return byte_vector;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

string convert_int_to_hex(int t_int)
{
  if(t_int < 0 || t_int > 0xffff) return "FFFF"; // returns max if out of range
  string hex {"0123456789ABCDEF"};
  hex += hex[(t_int & 0xf000) >> 12]; // gets index of hex from first 4 bits
  hex += hex[(t_int & 0x0f00) >>  8]; // gets index of hex from second 4 bits
  hex += hex[(t_int & 0x00f0) >>  4]; // gets index of hex from third 4 bits
  hex += hex[(t_int & 0x000f) >>  0]; // gets index of hex from last 4 bits
  return string {hex, 16, 4};
}

/*---------------------------------------------------------------------------*/
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char get_symbol_type(const char t_char)
{
  // if none of the above, return the char itself;
  return s_symbol_type[(uint8_t) t_char];
}

/*--------------------------------------------------------------------------*/
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

vector<RawChar> convert_hex_to_rawchar(string& t_string)
{
  while(t_string.size() % 4) t_string = '0' + t_string;
  vector<RawChar> raw_vector; // vector to store results
  raw_vector.reserve(t_string.size() / 4);
  for(size_t i = 0; i < (t_string.size() - 3); i += 4)
  {
    raw_vector.emplace_back(((s_hexmap[t_string[i]] & 0x000f) << 12)      |
                            ((s_hexmap[t_string[i + 1]] & 0x000f) << 8)  |
                            ((s_hexmap[t_string[i + 2]] & 0x000f) << 4)  |
                             (s_hexmap[t_string[i + 3]] & 0x000f));
  }
  return raw_vector;
}

/*--------------------------------------------------------------------------*/
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

vector<RawChar> convert_string_to_rawchar(const string& t_string)
{
  vector<RawChar> result; // vector to hold results
  result.reserve(t_string.size());

  if(!t_string.empty())
  {
    for(auto string_char : t_string)
    {
      result.emplace_back(0x00ff & ((uint8_t) string_char)); // convert uint16
    }
  }
  return result;
}

/*--------------------------------------------------------------------------*/
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

vector<int> parse_references(const string& t_string)
{
  enum RefState // defines the possible states of the finite state machine (fsm)
  {
    WAIT_FOR_START, // Waiting for an integer
    IN_FIRST_INT,   // In an integer
    WAIT_FOR_GEN,   // Waiting for a generation number
    IN_GEN,         // In a second integer
    WAIT_FOR_R      // Wait to see if next char is R to confirm this is a ref
  };
  vector<int> result; // vector to hold result
  string buf; // a buffer to hold characters until stored or discarded
  RefState state = WAIT_FOR_START; // the current state of the fsm
  for(const auto& i : t_string)
  {
    char m = get_symbol_type(i);
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
      case WAIT_FOR_R:      if(i == 'R') result.push_back(stoi(buf));
                            buf.clear(); state = WAIT_FOR_START;
                            break;
    }
  }
  return result;
}

/*--------------------------------------------------------------------------*/
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

vector<int> parse_ints(const string& t_string)
{
  vector<int> result; // vector to store results
  string buf;  // string buffer to hold characters which may be ints
  enum IntState
  {
    WAITING,// waiting for digit or minus sign
    NEG,    // found a minus sign, looking to see if this starts a negative int
    INT,    // found a digit, now recording successive digits to buffer
    IGNORE  // ignoring any digits between decimal point and next non-number
  };
  IntState state = WAITING; // current state of fsm.
  for(auto i : t_string)
  {
    char m = get_symbol_type(i);
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
                      if(buf != "-") result.push_back(stoi(buf));
                      buf.clear();
                      if(i == '.') state = IGNORE;
                      else         state = WAITING;
                    }
                    break;
      case IGNORE : if(m != 'D') state = WAITING; break;
    }
  }
  if(state == INT && !buf.empty()) result.push_back(stoi(buf));
  return result;
}

/*--------------------------------------------------------------------------*/
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// include ints but not hex, octal or scientific notation (eg 10e5)

vector<float> parse_floats(const string& t_string)
{
  vector<float> result; // vector to store and return results
  string buf; // a buffer to hold characters until stored or discarded
  enum FloatState // The possible states of the fsm
  {
    WAITING,// awaiting the first character that might represent a number
    NEG,    // found a minus sign. Could be start of negative number
    PRE,    // Now reading an integer, waiting for whitespace or decimal point
    POST    // Have read integer and found point, now reading fractional number
  };
  FloatState state = WAITING; // current state of fsm
  for(const auto& i : t_string)
  {
    char m = get_symbol_type(i);
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
                    if(buf != "-") result.push_back(stof(buf));
                    buf.clear(); state = WAITING;
                  }
                  break;
    case POST:    if(m == 'D') buf += i;
                  else{ result.push_back(stof(buf));
                        state = WAITING; buf.clear();}
                  break;
    }
  }
  if(state == PRE && !buf.empty()) result.push_back(stof(buf));
  if(state == POST && buf != "-0.") result.push_back(stof(buf));
  return result;
}

/*--------------------------------------------------------------------------*/
// Loads a file's contents into a single std::string using <fstream>

string get_file(const string& file)
{
  // A new string in which to store file contents.
  string filestring;

  // Open connection to file
  ifstream fileCon(file.c_str(), ios::in | ios::binary);

  // Move to end of file
  fileCon.seekg(0, ios::end);

  // Ensure string is big enough
  filestring.resize(fileCon.tellg());

  // Move to start of file
  fileCon.seekg(0, ios::beg);

  // Copy contents
  fileCon.read(&filestring[0], filestring.size());

  // Ensure the connection is closed before proceeding
  fileCon.close();

  return filestring;
}


