//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR dictionary header file                                              //
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

//---------------------------------------------------------------------------//

class Dictionary
{
  std::unordered_map<std::string, std::string> map_;

  public:
  // Constructors
  Dictionary(std::shared_ptr<const std::string>);

  Dictionary(std::shared_ptr<const std::string>, size_t);

  Dictionary(std::unordered_map<std::string, std::string>);

  Dictionary(const Dictionary& d): map_(d.map_){};

  Dictionary(Dictionary&& t_other) noexcept
  {
    std::swap(this->map_, t_other.map_);
  }

  Dictionary& operator=(Dictionary&& t_other) noexcept
  {
    std::swap(map_, t_other.map_);
    return *this;
  }

  Dictionary();

  Dictionary& operator=(const Dictionary& t_other)
  {
    map_ = t_other.map_;
    return *this;
  }

  // Public member functions
  std::string GetString(const std::string&)              const;
  bool HasKey(const std::string&)                        const;
  bool ContainsReferences(const std::string&)            const;
  bool ContainsInts(const std::string&)                  const;
  bool ContainsDictionary(const std::string&)            const;
  std::vector<int> GetReferences(const std::string&)     const;
  int GetReference(const std::string&)                   const;
  std::vector<int> GetInts(const std::string&)           const;
  std::vector<float> GetFloats(const std::string&)       const;
  std::vector<std::string> GetAllKeys()                  const;
  Dictionary GetDictionary(const std::string&)           const;
  std::unordered_map<std::string, std::string> GetMap()  const;

  // Inline definition of dictionary iterators
  typedef std::unordered_map<std::string, std::string>::const_iterator DictIt;
  inline DictIt begin() const { return map_.cbegin(); }
  inline DictIt end()   const { return map_.cend();   }
};

//---------------------------------------------------------------------------//

#endif
