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

tokenizer::tokenizer(shared_ptr<string> input, parser* GS) :
 s(input), i(s->begin()), state(NEWSYMBOL), gs(GS)
{
  tokenize();
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
      shared_ptr<string> xo = gs->getXobject(inloop);

      // Don't try to parse binary objects like images etc
      if(IsAscii(*xo)) tokenizer(xo, gs);
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
  while(i != s->end()) // ensure the iterator doesn't exceed string length
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
  switch(char_lookup[*i])
  {
    case LAB:   pushbuf(RESOURCE, HEXSTRING);               break;
    case LET:   buf.append(i, i + 1);                       break;
    case DIG:   buf.append(i, i + 1);                       break;
    case USC:   buf.append("_");                            break;
    case LSB:   pushbuf(RESOURCE, ARRAY);                   break;
    case FSL:   pushbuf(RESOURCE, RESOURCE);                break;
    case AST:   buf.append("*");                            break;
    case LCB:   pushbuf(RESOURCE, STRING);                  break;
    case SUB:   buf.append("-");                            break;
    case BSL:   throw runtime_error("illegal character");
    case SPC:   pushbuf(RESOURCE, NEWSYMBOL);               break;
    case RAB:   throw runtime_error("illegal character");
    case PER:   throw runtime_error("illegal character");
    case ADD:   buf.append("+");                            break;
    default:    throw runtime_error("illegal character");
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is receptive for next token

void tokenizer::newsymbolState()
{
  // get symbol_type of current char
  switch(char_lookup[*i])
  {
    case LAB:                             state = HEXSTRING;   break;
    case LET:   buf.append(i, i + 1);     state = IDENTIFIER;  break;
    case DIG:   buf.append(i, i + 1);     state = NUMBER;      break;
    case USC:   buf.append(i, i + 1);     state = IDENTIFIER;  break;
    case LSB:                             state = ARRAY;       break;
    case FSL:   buf.append(i, i + 1);     state = RESOURCE;    break;
    case AST:   buf.append(i, i + 1);     state = IDENTIFIER;  break;
    case LCB:                             state = STRING;      break;
    case SUB:   buf.append(i, i + 1);     state = NUMBER;      break;
    case PER:   buf.append("0.");         state = NUMBER;      break;
    case SQO:   buf.append(i, i + 1);     state = IDENTIFIER;  break;
    default : buf.clear();                state = NEWSYMBOL;   break;
  }
}

/*---------------------------------------------------------------------------*/
// Lexer is reading an identifier (instruction or keyword)

void tokenizer::identifierState()
{
  // get symbol_type of current char
  switch(char_lookup[*i])
  {
    case LAB:   pushbuf(IDENTIFIER, HEXSTRING); break;
    case LET:   buf.append(i, i + 1);           break;
    case DIG:   buf.append(i, i + 1);           break;
    case SPC:   if (buf == "BI") state = WAIT;  else // BI == inline image
                pushbuf(IDENTIFIER, NEWSYMBOL); break;
    case FSL:   pushbuf(IDENTIFIER, RESOURCE);
                buf = "/";                      break;
    case LSB:   pushbuf(IDENTIFIER, ARRAY);     break;
    case LCB:   pushbuf(IDENTIFIER, STRING);    break;
    case SUB:   buf.append(i, i + 1);           break;
    case USC:   buf.append(i, i + 1);           break;
    case AST:   buf.append(i, i + 1);           break;
    default:                                    break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a number

void tokenizer::numberState()
{
  // get symbol_type of current char
  switch(char_lookup[*i])
  {
    case LAB:   pushbuf(NUMBER, HEXSTRING);           break;
    case DIG:   buf.append(i, i + 1);                 break;
    case SPC:   pushbuf(NUMBER, NEWSYMBOL);           break;
    case PER:   buf.append(i, i + 1);                 break;
    case LCB:   pushbuf(NUMBER, STRING);              break;
    case LET:   buf.append(i, i + 1);                 break;
    case USC:   buf.append(i, i + 1);                 break;
    case SUB:   pushbuf(NUMBER, NUMBER); buf.clear(); break;
    case AST:   pushbuf(NUMBER, NUMBER); buf.clear(); break;
    case FSL:   pushbuf(NUMBER, NUMBER); buf.clear(); break;
    case LSB:   pushbuf(NUMBER, ARRAY);               break;
    default:    pushbuf(NUMBER, NEWSYMBOL);
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a (bracketed) string

void tokenizer::stringState()
{
  // get symbol_type of current char
  switch(char_lookup[*i])
  {
    case RCB:   pushbuf(STRING, NEWSYMBOL);       break;
    case BSL:   escapeState();                    break;
    default:    buf.append(i, i + 1);             break;
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
  switch(char_lookup[*i])
  {
    case RAB:   if (!buf.empty()) pushbuf(HEXSTRING, NEWSYMBOL);
                state = NEWSYMBOL;                                break;
    case LAB:   buf.clear(); state = DICT;                        break;
    case BSL:  buf.append(i, i + 1); ++i; buf.append(i, i + 1);   break;
    default:   buf.append(i, i + 1);                              break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer is reading a dictionary and will keep writing until it comes across
// a pair of closing angle brackets

void tokenizer::dictState()
{
  // get symbol_type of current char
  switch(char_lookup[*i])
  {
    case BSL:  buf.append(i, i + 1); ++i; buf.append(i, i + 1);   break;
    case RAB:  pushbuf(DICT, HEXSTRING);                          break;
    default:   buf.append(i, i + 1);                              break;
  }
}

/*---------------------------------------------------------------------------*/
// lexer has come across a backslash which indicates an escape character

void tokenizer::escapeState()
{
  ++i;

  // Read the next char - if it's a digit it's likely an octal
  if (char_lookup[*i] == DIG)
  {
    int octcount = 0;
    pushbuf(STRING, STRING);

    // Add consecutive chars to octal (up to 3)
    while(char_lookup[*i] == DIG && octcount < 3)
    {
      buf.append(i, i + 1); ++i;
      octcount++;
    }

    // Convert octal string to int
    int newint = stoi(buf, nullptr, 8);
    buf = intToHexstring(newint);
    pushbuf(HEXSTRING, STRING);
    i--;  // Decrement to await next char
  }

  // If not a digit, get escaped char
  else buf.append(i, i + 1);
}

/*---------------------------------------------------------------------------*/
// The lexer has reached an inline image, which indicates it should ignore the
// string until it reaches the keyword "EI" at the end of the image

void tokenizer::waitState()
{
  do
  {
    i = s->begin() + s->find("EI", i - s->begin()) + 2;
  }
  while (char_lookup[*i] != SPC);
  buf.clear();
  state = NEWSYMBOL; // Only break out of wait state by finding EI (or EOF)
}


