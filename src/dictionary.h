//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Dictionary header file                                              //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_DICT

//---------------------------------------------------------------------------//

#define PDFR_DICT

/* This header is the second in a "daisy chain" of headers which build up the
 * tools needed to read pdfs. It comes straight after utilities.h, and is
 * required by most of the other source files here. (see headerMap.txt)
 *
 * The dictionary is an important part of a pdf's data structure. It consists
 * of a variable number of name-value pairs. The names are designated by a
 * preceding forward slash, eg /PDFName. The values in the name:value pair can
 * be of four different basic types: boolean, number, object reference and
 * string. It can also be one of two composite types: an array (enclosed in
 * square brackets), or another dictionary. Dictionaries can thus be arbitrarily
 * nested.
 *
 * A dictionary is enclosed in <<double angle brackets>>. Most pdf objects
 * start with a dictionary, and many are only dictionaries. It is therefore
 * necessary to define a dictionary class early on as it is a prerequisite
 * of navigating and interpreting a pdf.
 *
 * This class is created by providing a pointer to a std::string containing a
 * pdf dictionary. It is overloaded to allow a starting position to be
 * specified, though no stopping point is needed as the dictionary lexer
 * will stop reading automatically when it comes to the end of the dictionary.
 *
 * This string is passed through a lexer. This is implemented by a helper class
 * in the implementation file that does not require a public interface. The
 * lexer parses the name:value pairs into a std::unordered_map. The values are
 * all stored as strings and processed as required. Mostly this processing is
 * done by the class itself from public member functions which can return
 * numbers, references, strings and dictionaries on request. The interface is
 * therefore large but read-only. There is no ability to change a dictionary
 * after its creation.
 */

#include "utilities.h"
#include <memory>

//---------------------------------------------------------------------------//

class Dictionary
{
  using StringPointer = std::shared_ptr<const std::string>;

 public:
  // Constructors
  Dictionary(StringPointer dictionary_string_ptr);

  Dictionary(StringPointer dictionary_string_ptr, size_t start_position);

  Dictionary(std::unordered_map<std::string, std::string> dictionary_map);

  Dictionary(const Dictionary& t_other): m_map(t_other.m_map){};

  Dictionary(Dictionary&& t_other) noexcept { std::swap(m_map, t_other.m_map); }

  Dictionary() = default;

  Dictionary& operator=(Dictionary&& t_other) noexcept
  {
    std::swap(m_map, t_other.m_map);
    return *this;
  }

  Dictionary& operator=(const Dictionary& t_other)
  {
    m_map = t_other.m_map;
    return *this;
  }

  // Public member functions
  std::string GetString_(const std::string& key)              const;
  bool HasKey_(const std::string& key)                        const;
  bool ContainsReferences_(const std::string& key)            const;
  bool ContainsInts_(const std::string& key)                  const;
  bool ContainsDictionary_(const std::string& key)            const;
  std::vector<int> GetReferences_(const std::string& key)     const;
  int GetReference_(const std::string& key)                   const;
  std::vector<int> GetInts_(const std::string& key)           const;
  std::vector<float> GetFloats_(const std::string& key)       const;
  std::vector<std::string> GetAllKeys_()                      const;
  Dictionary GetDictionary_(const std::string& key)           const;
  std::unordered_map<std::string, std::string> GetMap_()      const;

  // Inline definition of dictionary iterators
  // This is needed for some range based loops.
  typedef std::unordered_map<std::string, std::string>::const_iterator DictIt;
  inline DictIt begin() const { return m_map.cbegin(); }
  inline DictIt end()   const { return m_map.cend();   }

 private:
  // The single data member
  std::unordered_map<std::string, std::string> m_map;
};

//---------------------------------------------------------------------------//

#endif
