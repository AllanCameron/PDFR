//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR CharString implementation file                                      //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "charstring.h"
#include<stdexcept>

/*--------------------------------------------------------------------------*/
// Returns a pointer to the beginning of the first instance of a target
// character literal in a CharString

const char* CharString::find(const char* target) const
{
  int first_char = -1;
  size_t target_index = 0;

  for (auto it = this->begin(); it != this->end(); ++it)
  {
    if (*it == *(target + target_index))
    {
      if (first_char == -1) first_char = it - this->begin();
      ++target_index;
    }
    else
    {
      if (*(target) == *it)
      {
        first_char = it - this->begin();
        target_index = 1;
      }
      else
      {
        first_char = -1;
        target_index = 0;
      }
    }
    if (*(target + target_index) == '\0') return this->begin() + first_char;
  }
  return this->end();
}

const char* CharString::find(const CharString& target) const
{
  int first_char = -1;
  size_t target_index = 0;

  for (auto it = this->begin(); it != this->end(); ++it)
  {
    if (*it == target[target_index])
    {
      if (first_char == -1) first_char = it - this->begin();
      ++target_index;
    }
    else
    {
      if (*(target.begin()) == *it)
      {
        first_char = it - this->begin();
        target_index = 1;
      }
      else
      {
        first_char = -1;
        target_index = 0;
      }
    }
    if (target[target_index] == '\0') return this->begin() + first_char;
  }
  return this->end();
}

/*--------------------------------------------------------------------------*/

std::ostream& operator<<(std::ostream& os, const CharString& cs)
{
  for(auto it = cs.begin(); it != cs.end(); ++it) os << *it;
  return os;
}

/*--------------------------------------------------------------------------*/
// A CharString matches a C-string only if all characters in the two strings
// match, not including the C-string's terminal nul character.

bool CharString::operator==(const char* cstring) const
{
    if (length_ == 0) return false;
    for (size_t i = 0; i < length_; ++i)
    {
      if (*(begin_ + i) != *(cstring + i)) return false;
      if (*(cstring + i) == '\0') return false;
      if (length_ - i == 1 && *(cstring + i + 1) != '\0') return false;
    }
    return true;
}

/*--------------------------------------------------------------------------*/

bool CharString::operator==(const CharString& other) const
{
  if (length_ != other.length_) return false;
  if (begin_ == other.begin_) return true;
  for (size_t i = 0; i < length_; ++i)
  {
    if (*(begin_ + i) != other[i]) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

bool CharString::operator==(const std::string& stdstring) const
{
  if (length_ != stdstring.size()) return false;
  for (size_t i = 0; i < length_; ++i)
  {
    if (*(begin_ + i) != stdstring[i]) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

CharString CharString::substr(size_t start, size_t length) const
{
  if (start >= this->size())
  {
    throw std::runtime_error("Invalid substring range in CharString::substr");
  }

  if (start + length > this->size())
  {
    length = this->size() - start;
  }

  return CharString(this->begin(), start, start + length);
}

CharString CharString::CarveOut(const char* left, const char* right) const
{
  size_t leftsize = 0;
  while(*(leftsize + left)) ++leftsize;
  const char* newstart = find(left);
  if (newstart == end()) return *this; else newstart += leftsize;
  CharString leftchunk(newstart, end() - newstart);
  const char* newend = leftchunk.find(right);
  return CharString(newstart, newend - newstart);
}
