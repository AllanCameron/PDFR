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

const std::array<CharType, 256> TokenizerBuffer::char_lookup_ =
{
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, SPC, OTH, OTH, SPC, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  SPC, OTH, QOT, OTH, OTH, OTH, SQO, APO,
  LCB, RCB, AST, ADD, OTH, SUB, PER, FSL,
  DIG, DIG, DIG, DIG, DIG, DIG, DIG, DIG,
  DIG, DIG, OTH, OTH, LAB, OTH, RAB, OTH,
  OTH, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LSB, BSL, RSB, OTH, USC,
  OTH, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, LET, LET, LET, LET, LET,
  LET, LET, LET, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH
};

/*---------------------------------------------------------------------------*/
// constructor of Tokenizer - initializes members and starts main
// lexer function

Tokenizer::Tokenizer(shared_ptr<string> p_input, Parser* p_interpreter)
  : it_(p_input),
    state_(NEWSYMBOL),
    interpreter_(p_interpreter)
{
  Tokenize_();
}


/*---------------------------------------------------------------------------*/
// This pattern, of switching state, creating a token / type pair, pushing it
// to the instruction set, and clearing the buffer is very common in the
// lexer. This function acts as a shorthand to prevent boilerplate

void Tokenizer::PushBuffer_(const TokenState p_type, const TokenState p_state)
{
  if (it_ == "Do" && p_state == IDENTIFIER)
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
  auto content = it_.Contents();
  state_ = p_state; // switch state
  interpreter_->Reader(content, p_type); // make pair and push to result
  it_.Clear(); // clear buffer
}

/*---------------------------------------------------------------------------*/
// This function co-ordinates the lexer by calling a subroutine depending on
// the state. Each subroutine handles specific characters in a different but
// well-specified way

