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

struct Buffer
{
  Buffer(const std::string& p_input, int p_start, int p_length) :
    string_(p_input.c_str()),
    start_(p_start),
    length_(p_length),
    size_(p_input.size())
  {
    if(size_ < start_ + length_)
      throw std::runtime_error("Invalid buffer position on creation.");
  };

  Buffer(): string_(nullptr), start_(0), length_(0), size_(0)
  {
    throw std::runtime_error("Invalid buffer creation to nullptr.");
  }

  void Clear(){start_ = start_ + length_; length_ = 0;}

  void operator++()
  {
    if(size_ > start_ + length_)
      ++length_;
    else
      throw std::runtime_error("Buffer exceeds string size");
  }

  void operator++(int)
  {
    ++(*this);
  }

  void StartAt(size_t p_new_start)
  {
    length_ = 1;
    if(size_ > p_new_start) start_ = p_new_start;
    else throw std::runtime_error("Cannot start buffer past end of string");
  }

  std::string AsString() {return std::string(string_ + start_, length_);}

  const char* string_;
  size_t start_;
  size_t length_;
  size_t size_;
};

//---------------------------------------------------------------------------//

class Dictionary
{
  using StringPointer = std::shared_ptr<const std::string>;

 public:
  // Constructors
  Dictionary(StringPointer dictionary_string_ptr);

  Dictionary(StringPointer dictionary_string_ptr, size_t start_position);

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
  std::string operator[](const std::string& key) const {return GetString(key);}

  // Inline definition of dictionary iterators
  typedef std::unordered_map<std::string, std::string>::const_iterator DictIt;
  inline DictIt begin() const { return map_.cbegin(); }
  inline DictIt end()   const { return map_.cend();   }

 private:
  // The single data member
  std::unordered_map<std::string, std::string> map_;
};

//---------------------------------------------------------------------------//

#endif
