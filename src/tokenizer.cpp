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

/*---------------------------------------------------------------------------*/
// constructor of tokenizer - initializes members and starts main
// lexer function

tokenizer::tokenizer(string& input) : i(0), s(input),  state(NEWSYMBOL)
{
  s.push_back(' '); // easier to do this than handle full buffer at EOF
  tokenize();       // instigate lexer
}

/*---------------------------------------------------------------------------*/
// Simple getter which returns main private data member

vector<pair<string, TState>> tokenizer::result()
{
  return output;
}

/*---------------------------------------------------------------------------*/
// This pattern, of switching state, creating a token / type pair, pushing it
// to the instruction set, and clearing the buffer is very common in the
// lexer. This function acts as a shorthand to prevent boilerplate

void tokenizer::pushbuf(TState type, TState statename)
{
  state = statename; // switch state
  output.push_back(make_pair(buf, type)); // make pair and push to result
  buf.clear(); // clear buffer
}

/*---------------------------------------------------------------------------*/
// This function co-ordinates the lexer by calling a subroutine depending on
// the state. Each subroutine handles specific characters in a different but
// well-specified way

void tokenizer::tokenize()
{
  while(i < s.length()) // ensure the iterator doesn't exceed string length
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
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case 'L':   buf += m;                         break;
    case 'D':   buf += m;                         break;
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
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case 'L':   buf += m;    state = IDENTIFIER;  break;
    case 'D':   buf += m;    state = NUMBER;      break;
    case '-':   buf += m;    state = NUMBER;      break;
    case '_':   buf += m;    state = IDENTIFIER;  break;
    case '*':   buf += m;    state = IDENTIFIER;  break;
    case '\'':  buf += m;    state = IDENTIFIER;  break;
    case '/':   buf += m;    state = RESOURCE;    break;
    case '.':   buf += "0."; state = NUMBER;      break;
    case '[':                state = ARRAY;       break;
    case '(':                state = STRING;      break;
    case '<':                state = HEXSTRING;   break;
    default :   buf = "";    state = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void tokenizer::identifierState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case '/':   pushbuf(IDENTIFIER, RESOURCE);
                buf = "/";                        break;
    case ' ':   if (buf == "BI") state = WAIT;    else // BI == inline image
                pushbuf(IDENTIFIER, NEWSYMBOL);   break;
    case '[':   pushbuf(IDENTIFIER, ARRAY);       break;
    case '(':   pushbuf(IDENTIFIER, STRING);      break;
    case '<':   pushbuf(IDENTIFIER, HEXSTRING);   break;
    case 'L':   buf += m;                         break;
    case 'D':   buf += m;                         break;
    case '-':   buf += m;                         break;
    case '_':   buf += m;                         break;
    case '*':   buf += m;                         break;
    default:                                      break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void tokenizer::numberState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case 'L':   buf += m;                         break;
    case 'D':   buf += m;                         break;
    case '_':   buf += m;                         break;
    case '.':   buf += m;                         break;
    case '-':   pushbuf(NUMBER,     NUMBER);      buf=m;
                pushbuf(OPERATOR,   NUMBER);      break;
    case '*':   pushbuf(NUMBER,     NUMBER);      buf=m;
                pushbuf(OPERATOR,   NUMBER);      break;
    case '/':   pushbuf(NUMBER,     NUMBER);      buf=m;
                pushbuf(OPERATOR,   NUMBER);      break;
    case ' ':   pushbuf(NUMBER,     NEWSYMBOL);   break;
    case '[':   pushbuf(NUMBER,     ARRAY);       break;
    case '(':   pushbuf(NUMBER,     STRING);      break;
    case '<':   pushbuf(NUMBER,     HEXSTRING);   break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void tokenizer::stringState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case ')':   pushbuf(STRING, NEWSYMBOL);       break;
    case '\\':  escapeState();                    break;
    default:    buf += s[i];                      break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is in an array

void tokenizer::arrayState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case ']':   subtokenizer(buf); // at end of array, tokenize its contents
                state = NEWSYMBOL; buf = "";      break;
    case '\\':  buf += s[i++]; buf += s[i];       break;
    default:    buf += m;                         break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a hexstring of format <11FA>

void tokenizer::hexstringState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case '>':   if (!buf.empty())
                  pushbuf(HEXSTRING, NEWSYMBOL);
                state = NEWSYMBOL;                break;
    case '<':   buf = ""; state = DICT;           break;
    case '\\':  buf += m + s[++i];                break;
    default:    buf += m;                         break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void tokenizer::dictState()
{
  char m = s[i];            // simplifies code
  char n = symbol_type(m);  // get symbol_type of current char
  switch(n)
  {
    case '\\':  buf += m + s[++i];                break;
    case '>':   pushbuf(DICT, HEXSTRING);         break;
    default:    buf += m;                         break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void tokenizer::escapeState()
{
  char n = symbol_type(s[++i]); // read the next char
  if (n == 'D')                 // if it's a digit it's likely an octal
  {
    int octcount = 0;
    pushbuf(STRING, STRING);
    while(n == 'D' && octcount < 3) // add consecutive chars to octal (up to 3)
    {
      buf += s[i++];
      octcount++;
      n = symbol_type(s[i]);
    }
    int newint = stoi(buf, nullptr, 8); // convert octal string to int
    buf = intToHexstring(newint);
    pushbuf(HEXSTRING, STRING);
    i--;                                // decrement to await next char
  }
  else
    buf += s[i];                  // if not a digit, get escaped char
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void tokenizer::waitState()
{
  // the lexer
  buf =  s[i] + s[i + 1];
  buf += symbol_type(s[i + 2]);
  if(buf == "EI ") // look for EI with any whitespace char following
  {
    buf.clear();
    state = NEWSYMBOL; // only break out of wait state by finding EI (or EOF)
  }
}

/*---------------------------------------------------------------------------*/
// when tokenizer is called recursively on an array within the string, we
// want its instructions to be added to the same stack as the parent string

void tokenizer::subtokenizer(string &str)
{
  state = NEWSYMBOL;
  concat(output, tokenizer(str).result()); // concatenates results + subresults
}
