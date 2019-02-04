//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR tokenizer header file                                               //
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

#ifndef PDFR_TOKEN

//---------------------------------------------------------------------------//

#define PDFR_TOKEN

/* The tokenizer class is used to read page description programs from the
 * page contents objects (and form xobjects). Rather than using regex to do
 * this (which is extremely slow), we use a custom-built lexer. This takes the
 * page program as a text string and goes through each character, identifying
 * tokens as it goes and storing them in a buffer until it can be decided what
 * type of token it has read. It switches state according to a finite set of
 * rules so that it knows when to save the buffer to a vector and how to label
 * the token's type appropriately.
 *
 * When it has read the whole string, it has a complete set of labelled tokens
 * which are ready to be parsed by the parser method of Graphics state.
 *
 * In the sequence of the program, tokenizer is a bit of an outlier. It is not
 * required until late on in the pdf reading process, but it does not need any
 * knowledge of the structure or methods of pdfs. It #includes the utilities.h
 * header because it uses the standard library and some utility functions.
 *
 * Its interface is very simple - create the object by feeding it a string,
 * and it will return an instruction set using the result() method.
 *
 * It has a number of private members because it is a fairly complex lexer and
 * is easier to maintain as a collection of functions that pass private members
 * around, rather than one huge hairball function.
 */

#include "page.h"

//---------------------------------------------------------------------------//
// The states of the lexer are defined by this enum. It is defined in its own
// namespace rather than within the class because its states are also used
// as type labels in the instruction set it produces. These therefore need
// to be made available to the instruction reader in the graphic_state class

namespace Token
{
  enum TState
  {
    NEWSYMBOL,
    IDENTIFIER,
    NUMBER,
    RESOURCE,
    STRING,
    HEXSTRING,
    ARRAY,
    DICT,
    WAIT,
    OPERATOR
  };
};

//---------------------------------------------------------------------------//
// The tokenizer class. It has a simple interface of one constructor and one
// getter for the result. The private members allow for passing of state
// between member functions during the instruction set creation.

class tokenizer
{
public:

  // constructor

  tokenizer(const std::string& s);

  // get results from tokenizer

  std::vector<std::pair<std::string, Token::TState>> result();

private:

  // private data members

  size_t i;            // The iterator that moves through the string
  const std::string& s;// the input string itself
  std::string buf;     // a string buffer to hold chars until pushed to result
  Token::TState state; // The current state of the finite state machine

  // The main output of the lexer
  std::vector<std::pair<std::string, Token::TState>> output;


  // private methods

  void tokenize();                  // chooses state subroutine based on state
  void subtokenizer(const std::string&);  // uses recursion for sub-strings

  void pushbuf(const Token::TState, const Token::TState); // Avoids boilerplate by carrying
                                              // out the common lexer task of
                                              // pushing and clearing the
                                              // buffer then switching state

  void newsymbolState();    //--------//---------------------------------------
  void resourceState();               //
  void identifierState();             //
  void numberState();                 // These private member functions handle
  void stringState();                 // the various states of the lexer,
  void arrayState();                  // responding variously to each character
  void escapeState();                 // they come across to build the result
  void hexstringState();              //
  void dictState();                   //
  void waitState();         //--------//---------------------------------------

};

//---------------------------------------------------------------------------//

#endif

