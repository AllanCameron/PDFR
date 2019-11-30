//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Tokenizer implementation file                                       //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "tokenizer.h"
#include<iostream>

using namespace std;
using namespace Token;

std::string Tokenizer::in_loop_ = "none";


/*---------------------------------------------------------------------------*/
// constructor of Tokenizer - initializes members and starts tokenizing

Tokenizer::Tokenizer(shared_ptr<string> p_input, Parser* p_interpreter)
  : it_(p_input),
    state_(NEWSYMBOL),
    interpreter_(p_interpreter)
{
  // Now cycle through each character, switching state as needed and writing
  // to the interpreter when a parsed symbol has been obtained.
  while (!it_.HasOverflowed())
  {
    switch (state_)
    {
      // Each state has its own handler subroutine - self explanatory
      case NEWSYMBOL:   NewSymbolState_();         break;
      case RESOURCE:    ResourceState_();          break;
      case IDENTIFIER:  IdentifierState_();        break;
      case NUMBER:      NumberState_();            break;
      case ARRAY:       ArrayState_();             break;
      case STRING:      StringState_();            break;
      case HEXSTRING:   HexStringState_();         break;
      case DICT:        DictionaryState_();        break;
      case WAIT:        WaitState_();              break;
      case OPERATOR:                               break;
    }
    ++it_;
  }
}

/*---------------------------------------------------------------------------*/
// There may be an "external" xobject in the page description program.
// (i.e in a different pdf object). This needs to be fetched and parsed by the
// same Parser instance we are using, but we call up a new Tokenizer to read
// the symbols into the Parser. In theory, these XObjects can be nested so we
// need to keep track of which XObject we're in using the in_loop_ member.

void Tokenizer::HandleXObject_()
{
  string loop_name = interpreter_->GetOperand();
  if (loop_name != in_loop_)
  {
    in_loop_ = loop_name;
    shared_ptr<string> xobject = interpreter_->GetXObject(in_loop_);

    // Don't try to parse binary objects like images etc
    if (IsAscii(*xobject)) Tokenizer(xobject, interpreter_);
  }
}

/*---------------------------------------------------------------------------*/
// This pattern, of switching state, creating a token / type pair, pushing it
// to the instruction set, and clearing the buffer is very common in the
// lexer. This function acts as a shorthand to prevent boilerplate

void Tokenizer::PushBuffer_(const TokenState p_type, const TokenState p_state)
{
  if (it_ == "Do" && p_state == IDENTIFIER) HandleXObject_();
  interpreter_->Reader(it_.Contents(), p_type); // make pair and push to result
  NewToken_(p_state);
}


/*---------------------------------------------------------------------------*/
// Lexer is reading a resource (a /PdfName)

