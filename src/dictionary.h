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

#include<vector>
#include<string>
#include<memory>
#include<unordered_map>


//---------------------------------------------------------------------------//

class Dictionary
{
  using StringPointer = std::shared_ptr<const std::string>;

 public:
  // Constructors
  Dictionary(StringPointer dictionary_string_ptr);

  Dictionary(StringPointer dictionary_string_ptr, size_t start_position);

  Dictionary(const CharString&);

  Dictionary(std::unordered_map<std::string, std::string> p_map): map_(p_map){};

  Dictionary(const Dictionary& p_other): map_(p_other.map_){};

  Dictionary(Dictionary&& p_other) noexcept { std::swap(map_, p_other.map_); }

  Dictionary() = default;

  Dictionary& operator=(Dictionary&& p_other) noexcept
  {
    std::swap(map_, p_other.map_);
    return *this;
  }

  Dictionary& operator=(const Dictionary& p_other)
  {
    map_ = p_other.map_;
    return *this;
  }

  // Public member functions
  std::string operator[](const std::string& key)             const;
  std::string GetString(const std::string& key)              const;
  bool HasKey(const std::string& key)                        const;
  bool ContainsReferences(const std::string& key)            const;
  bool ContainsInts(const std::string& key)                  const;
  bool ContainsDictionary(const std::string& key)            const;
  std::vector<int> GetReferences(const std::string& key)     const;
  int GetReference(const std::string& key)                   const;
  std::vector<int> GetInts(const std::string& key)           const;
  std::vector<float> GetFloats(const std::string& key)       const;
  std::vector<std::string> GetAllKeys()                      const;
  Dictionary GetDictionary(const std::string& key)           const;
  std::unordered_map<std::string, std::string> GetMap()      const;
  void PrettyPrint ()                                        const;

  // Inline definition of dictionary iterators
  typedef std::unordered_map<std::string, std::string>::const_iterator DictIt;
  inline DictIt begin() const { return map_.cbegin(); }
  inline DictIt end()   const { return map_.cend();   }

 private:
  // The single data member
  std::unordered_map<std::string, std::string> map_;
};

/*---------------------------------------------------------------------------*/
// Simple getter of dictionary contents as a string from given key name

inline std::string Dictionary::GetString(const std::string& p_key) const
{
  // A simple map index lookup with square brackets adds the key to
  // map_, which we don't want. Using find(key) leaves it unaltered
  auto finder = map_.find(p_key);
  if (finder != map_.end()) return finder->second;

  // We want an empty string rather than an error if the key isn't found.
  // This allows functions that try to return references, ints, floats etc
  // to return an empty vector so a boolean test of their presence is
  // possible without calling the lexer twice.
  return std::string();
}

inline std::string Dictionary::operator[](const std::string& p_key) const
{
  return GetString(p_key);
}

/*---------------------------------------------------------------------------*/
// Returns any integers present in the value string as read by the ParseInts()
// global function defined in utilities.cpp

inline std::vector<int> Dictionary::GetInts(const std::string& p_key) const
{
  return ParseInts(this->GetString(p_key));
}

/*---------------------------------------------------------------------------*/
// Returns any floats present in the value string as read by the ParseFloats()
// global function defined in utilities.cpp

inline std::vector<float> Dictionary::GetFloats(const std::string& p_key) const
{
  return ParseFloats(this->GetString(p_key));
}

/*---------------------------------------------------------------------------*/
// Checks whether a subdictionary is present in the value string by looking
// for double angle brackets

inline bool Dictionary::ContainsDictionary(const std::string& p_key) const
{
  std::string dictionary = this->GetString(p_key);
  return dictionary.find("<<") != std::string::npos;
}

/*---------------------------------------------------------------------------*/
// Returns all the keys present in the dictionary using the GetKeys() template
// defined in utilities.h

inline std::vector<std::string> Dictionary::GetAllKeys() const
{
  return GetKeys(this->map_);
}

/*---------------------------------------------------------------------------*/
// Returns the entire map. This is useful for passing dictionaries out of
// the program, for example in debugging

inline std::unordered_map<std::string, std::string> Dictionary::GetMap() const
{
  return this->map_;
}

/*---------------------------------------------------------------------------*/
// Sometimes we just need a boolean check for the presence of a key

inline bool Dictionary::HasKey(const std::string& p_key) const
{
  return map_.find(p_key) != map_.end();
}

/*---------------------------------------------------------------------------*/
// We need to be able to check whether a key's value contains references.
// This should return true if the key is present AND its value contains
// at least one object reference, and should be false in all other cases

inline bool Dictionary::ContainsReferences(const std::string& p_key) const
{
  return !this->GetReferences(p_key).empty();
}

/*---------------------------------------------------------------------------*/
// Checks whether the key's values contains any integers. If a key is present
// AND its value contains ints, this returns true. Otherwise false.

inline bool Dictionary::ContainsInts(const std::string& p_key) const
{
  return !this->GetInts(p_key).empty();
}

/*---------------------------------------------------------------------------*/
// Returns a vector of object numbers from any object references found in the
// given key's value. Uses a global function from utilities.h

inline std::vector<int> Dictionary::GetReferences(const std::string& p_key)const
{
  return ParseReferences(this->GetString(p_key));
}

#endif
