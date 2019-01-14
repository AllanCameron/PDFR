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

tokenizer::tokenizer(string& input) : i(0), s(input),  state(NEWSYMBOL)
{
  s.push_back(' ');
  tokenize();
}

/*---------------------------------------------------------------------------*/

vector<pair<string, TState>> tokenizer::result()
{
  return output;
}

/*---------------------------------------------------------------------------*/

void tokenizer::pushbuf(TState type, TState statename)
{
  state = statename;
  output.push_back(make_pair(buf, type));
  buf.clear();
}

/*---------------------------------------------------------------------------*/

void tokenizer::tokenize()
{
  while(i < s.length())
  {
    switch(state)
    {
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
    ++i;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::resourceState()
{
  char m = s[i];
  char n = symbol_type(m);
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

void tokenizer::newsymbolState()
{
  char m = s[i];
  char n = symbol_type(m);
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

void tokenizer::identifierState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case '/':   pushbuf(IDENTIFIER, RESOURCE);
                buf = "/";                        break;
    case ' ':   if (buf == "BI") state = WAIT;    else
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

void tokenizer::numberState()
{
  char m = s[i];
  char n = symbol_type(m);
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

void tokenizer::stringState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case ')':   pushbuf(STRING, NEWSYMBOL);       break;
    case '\\':  escapeState();                    break;
    default:    buf += s[i];                      break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::arrayState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case ']':   subtokenizer(buf);
                state = NEWSYMBOL; buf = "";      break;
    case '\\':  buf += s[i++]; buf += s[i];       break;
    default:    buf += m;                         break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::hexstringState()
{
  char m = s[i];
  char n = symbol_type(m);
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

void tokenizer::dictState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case '\\':  buf += m + s[++i];                break;
    case '>':   pushbuf(DICT, HEXSTRING);         break;
    default:    buf += m;                         break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::escapeState()
{
  char n = symbol_type(s[++i]);
  if (n == 'D')
  {
    int octcount = 0;
    pushbuf(STRING, STRING);
    while(n == 'D' && octcount < 3)
    {
      buf += s[i++];
      octcount++;
      n = symbol_type(s[i]);
    }
    int newint = oct2dec(stoi(buf));
    buf = intToHexstring(newint);
    pushbuf(HEXSTRING, STRING);
    i--;
  }
  else
    buf += s[i];
}

/*---------------------------------------------------------------------------*/

void tokenizer::waitState()
{
  buf =  s[i] + s[i + 1];
  buf += symbol_type(s[i + 2]);
  if(buf == "EI ")
  {
    buf.clear();
    state = NEWSYMBOL;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::subtokenizer(string &str)
{
  state = NEWSYMBOL;
  concat(output, tokenizer(str).result());
}
