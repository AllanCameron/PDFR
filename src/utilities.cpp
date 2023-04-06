//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities implementation file                                       //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include<fstream>
#include<iostream>
#include<iomanip>

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

const std::array<CharType, 256> Reader::char_lookup_ =
{
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, SPC, OTH, OTH, SPC, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  SPC, OTH, QOT, OTH, OTH, OTH, SQO, APO,
  LCB, RCB, AST, ADD, OTH, SUB, PER, FSL,
  DIG, DIG, DIG, DIG, DIG, DIG, DIG, DIG,
  DIG, DIG, OTH, OTH, LAB, OTH, RAB, OTH,
  OTH, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LSB, BSL, RSB, OTH, USC,
  OTH, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH
};

/*---------------------------------------------------------------------------*/
// Returns the first substring of haystack that lies between two delimiters.
// e.g.
//
// CarveOut("Hello there world!", "Hello", "world") == " there ";

string CarveOut(const string& haystack,
                const string& left,
                const string& right)
{
  // Find the starting point of the left delimiter
  int start =  haystack.find(left);

  // If left delimiter absent, start at haystack[0], otherwise start at the end
  // of first occurrence of left delimiter
  if (start >= 0) start += left.size();
  else start = 0;

  // Now find the starting point of the first occurrence of right delimiter
  // in the remaining string
  int length = haystack.find(right, start);
  length -= start;

  // If not found, stop at the end of the string
  if (length < 0) length = haystack.length() - start;
  return haystack.substr(start, length);
}

/*---------------------------------------------------------------------------*/
// Finds all closest pairs of delimiters and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text. e.g.
//
// string target("I'm not a pheasant plucker, I'm a pheasant plucker's son");
// auto result = MultiCarve(target, "I'm", "plucker");
// result == vector<string> {" not a pheasant ", " a pheasant "};
//

vector<string> MultiCarve(const string& haystack,
                          const string& left,
                          const string& right)
{
  vector<string> result;

  // If any of the strings are length 0 then return an empty vector
  if (haystack.empty() || left.empty() || right.empty()) return result;

  // Makes a copy to allow const correctness
  string trimmed(haystack);

  // This loop finds the first occurrence of the left delimiter, then stores
  // a copy of the portion of haystack between the end of the left delimiter
  // and the start of the right delimiter. It then trims the beginning of the
  // string, leaving the portion after the right delimiter. It repeats until
  // the whole haystack is consumed
  while (true)
  {
    int start = trimmed.find(left);
    if (start == -1) break;

    // Chops beginning off haystack up to end of first match of left delimiter
    int end_of_match     = start + left.length();
    int remaining_length = trimmed.length() - end_of_match;
    trimmed              = trimmed.substr(end_of_match, remaining_length);

    int stop = trimmed.find(right);
    if (stop == -1) break;

    // Target found - push to result
    result.push_back(trimmed.substr(0, stop));

    // Now discard the target plus the following instance of right delimiter
    end_of_match     = stop + right.length();
    remaining_length = trimmed.length() - end_of_match;
    trimmed          = trimmed.substr(end_of_match, remaining_length);
  }

  return result;
}

/*---------------------------------------------------------------------------*/
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std. e.g.
//
// IsAscii("I am an Ascii string.") == true;
// IsAscii("I am an Äscii string.") == false;

bool IsAscii(const string& candidate)
{
  if (candidate.empty()) return false; // Not sure if this is true or false

  // Use minmax to get a pair of iterators pointing to min & max char values
  auto&& minmax_ptrs = minmax_element(candidate.begin(), candidate.end());

  // If any are outside the ascii range return false, otherwise return true
  return *(minmax_ptrs.first) > 7 && *(minmax_ptrs.second) < 127;
}

/*---------------------------------------------------------------------------*/
// Converts an Ascii-encoded string of bytes to a vector of bytes, e.g.
//
// ConvertHexToBytes("01ABEF2A") == vector<uint8_t> { 0x01, 0xAB, 0xEF, 0x2A };

