//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR dictionary implementation file                                      //
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

#include "pdfr.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "dictionary.h"

using namespace std;

/*---------------------------------------------------------------------------*/

void dictionary::printdict()
{
  vector<string> dickeys = this->getDictKeys();
  for(auto i : dickeys)
    cout << i << ": " << DictionaryMap[i] << std::endl;
}

/*---------------------------------------------------------------------------*/

void dictionary::tokenize_dict()
{
  size_t maxlen = i + 100000;
  while(i < (*s).length() && i < maxlen)
  {
    char n = symbol_type((*s)[i]);
    switch(state)
    {
      case PREENTRY:    if(n == '<')  state = MAYBE;                    break;
      case MAYBE:       handleMaybe(n);                                 break;
      case START:       handleStart(n);                                 break;
      case KEY:         handleKey(n);                                   break;
      case PREVALUE:    handlePrevalue(n);                              break;
      case VALUE:       handleValue(n);                                 break;
      case ARRAYVAL:    handleArrayval(n);                              break;
      case DSTRING:     handleDstring(n);                               break;
      case QUERYDICT:   handleQuerydict(n);                             break;
      case SUBDICT:     handleSubdict(n);                               break;
      case QUERYCLOSE:  if(n == '>') state = CLOSE; else state = START; break;
      case CLOSE:       handleClose(n);                                 break;
      case THE_END:     return;
    }
    ++i;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::sortkey(string b, DState st)
{
  if(!keyPending)
    pendingKey = buf;
  else
    DictionaryMap[pendingKey] = buf;
  keyPending = !keyPending;
  buf = b;
  state = st;
}

/*---------------------------------------------------------------------------*/

void dictionary::assignValue(string b, DState st)
{
  DictionaryMap[pendingKey] = buf;
  keyPending = false;
  buf = b;
  state = st;
}

/*---------------------------------------------------------------------------*/

void dictionary::handleKey(char n)
{
  switch(n)
  {
    case 'L': buf += (*s)[i];                 break;
    case 'D': buf += (*s)[i];                 break;
    case '+': buf += (*s)[i];                 break;
    case '-': buf += (*s)[i];                 break;
    case '_': buf += (*s)[i];                 break;
    case '/': sortkey("/",       KEY);      break;
    case ' ': sortkey("",   PREVALUE);      break;
    case '(': sortkey("(",   DSTRING);      break;
    case '[': sortkey("[",  ARRAYVAL);      break;
    case '<': sortkey("",  QUERYDICT);      break;
    case '>': sortkey("", QUERYCLOSE);      break;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handleMaybe(char n)
{
  switch(n)
  {
    case '<': state = START;                 break;
    default:  buf =""; state = PREENTRY;     break;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handleStart(char n)
{
  switch(n)
  {
    case '/': buf += '/'; state = KEY;      break;
    case '>': state = QUERYCLOSE;           break;
    default :                               break;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handlePrevalue(char n)
{
  switch(n)
  {
    case ' ': state = PREVALUE;             break;
    case '<': state = QUERYDICT;            break;
    case '>': state = QUERYCLOSE;           break;
    case '/': state = KEY;      buf = '/';  break;
    case '[': state = ARRAYVAL; buf = '[';  break;
    default : state = VALUE;    buf = (*s)[i]; break;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handleValue(char n)
{
  switch(n)
  {
    case '/': assignValue("/", KEY);        break;
    case '<': assignValue("", QUERYDICT);   break;
    case '>': assignValue("", QUERYCLOSE);  break;
    case ' ': buf += ' ';                   break;
    default : buf += (*s)[i];                  break;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handleArrayval(char n)
{
  buf += (*s)[i];
  if(n == ']')
    assignValue("", START);
}

/*---------------------------------------------------------------------------*/

void dictionary::handleDstring(char n)
{
  buf += (*s)[i];
  if(n == ')')
    assignValue("", START);
}

/*---------------------------------------------------------------------------*/

void dictionary::handleQuerydict(char n)
{
  if(n == '<')
  {
    buf = "<<";
    state = SUBDICT;
    minibuf = 2;
  }
  else
  {
    buf = "";
    state = START;
  }
}

/*---------------------------------------------------------------------------*/

void dictionary::handleSubdict(char n)
{
  switch(n)
  {
    case '<': buf += (*s)[i]; minibuf ++; break;
    case '>': buf += (*s)[i]; minibuf --; break;
    default: buf += (*s)[i]; break;
  }
  if (minibuf == 0)
    assignValue("", START);
}

/*---------------------------------------------------------------------------*/

void dictionary::handleClose(char n)
{
  switch(n)
  {
    case ' ': state = CLOSE; break;
    case 'L': if(i < (*s).length() - 7)
              {
                if((*s).substr(i, 6) == "stream")
                {
                int ex = 7;
                while(symbol_type((*s)[i + ex]) == ' ')
                  ex++;
                DictionaryMap["stream"] =
                  to_string(i + ex);
                }
              }
              state = THE_END;
    default:  state = THE_END;
  }
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary(string* str) :
  s(str), i(0), minibuf(0), keyPending(false), state(PREENTRY)
{
  if(((*s).length() == 0)) *this = dictionary();
  tokenize_dict();
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary(string* str, size_t pos) :
  s(str), i(pos), minibuf(0), keyPending(false), state(PREENTRY)
{
  if(((*s).length() == 0) || (i >= (*s).length())) *this = dictionary();
  else tokenize_dict();
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary()
{
  map<string, string> Empty;
  DictionaryMap = Empty;
}

/*---------------------------------------------------------------------------*/

string dictionary::get(const string& Key)
{
  return DictionaryMap[Key];
}

/*---------------------------------------------------------------------------*/

bool dictionary::has(const string& Key)
{
  return DictionaryMap.find(Key) != DictionaryMap.end();
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasRefs(const string& Key)
{
  if(this->has(Key)) return this->getRefs(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasInts(const string& Key)
{
  if(this->has(Key)) return this->getInts(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/

vector<int> dictionary::getRefs(const string& Key)
{
  vector<int> References;
  if(this->has(Key))
  {
    string keyval = this->get(Key);
    string refmatch = "(0|1|2|3|4|5|6|7|8|9)+ (0|1|2|3|4|5|6|7|8|9) R";
    vector<string> refs = Rex(keyval, refmatch).get();
    for (auto i : refs) References.push_back(stoi(i));
  }
  return References;
}

/*---------------------------------------------------------------------------*/

vector<int> dictionary::getInts(const string& Key)
{
  vector<int> blank;
  if(this->has(Key)) return getints(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

vector<float> dictionary::getNums(const string& Key)
{
  vector<float> blank;
  if(this->has(Key)) return getnums(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

dictionary dictionary::getDictionary(const string& Key)
{
  if(this->has(Key))
  {
    string dict = this->get(Key);
    if(isDictString(dict)) return dictionary(&dict);
  }
  return dictionary();
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasDictionary(const string& Key)
{
  if(this->has(Key)) return isDictString(this->get(Key));

  return false;
}

/*---------------------------------------------------------------------------*/

vector<string> dictionary::getDictKeys()
{
  return getKeys(this->DictionaryMap);
}
