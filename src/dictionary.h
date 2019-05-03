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
 * will stop reading automatically when it comes to the end if the dictionary.
 *
 * This string is passed through a lexer. This is implemented by a helper class
 * in the implementation file that does not require a public interface. The
 * lexer parses the name:value pairs into a std::unordered_map. The values are
 * all stored as strings and processed as required. Mostly this processing is
 * done by the class itself from public member functions which can return
 * numbers, references, strings and dictionaries on request. The interface is
 * therefore large but read-only.
 */

#include "utilities.h"

//---------------------------------------------------------------------------//

class Dictionary
{
  std::unordered_map<std::string, std::string> m_Map; // data holder

  public:
  // Constructors
  Dictionary(std::shared_ptr<const std::string>); // make dictionary from string
  Dictionary(std::shared_ptr<const std::string>, size_t); // dict from pos + str
  Dictionary(std::unordered_map<std::string, std::string>); // create from map
  Dictionary(const Dictionary& d): m_Map(d.m_Map){};
  Dictionary(Dictionary&& d) noexcept {std::swap(this->m_Map, d.m_Map);};
  Dictionary& operator=(Dictionary&& d) noexcept {
    std::swap(m_Map, d.m_Map); return *this;}
  Dictionary(); // empty dictionary
  Dictionary& operator=(const Dictionary& d){m_Map = d.m_Map; return *this;}

  // Public member functions
  std::string get_string(const std::string&) const;  // get value as string given name
  bool has_key(const std::string&) const;         // confirms a key is present
  bool contains_references(const std::string&) const;   // tests if given key has references
  bool contains_ints(const std::string&) const;     // tests if given key has ints
  bool contains_dictionary(const std::string&) const; // tests if key has dictionary
  std::vector<int> get_references(const std::string&) const;
  int get_reference(const std::string& Key) const;
  std::vector<int> get_ints(const std::string&) const;
  std::vector<float> get_floats(const std::string&) const;
  std::vector<std::string> get_all_keys() const;
  Dictionary get_dictionary(const std::string&) const;
  const std::unordered_map<std::string, std::string>& R_out() const;

  std::unordered_map<std::string, std::string>::const_iterator begin()
  {
    return m_Map.cbegin();
  }

  std::unordered_map<std::string, std::string>::const_iterator end()
  {
    return m_Map.cend();
  }
};

//---------------------------------------------------------------------------//

#endif
