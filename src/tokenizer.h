//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR tokenizer header file                                               //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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
// The Tokenizer class. It has a simple interface of one constructor and one
// getter for the result. The private members allow for passing of state
// between member functions during the instruction set creation.

class Tokenizer
{
 public:
  // Constructor. Takes a string pointer to the page description program
  // and a fresh Parser object
  Tokenizer(std::shared_ptr<std::string> p_input_string, Parser* p_parser);

 private:
  // Enumerates the types of characters that can alter state differently
  enum CharType
  {
    LAB, LET, DIG, USC, LSB, FSL, AST, LCB, SUB,
    BSL, SPC, RAB, PER, ADD, QOT, RCB, RSB, SQO, OTH
  };

  std::shared_ptr<std::string> contents_; // Pointer to input string
  std::string::const_iterator it_;        // Iterates through contents_
  std::string buffer_;                    // Tokenizer's string buffer
  Token::TokenState state_;               // Current Tokenizer state
  Parser* interpreter_;                   // The Parser instructions are sent to
  static std::string in_loop_;            // Prevents an infinite loop
  static std::array<CharType, 256> char_lookup_; // Lookup table for char type

  // private methods
  void Tokenize_();                    // chooses subroutine based on state
  void NewSymbolState_();    //--------//---------------------------------------
  void ResourceState_();               //
  void IdentifierState_();             //
  void NumberState_();                 // These private member functions handle
  void StringState_();                 // the various states of the lexer,
  void ArrayState_();                  // responding variously to each character
  void EscapeState_();                 // they come across to build the result
  void HexStringState_();              //
  void DictionaryState_();             //
  void WaitState_();         //--------//---------------------------------------

  // Frequently used helper function to update buffer and state
  void PushBuffer_(const Token::TokenState, const Token::TokenState);
};

//---------------------------------------------------------------------------//

#endif
