//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Tokenizer implementation file                                       //
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

#include "tokenizer.h"

using namespace std;
using namespace Token;

std::string Tokenizer::in_loop_ = "none";

std::array<Tokenizer::CharType, 256> Tokenizer::char_lookup_ = {
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, SPC, OTH, OTH, SPC, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,
  SPC, OTH, QOT, OTH, OTH, OTH, SQO, OTH,
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

Tokenizer::Tokenizer(shared_ptr<string> t_input, parser* t_interpreter) :
  contents_(t_input),
  it_(contents_->begin()),
  state_(NEWSYMBOL),
  interpreter_(t_interpreter)
{
  Tokenize();
}


/*---------------------------------------------------------------------------*/
// This pattern, of switching state, creating a token / type pair, pushing it
// to the instruction set, and clearing the buffer is very common in the
// lexer. This function acts as a shorthand to prevent boilerplate

void Tokenizer::PushBuffer(const TState t_type, const TState t_state_name)
{
  if(buffer_ == "Do" && t_state_name == IDENTIFIER)
  {
    string loop_name = interpreter_->getOperand();
    if(loop_name != in_loop_)
    {
      in_loop_ = loop_name;
      shared_ptr<string> xobject = interpreter_->getXobject(in_loop_);

      // Don't try to parse binary objects like images etc
      if(IsAscii(*xobject)) Tokenizer(xobject, interpreter_);
    }
  }

  state_ = t_state_name; // switch state
  interpreter_->reader(buffer_, t_type); // make pair and push to result
  buffer_.clear(); // clear buffer
}

/*---------------------------------------------------------------------------*/
// This function co-ordinates the lexer by calling a subroutine depending on
// the state. Each subroutine handles specific characters in a different but
// well-specified way

void Tokenizer::Tokenize()
{
  // Ensures the iterator doesn't exceed string length
  while (it_ != contents_->end())
  {
    switch (state_)
    {
      // Each state has its own handler subroutine - self explanatory
      case NEWSYMBOL:   NewSymbolState();         break;
      case RESOURCE:    ResourceState();          break;
      case IDENTIFIER:  IdentifierState();        break;
      case NUMBER:      NumberState();            break;
      case ARRAY:       ArrayState();             break;
      case STRING:      StringState();            break;
      case HEXSTRING:   HexStringState();         break;
      case DICT:        DictionaryState();        break;
      case WAIT:        WaitState();              break;
      case OPERATOR:                              break;
    }
    ++it_; // move to next character in the string
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading a resource (a /PdfName)

void Tokenizer::ResourceState()
{
  switch (char_lookup_[*it_])
  {
    case LAB:   PushBuffer(RESOURCE, HEXSTRING);              break;
    case LET:   buffer_.append(it_, it_ + 1);                 break;
    case DIG:   buffer_.append(it_, it_ + 1);                 break;
    case USC:   buffer_.append("_");                          break;
    case LSB:   PushBuffer(RESOURCE, ARRAY);                  break;
    case FSL:   PushBuffer(RESOURCE, RESOURCE);               break;
    case AST:   buffer_.append("*");                          break;
    case LCB:   PushBuffer(RESOURCE, STRING);                 break;
    case SUB:   buffer_.append("-");                          break;
    case BSL:   throw runtime_error("illegal character");
    case SPC:   PushBuffer(RESOURCE, NEWSYMBOL);              break;
    case RAB:   throw runtime_error("illegal character");
    case PER:   throw runtime_error("illegal character");
    case ADD:   buffer_.append("+");                          break;
    default:    throw runtime_error("illegal character");
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void Tokenizer::NewSymbolState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case LAB:                                 state_ = HEXSTRING;   break;
    case LET:   buffer_.append(it_, it_ + 1); state_ = IDENTIFIER;  break;
    case DIG:   buffer_.append(it_, it_ + 1); state_ = NUMBER;      break;
    case USC:   buffer_.append(it_, it_ + 1); state_ = IDENTIFIER;  break;
    case LSB:                                 state_ = ARRAY;       break;
    case FSL:   buffer_.append(it_, it_ + 1); state_ = RESOURCE;    break;
    case AST:   buffer_.append(it_, it_ + 1); state_ = IDENTIFIER;  break;
    case LCB:                                 state_ = STRING;      break;
    case SUB:   buffer_.append(it_, it_ + 1); state_ = NUMBER;      break;
    case PER:   buffer_.append("0.");         state_ = NUMBER;      break;
    case SQO:   buffer_.append(it_, it_ + 1); state_ = IDENTIFIER;  break;
    default : buffer_.clear();                state_ = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void Tokenizer::IdentifierState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case LAB:   PushBuffer(IDENTIFIER, HEXSTRING);      break;
    case LET:   buffer_.append(it_, it_ + 1);           break;
    case DIG:   buffer_.append(it_, it_ + 1);           break;
    case SPC:   if (buffer_ == "BI") state_ = WAIT;     // BI == inline image
                else PushBuffer(IDENTIFIER, NEWSYMBOL); break;
    case FSL:   PushBuffer(IDENTIFIER, RESOURCE);
                buffer_ = "/";                          break;
    case LSB:   PushBuffer(IDENTIFIER, ARRAY);          break;
    case LCB:   PushBuffer(IDENTIFIER, STRING);         break;
    case SUB:   buffer_.append(it_, it_ + 1);           break;
    case USC:   buffer_.append(it_, it_ + 1);           break;
    case AST:   buffer_.append(it_, it_ + 1);           break;
    default:                                            break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void Tokenizer::NumberState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case LAB:   PushBuffer(NUMBER, HEXSTRING);                break;
    case DIG:   buffer_.append(it_, it_ + 1);                 break;
    case SPC:   PushBuffer(NUMBER, NEWSYMBOL);                break;
    case PER:   buffer_.append(it_, it_ + 1);                 break;
    case LCB:   PushBuffer(NUMBER, STRING);                   break;
    case LET:   buffer_.append(it_, it_ + 1);                 break;
    case USC:   buffer_.append(it_, it_ + 1);                 break;
    case SUB:   PushBuffer(NUMBER, NUMBER); buffer_.clear();  break;
    case AST:   PushBuffer(NUMBER, NUMBER); buffer_.clear();  break;
    case FSL:   PushBuffer(NUMBER, NUMBER); buffer_.clear();  break;
    case LSB:   PushBuffer(NUMBER, ARRAY);                    break;
    default:    PushBuffer(NUMBER, NEWSYMBOL);
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void Tokenizer::StringState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case RCB:   PushBuffer(STRING, NEWSYMBOL);            break;
    case BSL:   EscapeState();                            break;
    default:    buffer_.append(it_, it_ + 1);             break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is in an array

void Tokenizer::ArrayState()
{
  it_--;
  state_ = NEWSYMBOL;
}

/*---------------------------------------------------------------------------*/
// lexer is reading a hexstring of format <11FA>

void Tokenizer::HexStringState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case RAB: if (!buffer_.empty()) PushBuffer(HEXSTRING, NEWSYMBOL);
               state_ = NEWSYMBOL;                                    break;
    case LAB:  buffer_.clear(); state_ = DICT;                        break;
    case BSL:  buffer_.append(it_, it_ + 1); ++it_;
               buffer_.append(it_, it_ + 1);                          break;
    default:   buffer_.append(it_, it_ + 1);                          break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void Tokenizer::DictionaryState()
{
  // get symbol_type of current char
  switch (char_lookup_[*it_])
  {
    case BSL:  buffer_.append(it_, it_ + 1); ++it_;
               buffer_.append(it_, it_ + 1);                          break;
    case RAB:  PushBuffer(DICT, HEXSTRING);                           break;
    default:   buffer_.append(it_, it_ + 1);                          break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void Tokenizer::EscapeState()
{
  ++it_;

  // Read the next char - if it's a digit it's likely an octal
  if (char_lookup_[*it_] == DIG)
  {
    int octcount = 0;
    PushBuffer(STRING, STRING);

    // Add consecutive chars to octal (up to 3)
    while (char_lookup_[*it_] == DIG && octcount < 3)
    {
      buffer_.append(it_, it_ + 1); ++it_;
      octcount++;
    }

    // Convert octal string to int
    int newint = stoi(buffer_, nullptr, 8);
    buffer_ = ConvertIntToHex(newint);
    PushBuffer(HEXSTRING, STRING);
    it_--;  // Decrement to await next char
  }

  // If not a digit, get escaped char
  else buffer_.append(it_, it_ + 1);
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void Tokenizer::WaitState()
{
  do
  {
    auto ei_position = contents_->find("EI", it_ - contents_->begin()) + 2;
    it_ = contents_->begin() + ei_position;
  }
  while (char_lookup_[*it_] != SPC);
  buffer_.clear();
  state_ = NEWSYMBOL; // Only break out of wait state by finding EI (or EOF)
}