void Tokenizer::Tokenize_()
{
  // Ensures the iterator doesn't exceed string length
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
// Lexer is reading a resource (a /PdfName)

void Tokenizer::ResourceState_()
{
  switch (it_.GetCharType())
  {
    case LAB:   PushBuffer_(RESOURCE, HEXSTRING); ++it_; it_.Clear(); break;
    case LET:                                                         break;
    case DIG:                                                         break;
    case USC:                                                         break;
    case LSB:   PushBuffer_(RESOURCE, ARRAY);                         break;
    case FSL:   PushBuffer_(RESOURCE, RESOURCE);                      break;
    case AST:                                                         break;
    case LCB:   PushBuffer_(RESOURCE, STRING);    ++it_; it_.Clear(); break;
    case SUB:                                                         break;
    case BSL:   throw runtime_error("illegal character");
    case SPC:   PushBuffer_(RESOURCE, NEWSYMBOL);                     break;
    case RAB:   throw runtime_error("illegal character");
    case PER:   throw runtime_error("illegal character");
    case ADD:                                                         break;
    default:    throw runtime_error("illegal character");
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void Tokenizer::NewSymbolState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
  {
    case LAB: ++it_;  it_.Clear();  state_ = HEXSTRING;   break;
    case LET:         it_.Clear();  state_ = IDENTIFIER;  break;
    case DIG:         it_.Clear();  state_ = NUMBER;      break;
    case USC:         it_.Clear();  state_ = IDENTIFIER;  break;
    case LSB:         it_.Clear();  state_ = NEWSYMBOL;   break;
    case RSB:         it_.Clear();  state_ = NEWSYMBOL;   break;
    case FSL:         it_.Clear();  state_ = RESOURCE;    break;
    case AST:         it_.Clear();  state_ = IDENTIFIER;  break;
    case LCB: ++it_;  it_.Clear(); if (it_.GetCharType() == BSL)
                                   EscapeState_();        else
                                   state_ = STRING;       break;
    case SUB:         it_.Clear();  state_ = NUMBER;      break;
    case PER:         it_.Clear();  state_ = NUMBER;      break;
    case SQO:         it_.Clear();  state_ = IDENTIFIER;  break;
    case APO: PushBuffer_(IDENTIFIER, NEWSYMBOL);         break;
    default :         it_.Clear();  state_ = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void Tokenizer::IdentifierState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
  {
    case LAB: PushBuffer_(IDENTIFIER, HEXSTRING); ++it_; it_.Clear(); break;
    case LET:                                                         break;
    case DIG:                                                         break;
                //  BI == inline image
    case SPC: if (it_ == "BI") state_ = WAIT;
              else PushBuffer_(IDENTIFIER, NEWSYMBOL);                break;
    case FSL: PushBuffer_(IDENTIFIER, RESOURCE); --it_;               break;
    case LSB: PushBuffer_(IDENTIFIER, NEWSYMBOL);                     break;
    case LCB: PushBuffer_(IDENTIFIER, STRING);++it_; it_.Clear();     break;
    case SUB:                                                         break;
    case USC:                                                         break;
    case AST:                                                         break;
    default : it_.Clear();                                            break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void Tokenizer::NumberState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
  {
    case LAB:   PushBuffer_(NUMBER, HEXSTRING); ++it_; it_.Clear();   break;
    case DIG:                                                         break;
    case SPC:   PushBuffer_(NUMBER, NEWSYMBOL);                       break;
    case PER:                                                         break;
    case LCB:   PushBuffer_(NUMBER, STRING); ++it_; it_.Clear();      break;
    case LET:                                                         break;
    case USC:                                                         break;
    case SUB:   PushBuffer_(NUMBER, NUMBER);                          break;
    case AST:   PushBuffer_(NUMBER, NUMBER);                          break;
    case FSL:   PushBuffer_(NUMBER, NUMBER);                          break;
    case LSB:   PushBuffer_(NUMBER, ARRAY);                           break;
    default:    PushBuffer_(NUMBER, NEWSYMBOL);
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void Tokenizer::StringState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
  {
    case RCB:   PushBuffer_(STRING, NEWSYMBOL);             break;
    case BSL:   EscapeState_();                             break;
    default:                                                break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is in an array

void Tokenizer::ArrayState_()
{
  state_ = NEWSYMBOL;
  it_.Clear();
}

/*---------------------------------------------------------------------------*/
// lexer is reading a hexstring of format <11FA>

void Tokenizer::HexStringState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
  {
    case RAB:  if (!it_.Empty())
                 PushBuffer_(HEXSTRING, NEWSYMBOL);
               state_ = NEWSYMBOL;                    break;
    case LAB:  it_.Clear(); state_ = DICT;            break;
    case BSL:  ++it_;                                 break;
    default:                                          break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void Tokenizer::DictionaryState_()
{
  // get symbol_type of current char
  switch (it_.GetCharType())
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
  if ( ! it_.Empty()) {--it_; PushBuffer_(STRING, STRING); ++it_;}
  ++it_; it_.Clear();

  // Read the next char - if it's a digit it's likely an octal
  if (it_.GetCharType() == DIG)
  {
    int octcount = 0;
    PushBuffer_(STRING, STRING);

    // Add consecutive chars to octal (up to 3)
    while (it_.GetCharType() == DIG && octcount < 3)
    {
       ++it_;
      octcount++;
    }

    // Convert octal string to int
    int newint = stoi(it_.Contents(), nullptr, 8);
    auto hexstring = ConvertIntToHex(newint);
    state_ = STRING; // switch state
    interpreter_->Reader(hexstring, HEXSTRING); // make pair and push to result
    it_.Clear();
    if(it_.GetCharType() == RCB) state_ = NEWSYMBOL; else state_ = STRING;
  }


}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void Tokenizer::WaitState_()
{

  while (it_.GetChar() != 'E') {++it_; it_.Clear();}
  if(it_.GetChar() != 'I') WaitState_();
  it_.Clear();
  state_ = NEWSYMBOL; // Only break out of wait state by finding EI (or EOF)
}