vector<uint8_t> ConvertHexToBytes(const string& hexstring)
{
  vector<uint8_t> byte_vector{};
  byte_vector.reserve(hexstring.size());

  // If hexstring is empty, return an empty vector;
  if (hexstring.empty()) return byte_vector;

  for (auto hexchar : hexstring)
  {
    const auto& found = s_hexmap.find(hexchar);
    if (found != s_hexmap.end()) byte_vector.push_back(found->second);
  }

  // We cannot allow odd-length vectors;
  if (byte_vector.size() == 0x01) byte_vector.push_back(0);

  // Now take each pair of four-bit bytes, left shift the first by four bits
  // and add them together using a bitwise OR, overwriting the source as we go
  for (size_t i = 0; i < byte_vector.size(); i += 2)
  {
    byte_vector[i / 2] = (0xf0 & (byte_vector[i + 0] << 4)) |
                         (0x0f &  byte_vector[i + 1] << 0);
  }

  // Remove the non-overwritten part of the vector.
  byte_vector.resize(byte_vector.size()/2);
  return byte_vector;
}

/*---------------------------------------------------------------------------*/
// Converts an int to the relevant 2-byte ASCII hex string (4 characters long)
// e.g.
//
// ConvertIntToHex(161) == string("00A1");

string ConvertIntToHex(int input)
{
  if (input < 0 || input > 0xffff) return "FFFF"; // Returns max if out of range
  string hex {"0123456789ABCDEF"};
  hex.reserve(20);
  hex += hex[(input & 0xf000) >> 12];  // Gets index of hex from first 4 bits
  hex += hex[(input & 0x0f00) >>  8];  // Gets index of hex from second 4 bits
  hex += hex[(input & 0x00f0) >>  4];  // Gets index of hex from third 4 bits
  hex += hex[(input & 0x000f) >>  0];  // Gets index of hex from last 4 bits
  return string {hex, 16, 4};
}

/*--------------------------------------------------------------------------*/
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers e.g.
//
// ConvertHexToRawChar("ABCD0123") == vector<RawChar> {0xABCD, 0x0123};

vector<RawChar> ConvertHexToRawChar(string& hexstring)
{
  // Prepends zeros until the string can be split into length-4 sections
  while (hexstring.size() % 4) hexstring = '0' + hexstring;

  // Declares vector to store results and ensures it is large enough to do so
  vector<RawChar> raw_vector;
  raw_vector.reserve(hexstring.size() / 4);

  // Note this loop reads 4 chars at a time and stops incrementing at size - 3.
  // It looks up each character in the hexmap and places it in the correct
  // 4-bit section of the 16-bit result using the bit shift operator.
  for (size_t i = 0; i < (hexstring.size() - 3); i += 4)
  {
    raw_vector.emplace_back(((s_hexmap[hexstring[i + 0]] & 0x000f) << 12)  |
                            ((s_hexmap[hexstring[i + 1]] & 0x000f) <<  8)  |
                            ((s_hexmap[hexstring[i + 2]] & 0x000f) <<  4)  |
                            ((s_hexmap[hexstring[i + 3]] & 0x000f) <<  0)  );
  }
  return raw_vector;
}

/*--------------------------------------------------------------------------*/
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)
// e.g.
//
// ConvertStringToRawChar("Hello") ==
// vector<RawChar> { 0x0048, 0x0065, 0x006c, 0x006c, 0x006f};

vector<RawChar> ConvertStringToRawChar(const string& std_string)
{
  vector<RawChar> result {};            // Declare result vector
  if (std_string.empty()) return result; // If string empty, return empty vector
  result.reserve(std_string.size());      // Otherwise, reserve enough space
  for (auto string_char : std_string)     // Then place 2 bytes per char in result
  {
    result.emplace_back(0x00ff & string_char);
  }
  return result;
}

/*--------------------------------------------------------------------------*/
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found
// e.g.
//
// ParseReferences("<</Refs 1 0 R 2 0 R 31 5 R>>") == vector<int> {1, 2, 31};