void Tokenizer::ResourceState_()
{
  switch (GetCharType())
  {
    case LAB:   PushBuffer_(RESOURCE, HEXSTRING); HandleLAB_(); break;
    case LET:                                                   break;
    case DIG:                                                   break;
    case USC:                                                   break;
    case LSB:   PushBuffer_(RESOURCE, ARRAY);                   break;
    case FSL:   PushBuffer_(RESOURCE, RESOURCE);                break;
    case AST:                                                   break;
    case LCB:   PushBuffer_(RESOURCE, STRING);    Skip_();      break;
    case SUB:                                                   break;
    case BSL:   throw runtime_error("illegal character");       break;
    case SPC:   PushBuffer_(RESOURCE, NEWSYMBOL);               break;
    case RAB:   throw runtime_error("illegal character");       break;
    case PER:   throw runtime_error("illegal character");       break;
    case ADD:                                                   break;
    default:    throw runtime_error("illegal character");       break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void Tokenizer::NewSymbolState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case LAB: HandleLAB_();                       break;
    case LET: NewToken_(IDENTIFIER);              break;
    case DIG: NewToken_(NUMBER);                  break;
    case USC: NewToken_(IDENTIFIER);              break;
    case LSB: NewToken_(NEWSYMBOL);               break;
    case RSB: NewToken_(NEWSYMBOL);               break;
    case FSL: NewToken_(RESOURCE);                break;
    case AST: NewToken_(IDENTIFIER);              break;
    case LCB: HandleLCB_();                       break;
    case SUB: NewToken_(NUMBER);                  break;
    case PER: NewToken_(NUMBER);                  break;
    case SQO: NewToken_(IDENTIFIER);              break;
    case APO: interpreter_->Reader("'", IDENTIFIER); it_.Clear(); break;
    default : NewToken_(NEWSYMBOL);               break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void Tokenizer::IdentifierState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case LAB: PushBuffer_(IDENTIFIER, HEXSTRING); HandleLAB_(); break;
    case LET:                                                   break;
    case DIG:                                                   break;
    case SPC: if (it_ == "BI") state_ = WAIT;
              else PushBuffer_(IDENTIFIER, NEWSYMBOL);          break;
    case FSL: PushBuffer_(IDENTIFIER, RESOURCE); it_.Clear();   break;
    case LSB: PushBuffer_(IDENTIFIER, NEWSYMBOL);               break;
    case LCB: PushBuffer_(IDENTIFIER, STRING); Skip_();         break;
    case SUB:                                                   break;
    case USC:                                                   break;
    case AST:                                                   break;
    default : it_.Clear();                                      break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void Tokenizer::NumberState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case LAB:   PushBuffer_(NUMBER, HEXSTRING); HandleLAB_(); break;
    case DIG:                                                 break;
    case SPC:   PushBuffer_(NUMBER, NEWSYMBOL);               break;
    case PER:                                                 break;
    case LCB:   PushBuffer_(NUMBER, STRING); HandleLCB_();    break;
    case LET:                                                 break;
    case USC:                                                 break;
    case SUB:   PushBuffer_(NUMBER, NUMBER);                  break;
    case AST:   PushBuffer_(NUMBER, NUMBER);                  break;
    case FSL:   PushBuffer_(NUMBER, NUMBER);                  break;
    case LSB:   PushBuffer_(NUMBER, ARRAY);                   break;
    default:    PushBuffer_(NUMBER, NEWSYMBOL);               break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void Tokenizer::StringState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case RCB:   PushBuffer_(STRING, NEWSYMBOL);             break;
    case BSL:   EscapeState_();                             break;
    default:                                                break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is in an array

void Tokenizer::ArrayState_() {NewToken_(NEWSYMBOL);}

/*---------------------------------------------------------------------------*/
// lexer is reading a hexstring of format <11FA>

void Tokenizer::HexStringState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case RAB:  if (!empty()) PushBuffer_(HEXSTRING, NEWSYMBOL);
               state_ = NEWSYMBOL;                    break;
    case LAB:  NewToken_(DICT);                       break;
    default:                                          break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void Tokenizer::DictionaryState_()
{
  // get symbol_type of current char
  switch (GetCharType())
  {
    case BSL:   ++it_;                        break;
    case RAB:  PushBuffer_(DICT, HEXSTRING);  break;
    default:                                  break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void Tokenizer::EscapeState_()
{
  // If we're in the middle of a string, we'll need to send what we have so far
  // to the Parser.
  if (!empty()) PushBuffer_(STRING, STRING);

  // We know we're in an escaped state, so we skip the actual backslash
  Skip_();
  // Read the next char - if it's a digit it's probably an octal
  if (GetCharType() == DIG)
  {
    int octcount = 0;

    // Add consecutive chars to octal (up to 3)
    while (GetCharType() == DIG && octcount < 3) { ++it_; ++octcount; }

    // Convert octal string to int
    int newint = stoi(it_.Contents(), nullptr, 8);
    interpreter_->Reader(ConvertIntToHex(newint), HEXSTRING);

    // Get ready fpr the rest of the string
    NewToken_(STRING);

    // However, if there is no more left of the string, we need to switch out
    if(GetCharType() == RCB) state_ = NEWSYMBOL;
  }
  else
  {
    // We have an escape character which needs to be interpreted the long way
    string escaped;
    switch(GetChar())
    {
      case ')'  : escaped = ")";    break;
      case '\\' : escaped = "\\";   break;
      case 'n'  : escaped = "\n";   break;
      case 'r'  : escaped = "\r";   break;
      case 't'  : escaped = "\t";   break;
      default   : escaped = GetChar();
    }
    interpreter_->Reader(escaped, STRING);
    state_ = STRING;
    it_.Clear();
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void Tokenizer::WaitState_()
{
  // Moves through the string until it finds an 'E'
  while (GetChar() != 'E') Skip_();

  ++it_;

  // If the following character is not an 'I', keeps looking for an 'E'
  if(GetChar() != 'I') WaitState_();

  // After an 'EI' is found we are out of the inline image and ready for the
  // next token
  NewToken_(NEWSYMBOL);
}
