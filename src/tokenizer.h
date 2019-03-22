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

/* The tokenizer class represents the last of our dealings with the actual
 * pdf file. After this stage, we have a complete description of the text on
 * the page including the size and position of every correctly-encoded glyph.
 * The subsequent steps will use only this data to try to reconstruct useful
 * semantic information from the text position in an attempt to provide useable
 * data, and to output the result to a variety of formats.
 *
 * The tokenizer class is used to read page description programs from the
 * page contents objects (and form xobjects). Rather than using regex to do
 * this (which is extremely slow), we use a custom-built lexer. This takes the
 * page program as a text string and goes through each character, identifying
 * tokens as it goes and storing them in a buffer until it can be decided what
 * type of token it has read. It switches state according to a finite set of
 * rules so that it knows when to pass the buffer to the parser for
 * parsing.
 *
 * Its interface is very simple - create the object by feeding it a string and
 * a pointer to the graphics state. It will tokenize the string and send it
 * to the parser for parsing
 *
 * It has a number of private members because it is a fairly complex lexer and
 * is easier to maintain as a collection of functions that pass private members
 * around, rather than one huge hairball function.
 */

#include "parser.h"


//---------------------------------------------------------------------------//
// The tokenizer class. It has a simple interface of one constructor and one
// getter for the result. The private members allow for passing of state
// between member functions during the instruction set creation.

class tokenizer
{
public:
  // constructor
  tokenizer(std::string&& s, parser* GS);

private:
  // private data members
  const std::string& s;// the input string itself
  char j;
  std::string::const_iterator i;// The iterator that moves through the string
  std::string buf;     // a string buffer to hold chars until pushed to result
  Token::TState state; // The current state of the finite state machine
  parser* gs;   // The graphic state to which instructions are sent
  static std::string inloop; // keep track of whether we are in an xobject
                             // - this prevents an infinite loop

  // private methods

  void tokenize();                  // chooses state subroutine based on state
  void pushbuf(const Token::TState, const Token::TState);
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