vector<int> ParseReferences(const string& ref_string)
{
  // Defines the possible states of the finite state machine (fsm)
  enum ReferenceState
  {
    START,          // Waiting for an integer
    IN_FIRST_INT,   // In an integer
    WAIT_FOR_GEN,   // Waiting for a generation number
    IN_GEN,         // In a second integer (generation number)
    WAIT_FOR_R      // Wait to see if next char is R to confirm this is a ref
  };

  vector<int> result;            // Vector to hold and return result
  string buffer;                 // Buffer to hold characters until needed
  ReferenceState state = START;  // Current state of finite state machine

  // The main loop cycles through each char in the string to write the result
  for (const auto& chr : ref_string)
  {
    char m = GetSymbolType(chr);
    switch (state)
    {
      // To begin with, ignore all input until a digit is reached
      case START:  if (m == 'D'){ buffer += chr; state = IN_FIRST_INT;}
                            break;

      // Now in the first int. Write digits to the buffer until a space is
      // reached, at which point we assume we have found the object number
      // and are waiting for the generation number. If we come across anything
      // other than a space or digit, we were not in the object number and we
      // go back to a waiting state while progressing through the string.
      case IN_FIRST_INT:    switch (m)
                            {
                              case 'D' : buffer += chr;         break;
                              case ' ' : state = WAIT_FOR_GEN;  break;
                              default  : buffer.clear();
                                         state = START;
                            }
                            break;

      // We have come across a single int, which is sitting in the buffer, and
      // a subsequent space, so if all goes to plan the next char should be a
      // digit. If so, we proceed; otherwise, we start looking again from the
      // current point in the string
      case WAIT_FOR_GEN:    if (m == 'D') state = IN_GEN;
                            else {buffer.clear(); state = START;}
                            break;

      // We now have an integer stored in the buffer, and confirmed this was
      // followed by a space then another digit. We expect the next char to be
      // a space, or very rarely a digit followed by a space if the generation
      // number is >9. A digit therefore means we stay in this state waiting for
      // a space. A space means we should switch to waiting for an R. Any other
      // char means this is not a reference and we start looking again from
      // this point in the string
      case IN_GEN:          switch (m)
                            {
                              case 'D' :                        break;
                              case ' ' : state = WAIT_FOR_R;    break;
                              default  : buffer.clear();
                                         state = START;
                            }
                            break;

      // If we get here, we have come through two integers, each of which has
      // been followed by a space. The first is stored in the buffer. If the
      // next char is 'R', this was a reference and we store the integer in
      // the buffer as an int. In either case, we wipe the buffer and start
      // looking for the next reference until the end of the given string
      case WAIT_FOR_R:      if (chr == 'R') result.push_back(stoi(buffer));
                            buffer.clear(); state = START;
                            break;
    }
  }
  return result;
}

/*--------------------------------------------------------------------------*/
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)
// e.g.
//
// ParseInts("<</Refs 1 0 R 2 0 R 31 5 R>>") == vector<int> {1, 0, 2, 0, 31, 5};

vector<int> ParseInts(const CharString& int_string)
{
  // Define the possible states of the lexer
  enum IntState
  {
    WAITING,// Waiting for digit or minus sign
    NEG,    // Found a minus sign, looking to see if this starts a negative int
    INT,    // Found a digit, now recording successive digits to buffer
    IGNORE  // Ignoring any digits between decimal point and next non-number
  };

  vector<int> result;       // Vector to store results
  int buffer = 0;            // String buffer to hold chars which may be ints
  int neg = 1;
  IntState state = WAITING; // Current state of the finite state machine.

  // The main loop cycles through each char in the string to write the result
  for (auto it = int_string.begin(); it != int_string.end(); ++it)
  {
    char chr = *it;
    char m = GetSymbolType(chr);
    switch (state)
    {
      case WAITING: if (m == 'D')
                    {
                        buffer += (int) chr - 48;
                        state = INT;
                    }
                    else if (chr == '-')
                    {
                      neg = -1;
                      state = NEG;
                    }
                    break;

      case NEG    : if (m == 'D')
                    {
                      buffer = (int) chr - 48;
                      state = INT;
                    }
                    else { neg = 1; state = WAITING;}
                    break;

    case INT    : if (m == 'D') {buffer *= 10; buffer += (int) chr - 48;}
                    else
                    {
                      result.push_back(neg * buffer);
                      buffer = 0; neg = 1;
                      if (chr == '.') state = IGNORE;
                      else            state = WAITING;
                    }
                    break;

      case IGNORE : if (m != 'D') state = WAITING; break;
    }
  }
  if (state != WAITING) result.push_back(neg * buffer);
  return result;
}

