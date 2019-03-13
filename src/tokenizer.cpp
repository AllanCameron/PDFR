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
  switch(symbol_type(*i))
  {
    case 'L':   buf += *i;                        break;
    case 'D':   buf += *i;                        break;
    case '-':   buf += '-';                       break;
    case '+':   buf += '+';                       break;
    case '_':   buf += '_';                       break;
    case '*':   buf += '*';                       break;
    case '/':   pushbuf(RESOURCE, RESOURCE);      break;
    case ' ':   pushbuf(RESOURCE, NEWSYMBOL);     break;
    case '[':   pushbuf(RESOURCE, ARRAY);         break;
    case '(':   pushbuf(RESOURCE, STRING);        break;
    case '<':   pushbuf(RESOURCE, HEXSTRING);     break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void tokenizer::newsymbolState()
{
  // get symbol_type of current char
  switch(symbol_type(*i))
  {
    case 'L':   buf += *i;    state = IDENTIFIER;  break;
    case 'D':   buf += *i;    state = NUMBER;      break;
    case '-':   buf += *i;    state = NUMBER;      break;
    case '_':   buf += *i;    state = IDENTIFIER;  break;
    case '*':   buf += *i;    state = IDENTIFIER;  break;
    case '\'':  buf += *i;    state = IDENTIFIER;  break;
    case '/':   buf += *i;    state = RESOURCE;    break;
    case '.':   buf +=  "0."; state = NUMBER;      break;
    case '[':                 state = ARRAY;       break;
    case '(':                 state = STRING;      break;
    case '<':                 state = HEXSTRING;   break;
    default : buf = "";       state = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void tokenizer::identifierState()
{
  // get symbol_type of current char
  switch(symbol_type(*i))
  {
    case 'L':   buf += *i;                      break;
    case ' ':   if (buf == "BI") state = WAIT;  else // BI == inline image
                pushbuf(IDENTIFIER, NEWSYMBOL); break;
    case '/':   pushbuf(IDENTIFIER, RESOURCE);
                buf = "/";                      break;
    case '[':   pushbuf(IDENTIFIER, ARRAY);     break;
    case '(':   pushbuf(IDENTIFIER, STRING);    break;
    case '<':   pushbuf(IDENTIFIER, HEXSTRING); break;
    case 'D':   buf += *i;                      break;
    case '-':   buf += *i;                      break;
    case '_':   buf += *i;                      break;
    case '*':   buf += *i;                      break;
    default:                                    break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void tokenizer::numberState()
{
  // get symbol_type of current char
  switch(symbol_type(*i))
  {
    case 'D':   buf += *i;                         break;
    case ' ':   pushbuf(NUMBER, NEWSYMBOL);        break;
    case '.':   buf += *i;                         break;
    case '(':   pushbuf(NUMBER, STRING);           break;
    case '<':   pushbuf(NUMBER, HEXSTRING);        break;
    case 'L':   buf += *i;                         break;
    case '_':   buf += *i;                         break;
    case '-':   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case '*':   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case '/':   pushbuf(NUMBER, NUMBER); buf = ""; break;
    case '[':   pushbuf(NUMBER, ARRAY);            break;
    default:    pushbuf(NUMBER, NEWSYMBOL);
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void tokenizer::stringState()
{
  // get symbol_type of current char
  switch(symbol_type(*i))
  {
    case ')':   pushbuf(STRING, NEWSYMBOL);       break;
    case '\\':  escapeState();                    break;
    default:    buf += *i;                      break;
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
  switch(symbol_type(*i))
  {
    case '>':   if (!buf.empty())
                  pushbuf(HEXSTRING, NEWSYMBOL);
                state = NEWSYMBOL;                break;
    case '<':   buf = ""; state = DICT;           break;
    case '\\':  buf += *i; i++; buf += *i;        break;
    default:    buf += *i;                        break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void tokenizer::dictState()
{
  // get symbol_type of current char
  switch(symbol_type(*i))
  {
    case '\\':  buf += *i; i++; buf += *i;        break;
    case '>':   pushbuf(DICT, HEXSTRING);         break;
    default:    buf += *i;                        break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void tokenizer::escapeState()
{
  i++;
   // read the next char
  if (symbol_type(*i) == 'D') // if it's a digit it's likely an octal
  {
    int octcount = 0;
    pushbuf(STRING, STRING);
    while(symbol_type(*i) == 'D' && octcount < 3)
    {// add consecutive chars to octal (up to 3)
      buf += *i; i++;
      octcount++;
    }
    int newint = stoi(buf, nullptr, 8); // convert octal string to int
    buf = intToHexstring(newint);
    pushbuf(HEXSTRING, STRING);
    i--;                                // decrement to await next char
  }
  else buf += *i;                       // if not a digit, get escaped char
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void tokenizer::waitState()
{
  // the lexer
  buf = *i; i++; buf += *i; i++;
  buf += symbol_type(*i); i--; i--;
  if(buf == "EI ") // look for EI with any whitespace char following
  {
    buf.clear();
    state = NEWSYMBOL; // only break out of wait state by finding EI (or EOF)
  }
}


