//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR tokenizer implementation file                                       //
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

std::string tokenizer::inloop = "none";

std::array<tokenizer::chartype, 256> tokenizer::char_lookup = {
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
// constructor of tokenizer - initializes members and starts main
// lexer function

tokenizer::tokenizer(string&& input, parser* GS) :
 s(input), i(s.begin()), state(NEWSYMBOL), gs(GS)
{
  tokenize();       // instigate lexer
}


/*---------------------------------------------------------------------------*/
// This pattern, of switching state, creating a token / type pair, pushing it
// to the instruction set, and clearing the buffer is very common in the
// lexer. This function acts as a shorthand to prevent boilerplate

void tokenizer::pushbuf(const TState type, const TState statename)
{
  if(buf == "Do" && statename == IDENTIFIER)
  {
    string loopname = gs->getOperand();
    if(loopname != inloop)
    {
      inloop = loopname;
      string&& xo = gs->getXobject(inloop);
      if(IsAscii(xo)) // don't try to parse binary objects like images etc
        tokenizer(move(xo), gs);
    }
  }
  state = statename; // switch state
  gs->reader(buf, type); // make pair and push to result
  buf.clear(); // clear buffer
}

/*---------------------------------------------------------------------------*/
// This function co-ordinates the lexer by calling a subroutine depending on
// the state. Each subroutine handles specific characters in a different but
// well-specified way

void tokenizer::tokenize()
{
  while(i != s.end()) // ensure the iterator doesn't exceed string length
  {
    j = *i;
    switch(state)
    {
      // Each state has its own handler subroutine - self explanatory
      case NEWSYMBOL:   newsymbolState();         break;
      case RESOURCE:    resourceState();          break;
      case IDENTIFIER:  identifierState();        break;
      case NUMBER:      numberState();            break;
      case ARRAY:       arrayState();             break;
      case STRING:      stringState();            break;
      case HEXSTRING:   hexstringState();         break;
      case DICT:        dictState();              break;
      case WAIT:        waitState();              break;
      case OPERATOR:                              break;
    }
    ++i; // move to next character in the string
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading a resource (a /PdfName)

void tokenizer::resourceState()
{
  switch(char_lookup[j])
  {
    case LET:   buf += j;                        break;
    case DIG:   buf += j;                        break;
    case SUB:   buf += '-';                       break;
    case ADD:   buf += '+';                       break;
    case USC:   buf += '_';                       break;
    case AST:   buf += '*';                       break;
    case FSL:   pushbuf(RESOURCE, RESOURCE);      break;
    case SPC:   pushbuf(RESOURCE, NEWSYMBOL);     break;
    case LSB:   pushbuf(RESOURCE, ARRAY);         break;
    case LCB:   pushbuf(RESOURCE, STRING);        break;
    case LAB:   pushbuf(RESOURCE, HEXSTRING);     break;
  default: throw runtime_error("illegal character");
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void tokenizer::newsymbolState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case LET:   buf += j;    state = IDENTIFIER;  break;
    case DIG:   buf += j;    state = NUMBER;      break;
    case SUB:   buf += j;    state = NUMBER;      break;
    case USC:   buf += j;    state = IDENTIFIER;  break;
    case AST:   buf += j;    state = IDENTIFIER;  break;
    case SQO:  buf += j;    state = IDENTIFIER;  break;
    case FSL:   buf += j;    state = RESOURCE;    break;
    case PER:   buf +=  "0."; state = NUMBER;      break;
    case LSB:                 state = ARRAY;       break;
    case LCB:                 state = STRING;      break;
    case LAB:                 state = HEXSTRING;   break;
    default : buf = "";       state = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void tokenizer::identifierState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case LET:   buf += j;                      break;
    case SPC:   if (buf == "BI") state = WAIT;  else // BI == inline image
                pushbuf(IDENTIFIER, NEWSYMBOL); break;
    case FSL:   pushbuf(IDENTIFIER, RESOURCE);
                buf = "/";                      break;
    case LSB:   pushbuf(IDENTIFIER, ARRAY);     break;
    case LCB:   pushbuf(IDENTIFIER, STRING);    break;
    case LAB:   pushbuf(IDENTIFIER, HEXSTRING); break;
    case DIG:   buf += j;                      break;
    case SUB:   buf += j;                      break;
    case USC:   buf += j;                      break;
    case AST:   buf += j;                      break;
    default:                                    break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void tokenizer::numberState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case DIG:   buf += j;                         break;
    case SPC:   pushbuf(NUMBER, NEWSYMBOL);        break;
    case PER:   buf += j;                         break;
    case LCB:   pushbuf(NUMBER, STRING);           break;
    case LAB:   pushbuf(NUMBER, HEXSTRING);        break;
    case LET:   buf += j;                         break;
    case USC:   buf += j;                         break;
    case SUB:   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case AST:   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case FSL:   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case LSB:   pushbuf(NUMBER, ARRAY);            break;
    default:    pushbuf(NUMBER, NEWSYMBOL);
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void tokenizer::stringState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case RCB:   pushbuf(STRING, NEWSYMBOL);       break;
    case BSL:  escapeState();                    break;
    default:    buf += j;                      break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is in an array

void tokenizer::arrayState()
{
  i--;
  state = NEWSYMBOL;
}

/*---------------------------------------------------------------------------*/
// lexer is reading a hexstring of format <11FA>

void tokenizer::hexstringState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case RAB:   if (!buf.empty())
                  pushbuf(HEXSTRING, NEWSYMBOL);
                state = NEWSYMBOL;                break;
    case LAB:   buf = ""; state = DICT;           break;
    case BSL:  buf += j; i++; buf += j;        break;
    default:    buf += j;                        break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void tokenizer::dictState()
{
  // get symbol_type of current char
  switch(char_lookup[j])
  {
    case BSL:  buf += j; i++; buf += j;        break;
    case RAB:   pushbuf(DICT, HEXSTRING);         break;
    default:    buf += j;                        break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void tokenizer::escapeState()
{
  i++;
   // read the next char
  if (char_lookup[j] == DIG) // if it's a digit it's likely an octal
  {
    int octcount = 0;
    pushbuf(STRING, STRING);
    while(char_lookup[j] == DIG && octcount < 3)
    {// add consecutive chars to octal (up to 3)
      buf += j; i++;
      octcount++;
    }
    int newint = stoi(buf, nullptr, 8); // convert octal string to int
    buf = intToHexstring(newint);
    pushbuf(HEXSTRING, STRING);
    i--;                                // decrement to await next char
  }
  else buf += j;                       // if not a digit, get escaped char
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void tokenizer::waitState()
{
  // the lexer
  buf = j; i++; buf += *i; i++;
  buf += *i; i--; i--;
  if(buf == "EI " || buf == "EI\n" || buf == "EI\r") // look for EI and WS
  {
    buf.clear();
    state = NEWSYMBOL; // only break out of wait state by finding EI (or EOF)
  }
}


