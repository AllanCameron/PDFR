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
 * specified, though not stopping point is needed as the dictionary lexer
 * will stop reading automatically when it comes to the end if the dictionary.
 *
 * This string is passed through the lexer which parses the name:value pairs
 * into a std::unordered_map. The values are all stored as strings and
 * processed as required. Mostly this processing is done by the class itself
 * from public member functions which can return numbers, references, strings
 * and dictionaries on request. The interface is therefore large but read-only.
 */

#include "utilities.h"
#include<iostream>

//---------------------------------------------------------------------------//

class dictionary
{
  public:
  // Constructors
  dictionary(std::shared_ptr<const std::string>); // make dictionary from string
  dictionary(std::shared_ptr<const std::string>, size_t); // dict from pos + str
  dictionary(std::unordered_map<std::string, std::string>); // create from map
  dictionary(const dictionary& d): DictionaryMap(d.DictionaryMap){};
  dictionary(dictionary&& d): DictionaryMap(std::move(d.DictionaryMap)){};
  dictionary(); // empty dictionary
  dictionary& operator=(const dictionary& d){
    DictionaryMap = d.DictionaryMap;
    return *this;
    }

  // Public member functions
  std::string get(const std::string&) const;  // get value as string given name
  bool has(const std::string&) const;         // confirms a key is present
  bool hasRefs(const std::string&) const;   // tests if given key has references
  bool hasInts(const std::string&) const;     // tests if given key has ints
  bool hasDictionary(const std::string&) const; // tests if key has dictionary
  std::vector<int> getRefs(const std::string&) const; // gets obj refs from key
  std::vector<int> getInts(const std::string&) const; // gets ints from key
  std::vector<float> getNums(const std::string&) const; // gets floats from key
  std::vector<std::string> getDictKeys() const; // gets all keys from dictionary
  dictionary getDictionary(const std::string&) const; // gets sub-dict from key
  std::unordered_map<std::string, std::string> R_out() const; // gets full map

private:
// The lexer which constructs the dictionary is a finite state machine, which
// behaves in different ways to parse the string depending on its state.
// The state in turn may be changed by the character read by the lexer.
// The following enum lists the possible states of the finite state machine.
enum DState     {PREENTRY,
                 QUERYCLOSE,
                 VALUE,
                 MAYBE,
                 START,
                 KEY,
                 PREVALUE,
                 DSTRING,
                 ARRAYVAL,
                 QUERYDICT,
                 SUBDICT,
                 CLOSE,
                 THE_END};

  // Private data members

  std::shared_ptr<const std::string> s;   // pointer to the string being read
  size_t i;         // the string's iterator which is passed between functions
  int bracket;      // integer to store the nesting level of angle brackets
  bool keyPending;  // flag that indicates a key name has been read
  std::string buf;  // string to hold the read characters in memory until needed
  std::string pendingKey; // name of key waiting for a value
  DState state;     // current state of fsm
  std::unordered_map<std::string, std::string> DictionaryMap; // data holder

  // Private functions
  void tokenize_dict(); // co-ordinates the lexer
  void setkey(std::string, DState); //----//
  void assignValue(std::string, DState);  //
  void handleMaybe(char);                 //
  void handleStart(char);                 //
  void handleKey(char);                   //
  void handlePrevalue(char);              //--> functions to handle lexer states
  void handleValue(char);                 //
  void handleArrayval(char);              //
  void handleDstring(char);               //
  void handleQuerydict(char);             //
  void handleSubdict(char);               //
  void handleClose(char);           //----//
};

//---------------------------------------------------------------------------//

#endif
