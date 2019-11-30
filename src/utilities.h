//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities header file                                               //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_UTILTIES

//---------------------------------------------------------------------------//

#define PDFR_UTILTIES

/* This file is the first point in a daisy-chain of header files that
 * constitute the program. Since every other header file
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

#include "charstring.h"
#include<numeric>
#include<string>
#include<vector>
#include<unordered_map>
#include<algorithm>
#include<memory>
#include<array>
#include<iostream>

//---------------------------------------------------------------------------//
/* The characters in pdf strings are most portably interpreted as uint16_t.
 * They need to be translated to Unicode for rendition to the intended
 * characters. Since Unicode is best handled as uint16_t too, it is easy
 * to get confused about pre-translation to Unicode and post-translation.
 * There are thus two typedefs for uint16_t to allow easy tracking of which
 * is which.
 */

typedef uint16_t RawChar;
typedef uint16_t Unicode;


/*---------------------------------------------------------------------------*/
// A fixed static array to allow quick lookup of characters for use in the
// several lexers in this program. The position of each element represents an
// input char. The value is 'L' for any letter, 'D' for any digit, ' ' for any
// whitespace, and otherwise just the value of the char itself. This prevents
// having to go through a bunch of 'if' statements every time we look up a
// character in the lexer, since this may be done hundreds of thousands of times
// for each document.

static const std::array<uint8_t, 256> s_symbol_type =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x20, 0x20, 0x0b, 0x0c, 0x20, 0x0e, 0x0f,
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


//---------------------------------------------------------------------------//
// Template returning a vector of a std::map's keys. This can be used to
// enumerate the keys for iterating through the map or as a method of printing
// the map to the console

template< typename K, typename V > // K, V stand for key, value
std::vector<K> GetKeys(const std::unordered_map<K, V>& p_map)
{
  // vector to store results
  std::vector<K> result;

  // Ensure it is big enough
  result.reserve(p_map.size());

  // Declare map iterator according to its type
  typename std::unordered_map<K, V>::const_iterator it;

  // The following loop iterates through the map, gets the key and stores it
  for (it = p_map.begin(); it != p_map.end(); ++it) result.push_back(it->first);

  return result;
}

//---------------------------------------------------------------------------//
// Simple template to shorten boilerplate of sticking vectors together
// Given two vectors of the same type, add B's contents to the end of A
// This modifies A