vector<int> ParseInts(const string& int_string)
{
  return ParseInts(CharString(int_string));
}
/*--------------------------------------------------------------------------*/
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// include ints but not hex, octal or scientific notation (eg 10e5)
// e.g.
//
// ParseFloats("pi is 3.14, e is 2.72") == vector<float> {3.14, 2.72};

vector<float> ParseFloats(const string& float_string)
{
  enum FloatState  // The possible states of the finite state machine
  {
    WAITING,       // Awaiting a character that might represent a number
    PRE,           // Reading an integer until whitespace or decimal point
    POST           // Reading fractional number after integer and point found
  };

  vector<float> result;        // Vector to store and return results
  int pre = 0;
  int post = 0;
  int neg = 1;
  int post_size = 1;
  FloatState state = WAITING;  // Current state of the finite state machine

  // The main loop cycles through each char in the string to write the result
  for (const auto& chr : float_string)
  {
    char m = GetSymbolType(chr);
    switch (state)
    {
    case WAITING: if (m == 'D'){ pre = (int) chr - 48; state = PRE;}
                  else if (chr == '-'){ neg = -1; state = PRE;}
                  else if (chr == '.'){ state = POST;}
                  break;

    case PRE:     if (m == 'D') {pre *= 10; pre += (int) chr - 48;}
                  else if (chr == '.'){ state = POST;}
                  else
                  {
                    if (chr != '-') result.push_back((float) pre);
                    pre = 0; state = WAITING;
                  }
                  break;

    case POST:    if (m == 'D') {post *= 10; post_size *= 10;
                                 post += (int) chr - 48;}
                  else{ result.push_back(neg * (pre + ((float) post / post_size)));
                        pre = post = 0; neg = 1; post_size = 1;
                        state = WAITING;}
                  break;
    }
  }

  if (state != WAITING)
    result.push_back(neg * (pre + ((float) post / post_size)));
  return result;
}

/*--------------------------------------------------------------------------*/
// Loads a file's contents into a single std::string using <fstream>
// e.g.
//
// string file_contents = GetFile("C://documents/my_binary_file.bin");

string GetFile(const string& file_name)
{
  string file_string;  // A new string in which to store file contents.

  // Opens connection to the file. If it fails to open, it throws an error.
  ifstream file_stream(file_name.c_str(), ios::in | ios::binary);
  if (!file_stream) throw runtime_error("Couldn't open file.");

  file_stream.seekg(0, ios::end);           // Moves to end of file
  file_string.resize(file_stream.tellg());  // Ensures string is big enough
  file_stream.seekg(0, ios::beg);           // Moves back to start of file
  file_stream.read(&file_string[0], file_string.size()); // Copies contents
  file_stream.close(); // Ensures the connection is closed before proceeding

  return file_string;
}

/*--------------------------------------------------------------------------*/
// Allows a vector of bytes to be output to a stream using the << operator

std::ostream& operator<<(std::ostream& os, std::vector<uint8_t> bytes)
{
  os << "(";
  for(auto byte = bytes.begin(); byte != (bytes.end() - 1); ++byte)
  {
    os << "0x" << setfill('0') << setw(2) << hex << (int) *byte;
    os << ", ";
  }
  os << "0x" << setfill('0') << setw(2) << hex << (int) *(bytes.end() - 1);
  os << ")" << endl;
  return os;
}

