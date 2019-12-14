//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR CharString header file                                              //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//


#ifndef PDFR_CHARSTRING

//---------------------------------------------------------------------------//

#define PDFR_CHARSTRING

#include<string>
#include<iostream>

// The CharString class offers cheap, read-only access to std::strings and
// C-style strings without having to copy data at any point. It is effectively
// a glorified struct{const char* string, size_t length;} with member functions
// such as operator[](), size(), begin(), end(), find() and substr() that
// function as one might expect. It can be created from a std::string, a
// const char* or a string literal, and can be compared directly for equality
// against each of these using operator==(). It has its own output stream
// method so it can be written directly to the console.
//
// Although it doesn't own any other resources, it will only remain valid as
// long as the string to which it points exists, and this can make it
// problematic unless care is taken to ensure its lifetime falls strictly
// within the lifetime of the pointed-to string.
//
// It is used in PDFR because the entire pdf file is read into the free store as
// an std::string and sits there for the duration of the parsing process. It
// is therefore a safe and efficient tool for this job.
//
// This class is a wheel that has been reinvented many times, not least by the
// C++17 addition of string_view. My guess is that string_view is much more
// efficient, safe and portable than this class, but isn't available in C++11.
// I have tried to give the member functions the same names as those in
// string_view so that the code base can be easily upgraded in the future.
//
// Most of the methods are inlined, and only those that are a bit more complex
// such as the find() and substr() methods are defined seperately in the
// implementation file

class CharString
{
public:
  // There are several ways to construct a Charstring:

  // Give it a pointer, a starting offset and an endpoint
  CharString(const char* ptr, size_t start, size_t end) :
  begin_(ptr + start), length_(end - start) {}

  // Or just a pointer and a length
  CharString(const char* ptr, size_t length) : begin_(ptr), length_(length) {}

  // Or just a pointer to a zero-terminated string
  CharString(const char* ptr) :
  begin_(ptr), length_(0) { while (*(begin_ + length_)) ++length_; }

  // Or an std::string
  CharString(const std::string& s) : begin_(s.c_str()), length_(s.size()) {}

  // Or an std::string with a starting offset
  CharString(const std::string& str, size_t start) :
  begin_(str.c_str() + start), length_(str.size() - start) {}

  // Or another CharString
  CharString(const CharString&) = default;
  CharString& operator=(const CharString& chunk) = default;
  CharString& operator=(CharString&& chunk) noexcept = default;

  // Empty constructor
  CharString() : begin_(nullptr), length_(0) {}

  // The comparators are seperately defined
  bool operator==(const CharString& other)   const;
  bool operator==(const std::string& string) const;
  bool operator==(const char* cstring)       const;

  // Find and substr also require seperate definition
  const char* find(const char* target)                     const;
  const char* find(const CharString& target)               const;
  CharString substr(size_t start, size_t length)           const;
  CharString CarveOut(const char* left, const char* right) const;

  // The basic reading operations are all inlined
  char operator[](int index)               const {return *(begin_ + index);}
  std::string AsString()                   const {return {begin_, length_};}
  const char* begin()                      const {return begin_;}
  const char* end()                        const {return begin_ + length_;}
  char back()                              const {return *(end() - 1);}
  bool empty()                             const {return length_ == 0;}
  size_t size()                            const {return length_;}
  const char* find(const std::string& str) const {return find(str.c_str());}
  bool contains(const char* target)        const {return find(target) != end();}
  bool contains(std::string target)        const {return find(target) != end();}

private:
  const char* begin_;
  size_t length_;
};

// Declaration for output stream interface doesn't need to be a member
std::ostream& operator<<(std::ostream& os, const CharString& charstring);

// Create a string literal CharString directly
inline CharString operator "" _cs(const char* cstr) { return CharString(cstr);}

#endif