template <typename T>
inline void Concatenate(std::vector<T>& p_start, const std::vector<T>& p_append)
{
  p_start.insert(p_start.end(), p_append.begin(), p_append.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); returns the indices which if applied would sort the
// vector from lowest to highest

template <typename T>
std::vector<int> Order(std::vector<T> p_data)
{
  std::vector<int> index(p_data.size(), 0); // a new int vector to store results
  for (size_t i = 0; i < p_data.size(); ++i)
  {
    index[i] = std::count_if(p_data.begin(), p_data.end(),
               [&](T other) { return other < p_data[i]; });
  }

  return index;
}

//---------------------------------------------------------------------------//
// Sort one vector by another's order. Modifies supplied vector

template <typename Ta, typename Tb>
std::vector<Ta> SortBy(std::vector<Ta> p_sortee,
                       const std::vector<Tb>& p_sorter)
{
  // Nothing to do!
  if (p_sortee.empty()) return p_sortee;

  // Throw error if lengths don't match
  if (p_sortee.size() != p_sorter.size())
  {
    throw std::runtime_error("SortBy requires equal-lengthed vectors");
  }

  // Vector to store results
  std::vector<Ta> result;

  // Use Order(p_sorter) as defined above to sort sortee
  for (auto i : Order(p_sorter)) result.emplace_back(p_sortee[i]);

  return result;
}

//---------------------------------------------------------------------------//
// A Bare bones tree class. This holds a value / object at each node whether
// it is a root or leaf. It requires a bit of thought to populate, since it
// will mostly require filling by recursion to be useful. It can be used for
// representing the tree structures implicit in the page object layout of large
// PDF files. However, it has not been used at present simply because it did
// not confer any performance advantage over a slightly more involved method
// using std::list

template <typename T>
class Tree
{
public:
   Tree<T>(const T& p_item) : parent_(nullptr), children_({}), item_(p_item){};
   Tree<T>(Tree<T>* p_ptr, const T& p_item) :
     parent_(p_ptr), children_({}), item_(p_item) {};

   void AddChild(const T& p_item) {
     children_.emplace_back(std::make_shared<Tree<T>>(this, p_item));}

   void AddChildren(const std::vector<T>& p_kids)
   {
     for (const auto& i : p_kids) AddChild(i);
   }

   std::vector<std::shared_ptr<Tree<T>>> GetChildren() { return children_;}

   bool IsRoot() { return parent_ == nullptr;}
   bool IsLeaf() { return children_.empty();}
   T& GetItem() { return item_;}

   void GetDescendantLeafs(std::vector<T>& p_store)
   {
     for(auto& child : children_)
     {
       if(child->IsLeaf()) p_store.push_back(child->GetItem());
       else child->GetDescendantLeafs(p_store);
     }
   }

private:
  Tree<T>* parent_;
  std::vector<std::shared_ptr<Tree<T>>> children_;
  T item_;
};

enum CharType
{
  LAB, LET, DIG, USC, LSB, FSL, AST, LCB, SUB, APO,
  BSL, SPC, RAB, PER, ADD, QOT, RCB, RSB, SQO, OTH
};



// A 'magic number' to specify the maximum length that the dictionary
// lexer will look through to find a dictionary
static const size_t MAX_DICT_LEN = 100000;

class Reader
{
public:
  Reader() : start_(nullptr), first_(0), last_(0), size_(0) {}

  Reader(std::shared_ptr<const std::string> p_input, size_t p_start) :
    start_(p_input->c_str()),
    first_(p_start),
    last_(p_start),
    size_(p_input->size()) {}

  Reader(std::shared_ptr<const std::string> p_ptr) {*this = Reader(p_ptr, 0);}

  Reader(const std::string& p_input) :
    start_(p_input.c_str()),
    first_(0),
    last_(0),
    size_(p_input.size()) {}

  Reader(const CharString& p_input) :
    start_(p_input.begin()),
    first_(0),
    last_(0),
    size_(p_input.end() - p_input.begin()) {}

  void operator++() {++last_;}
  void operator--() {--last_;}
  void SkipFirstChar() { if (first_ < last_) ++first_;}
  std::string Contents() const {return std::string(start_+first_, last_- first_);}

  CharString Out() const { return CharString(start_ + first_, last_ - first_);}

  char GetChar() const {return *(start_ + last_);}

  bool StartsString(const std::string& p_string) const
  {
    std::string test_string(start_ + first_, p_string.size());
    if (p_string.size() > (size_ - last_)) return false;
    return p_string == test_string;
  }

  bool operator==(const char* p_literal)
  {
    auto store = first_;
    while(*p_literal)
    {
      if (*p_literal++ != *(start_ + first_++))
      {
        first_ = store; return false;
      }
    }
    first_ = store;
    return true;
  }
  bool empty() const {return last_ == first_;}
  bool HasOverflowed() const {return !(last_ < size_);}
  size_t first() const {return first_;}
  size_t last() const {return last_;}
  size_t size() const {return size_;}
  CharType GetCharType() const { return char_lookup_[this->GetChar()];}

  void Clear() {first_ = last_;}

private:
  static const std::array<CharType, 256> char_lookup_;
  const char* start_;
  size_t first_;
  size_t last_;
  size_t size_;
};



//---------------------------------------------------------------------------//
//                                                                           //
//                      global function declarations                         //
//                                                                           //
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Return the first substring of s that lies between two strings.
// This can be used e.g. to find the byte position that always sits between
// "startxref" and "%%EOF"

std::string CarveOut(const std::string& p_string_to_be_carved,
                     const std::string& p_left_delimiter,
                     const std::string& p_right_delimiter);

//---------------------------------------------------------------------------//
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string> MultiCarve(const std::string& p_string_to_be_carved,
                                    const std::string& p_left_delimiter,
                                    const std::string& p_right_delimiter);

//---------------------------------------------------------------------------//
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const std::string& p_string_to_be_tested);

//---------------------------------------------------------------------------//
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

std::vector<uint8_t> ConvertHexToBytes(const std::string& p_hex_encoded_string);

//---------------------------------------------------------------------------//
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

std::string ConvertIntToHex(int p_int_to_be_converted);

/*---------------------------------------------------------------------------*/
// Classifies characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace.
// e.g.
//
// GetSymbolType( 'a') == 'L'; // letter
// GetSymbolType( '9') == 'D'; // digit
// GetSymbolType('\t') == ' '; // space
// GetSymbolType( '#') == '#'; // not a letter, digit or space. Returns itself

inline char GetSymbolType(const char& p_char)
{
  // if none of the above, return the char itself;
  return s_symbol_type[(uint8_t) p_char];
}

//---------------------------------------------------------------------------//
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

std::vector<RawChar> ConvertHexToRawChar(std::string& p_hex_encoded_string);

//---------------------------------------------------------------------------//
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

std::vector<RawChar> ConvertStringToRawChar(const std::string& p_input_string);

//---------------------------------------------------------------------------//
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

std::vector<int> ParseReferences(const std::string& p_string_to_be_parsed);

//---------------------------------------------------------------------------//
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> ParseInts(const std::string& p_string_to_be_parsed);

//---------------------------------------------------------------------------//
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// consume and convert integers but not hex, octal or scientific notation

std::vector<float> ParseFloats(const std::string& p_string_to_be_parsed);

//---------------------------------------------------------------------------//
// Loads a file's contents into a single std::string using <fstream>

std::string GetFile(const std::string& p_path_to_file);

//---------------------------------------------------------------------------//
// Prints bytes to screen for debugging

void PrintBytes(std::vector<uint8_t> p_bytes, const std::string& p_message);

#endif
