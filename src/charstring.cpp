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

const char* CharString::find(const char* p_target) const
{
  int first_char = -1;
  size_t target_index = 0;

  for (auto it = this->begin(); it != this->end(); ++it)
  {
      if (*it == *(p_target + target_index))
      {
        if (first_char == -1) first_char = it - this->begin();
        ++target_index;
      }
      else
      {
        if (*(p_target) == *it)
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

      if (*(p_target + target_index) == '\0')
        return this->begin() + first_char;

  }
  return this->end();
}

/*--------------------------------------------------------------------------*/

std::ostream& operator<<(std::ostream& p_os, const CharString& p_cs)
{
  for(auto it = p_cs.begin(); it != p_cs.end(); ++it)
  {
    p_os << *it;
  }
  return p_os;
}

/*--------------------------------------------------------------------------*/

bool CharString::operator==(const char* p_cstring) const
{
    if (length_ == 0) return false;
    for (size_t i = 0; i < length_; ++i)
    {
      if (*(begin_ + i) != *(p_cstring + i)) return false;
      if (*(p_cstring + i) == '\0') return false;
      if (length_ - i == 1 && *(p_cstring + i + 1) != '\0') return false;
    }
    return true;
}

/*--------------------------------------------------------------------------*/

bool CharString::operator==(const CharString& p_other) const
{
  if (length_ != p_other.length_) return false;
  if (begin_ == p_other.begin_) return true;
  for (size_t i = 0; i < length_; ++i)
  {
    if (*(begin_ + i) != p_other[i]) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

bool CharString::operator==(const std::string& p_string) const
{
  if (length_ != p_string.size()) return false;
  for (size_t i = 0; i < length_; ++i)
  {
    if (*(begin_ + i) != p_string[i]) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

CharString CharString::substr(size_t p_start, size_t p_length) const
{
  if (p_start >= this->size())
  {
    throw std::runtime_error("Invalid substring range in CharString::substr");
  }

  if (p_start + p_length > this->size())
  {
    p_length = this->size() - p_start;
  }

  return CharString(this->begin(), p_start, p_start + p_length);
}
