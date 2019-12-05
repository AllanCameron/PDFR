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
  CharString(const char* p_ptr, size_t p_start, size_t p_end) :
  begin_(p_ptr + p_start), length_(p_end - p_start) {}

  // Or just a pointer and a length
  CharString(const char* p_ptr, size_t p_length) :
  begin_(p_ptr), length_(p_length) {}

  // Or just a pointer to a zero-terminated string
  CharString(const char* p_ptr) :
  begin_(p_ptr), length_(0) { while (*(begin_ + length_)) ++length_; }

  // Or an std::string
  CharString(const std::string& p_string) :
  begin_(p_string.c_str()), length_(p_string.size()) {}

  // Or an std::string with a starting offset
  CharString(const std::string& p_string, size_t p_start) :
  begin_(p_string.c_str() + p_start), length_(p_string.size() - p_start) {}

  // Or finally another CharString
  CharString(const CharString&) = default;
  CharString& operator=(const CharString& p_chunk) = default;

  // And an empty CharString
  CharString() : begin_(nullptr), length_(0) {}

  // The comparators are seperately defined
  bool operator==(const CharString& p_other)         const;
  bool operator==(const std::string& p_string)       const;
  bool operator==(const char* p_cstring)             const;

  // Find and substr also require seperate definition
  const char* find(const char* p_target)             const;
  CharString substr(size_t p_start, size_t p_length) const;
  CharString CarveOut(const char* p_left, const char* p_right) const;

  // The basic reading operations are all inlined
  char operator[](int p_index) const {return *(begin_ + p_index);}

  std::string AsString() const {return std::string(begin_, length_);};

  const char* begin() const {return begin_;}

  const char* end() const {return begin_ + length_;}

  char back() const { return *(end() - 1);}

  bool empty() const {return length_ == 0;}

  size_t size() const {return length_;}

  const char* find(const std::string& p_str) const {return find(p_str.c_str());}

  bool contains(const char* p_target) const { return find(p_target) != end();}

  bool contains(std::string p_target) const {return find(p_target) != end();}

private:
  const char* begin_;
  size_t length_;
};

// Declaration for output stream interface doesn't need to be a member
std::ostream& operator<<(std::ostream& p_os, const CharString& p_cs);

#endif
