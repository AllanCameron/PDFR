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

#include "dictionary.h"

using namespace std;

/* Almost all of the work done by dictionary class is in the creation of its
 * main data member, an unordered_map representing the key:value pairs in a
 * pdf dictionary.
 *
 * An important design decision was how to model the dictionaries in C++, since
 * the values can take one of six types as described in dictionary.h.
 *
 * It would be possible to use unions or specialised classes to deal with
 * any entry, but in the end it seemed more sensible to store values as
 * strings and try to return requested types when possible. This seemed an
 * easier way of protecting type safety as well as reducing the amount of
 * processing required to produce a dictionary object.
 *
 * The job of parsing the pdf dictionary is done by a hand-coded lexer. Rather
 * than writing this as one huge function, I split it into several small
 * functions, one each to handle the various states of a finite state machine.
 * Each function uses switch-case expressions to describe how various
 * characters should be dealt with when the lexer is in a particular state.
 *
 * The state is described by an enum, and the character of interest is tested
 * for its type - either letter, digit, whitespace or miscellaneous using the
 * get_symbol_type function defined in utilities.cpp. Any miscellaneous chars
 * that need to be handled by a specific state can be done so because
 * get_symbol_type returns the original char if it is not a letter, digit or
 * whitespace.
 *
 * The rest of the functions are essentially just getters, which request or
 * test for particular data types and act as the public interface for the class.
 *
 * A word about the non-standard layout of the code here: I find this layout
 * much more readable for switch statements, and the same format is used
 * elsewhere in this codebase for lexers which makes them more obvious and
 * easier to spot and debug. It's very unlikely to be to everyone's taste
 * though, so apologies if you find it hard to read.
 */

/*---------------------------------------------------------------------------*/
// A 'magic number' to specify the maximum length of string that the dictionary
// lexer will look through to find a dictionary

constexpr size_t MAX_DICT_LEN = 100000;

//---------------------------------------------------------------------------//
class dict_builder
{
public:
  dict_builder(std::shared_ptr<const std::string>);
  dict_builder(std::shared_ptr<const std::string>, size_t);
  dict_builder();
  std::unordered_map<std::string, std::string>&& get();

private:
  enum DState   {PREENTRY,
                 QUERYCLOSE,
                 VALUE,
                 MAYBE,
                 START,
                 KEY,
                 PREVALUE,
                 DSTRING,
                 ARRAYVAL,
                 QUERYDICT,
                 SUBDICT,
                 CLOSE,
                 THE_END};

  // Private data members

  std::shared_ptr<const std::string> s;   // pointer to the string being read
  char S;           // The actual character being read
  size_t i;         // the string's iterator which is passed between functions
  int bracket;      // integer to store the nesting level of angle brackets
  bool keyPending;  // flag that indicates a key name has been read
  std::string buf;  // string to hold the read characters in memory until needed
  std::string pendingKey; // name of key waiting for a value
  DState state;     // current state of fsm
  std::unordered_map<std::string, std::string> m_Map; // data holder

  // Private functions
  void tokenize_dict(); // co-ordinates the lexer
  void setkey(std::string, DState); //----//
  void assignValue(std::string, DState);  //
  void handleMaybe(char);                 //
  void handleStart(char);                 //
  void handleKey(char);                   //
  void handlePrevalue(char);              //--> functions to handle lexer states
  void handleValue(char);                 //
  void handleArrayval(char);              //
  void handleDstring(char);               //
  void handleQuerydict(char);             //
  void handleSubdict(char);               //
  void handleClose(char);           //----//
};

/*---------------------------------------------------------------------------*/
// This is the main loop which iterates through the string, reads the char,
// finds its type and then runs the subroutine that deals with the current
// state of the machine.

void dict_builder::tokenize_dict()
{
  // The lexer would go through an entire string without halting if it didn't
  // come across a dictionary. To prevent this in the event of a massive file,
  // set a limit on how far the lexer will read into a string
  size_t maxlen = i + MAX_DICT_LEN;
  // Main loop : read next char from string and pass to state handling function
  while (i < s->length() && i < maxlen)
  {
    S = (*s)[i];
    char n = get_symbol_type(S); // determine char type at start of each loop
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
      case THE_END:     return; // Stops the loop iff end state reached
    }
    ++i; // Don't forget to increment...
  }
}

/*---------------------------------------------------------------------------*/
// Often, and confusingly, a pdf Name is given as the value to be stored.
// This function determines whether a pdf name is to be used as the key or the
// value of a key:value pair. It does this by reading whether a key is expected
// or not using the keyPending flag. If there is a key waiting for a value, the
// keyPending flag is true and this function knows to write the name as a value
// to the data map. Otherwise, it knows the name it has just read is intended
// as a key name. In either case the buffer is read and reset and the keypending
// flag is flipped. Although this code is short, it is efficient and used a lot
// so needs its own function.

void dict_builder::setkey(string b, DState st)
{
  if(!keyPending)
    pendingKey = buf; // If no key is awaiting a value, store the name as a key
  else
    m_Map[pendingKey] = buf; // else the name is a value so store it
  keyPending = !keyPending; // flip the buffer flag in either case
  buf = b;
  state = st;  // set buffer and state as needed
}

/*---------------------------------------------------------------------------*/
// The pattern of assigning a value to a waiting key name crops up often
// enough to warrant this function to reduce duplication and error

void dict_builder::assignValue(string b, DState st)
{
  m_Map[pendingKey] = buf; // Contents of buffer assigned to key name
  keyPending = false;              // No key pending - ready for a new key
  buf = b;                         //
  state = st;                      // Defined values of buf and state set
}

/*---------------------------------------------------------------------------*/
// Handles the KEY state of the lexer. Reads characters and digits as a name
// along with a few other legal characters. Otherwise knows it is at the end
// of the name value and switches to the appropriate state depending on the
// next character

void dict_builder::handleKey(char n)
{
  switch(n)
  {
    case 'L': buf += S;                         break;
    case 'D': buf += S;                         break;
    case '+': buf += S;                         break;
    case '-': buf += S;                         break;
    case '_': buf += S;                         break;
    case '/': setkey("/",       KEY);           break; // A new name has begun
    case ' ': setkey("",   PREVALUE);           break; // await next new value
    case '(': setkey("(",   DSTRING);           break; // must be a string
    case '[': setkey("[",  ARRAYVAL);           break; // must be an array
    case '<': setkey("",  QUERYDICT);           break; // probably a dictionary
    case '>': setkey("", QUERYCLOSE);           break; // likely end of dict.
  }
}

/*---------------------------------------------------------------------------*/
// In this state, the lexer is waiting for the dictionary to start. It has
// just come across a '<' and knows it has encountered a dictionary if the
// next char is also a '<'. Otherwise it returns to a waiting state.

void dict_builder::handleMaybe(char n)
{
  if(n == '<')
    state = START;
  else
  {
    buf.clear();
    state = PREENTRY;
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just entered a dictionary. It should start with a key name,
// indicated by a '/'. If not, it will wait until it finds one or finds the
// end of the dictionary

void dict_builder::handleStart(char n)
{
  switch(n)
  {
    case '/': buf += '/'; state = KEY;          break; // should always be so
    case '>': state = QUERYCLOSE;               break; // empty dictionary
    default :                                   break; // linebreaks etc - wait
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just read a key name and now expects a value

void dict_builder::handlePrevalue(char n)
{
  switch(n)
  {
    case ' ': state = PREVALUE;                 break; // still waiting
    case '<': state = QUERYDICT;                break; // probable dict value
    case '>': state = QUERYCLOSE;               break; // ?end of dictionary
    case '/': state = KEY;      buf = '/';      break; // value is a name
    case '[': state = ARRAYVAL; buf = '[';      break; // value is an array
    default : state = VALUE;    buf = S;  break; // any other value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is now reading a value. It will do so until a special character
// is reached representing a new data type

void dict_builder::handleValue(char n)
{
  switch(n)
  {
    case '/': assignValue("/", KEY);            break; // end of value; new key
    case '<': assignValue("", QUERYDICT);       break; // may be new subdict
    case '>': assignValue("", QUERYCLOSE);      break; // probable end of dict
    case ' ': buf += ' ';                       break; // whitespace in value
    default : buf += S;                   break; // keep writing value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in an array. It will blindly copy the array until it gets
// to the matching closing bracket

void dict_builder::handleArrayval(char n)
{
  buf += S;
  if(n == ']') assignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer is now in a string; it blindly copies until finding a closing
// bracket.

void dict_builder::handleDstring(char n)
{
  buf += S;
  if(n == ')')
    assignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer has come across an angle bracket and needs to decide whether it
// is now in a subdictionary

void dict_builder::handleQuerydict(char n)
{
  if(n == '<') // now entering a subdictionary
  {
    buf = "<<";     // keep the angle brackets so a subdict is recognised later
    state = SUBDICT;
    bracket = 2;     // Record the nesting level to prevent early halting at >>
  }
  else
  {
    buf = "";
    state = START; // the single angle bracket wasn't recognised; start again
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in a subdictionary. It needs to know if it comes across
// further subdictionaries so it knows what level of nesting it is at.
// It does not otherwise process the subdictionary - the whole string can
// be used as the basis for a further dictionary object if required

void dict_builder::handleSubdict(char n)
{
  switch(n)
  {
    case '<': buf += S; bracket ++; break; // keep track of nesting
    case '>': buf += S; bracket --; break; // keep track of nesting
    default:  buf += S;             break; // keep on writing
  }
  if (bracket == 0)                              // now out of subdictionary
    assignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer has come out of the dictionary. It now checks whether a stream is
// present and if so records its position in the string used to create the
// dictionary object

void dict_builder::handleClose(char n)
{
  switch(n)
  {
    case ' ': state = CLOSE; break;      // ignore any whitespace.
    case 'L': if(i < (*s).length() - 7)  // is this a letter and is there enough
              {                          // space to contain the word "stream"?
                if((*s).substr(i, 6) == "stream") // OK, so is it "stream"?
                {
                  int ex = 7;
                  while (get_symbol_type((*s)[i + ex]) == ' ')
                    ex++; // read the whitespace characters after word "stream"
                  // Now store the location of the start of the stream
                  m_Map["stream"] = to_string(i + ex);
                }
              }
              state = THE_END; // stream or not, we are done
    default:  state = THE_END; // no stream, we are done
  }
}

/*---------------------------------------------------------------------------*/
// Creator function. Takes a string pointer so big strings can be passed
// cheaply. This version starts at the beginning of the given string

dict_builder::dict_builder(shared_ptr<const string> str) :
  s(str), i(0), bracket(0), keyPending(false), state(PREENTRY) // initializers
{
  if(s->empty()) *this = dict_builder(); // empty string -> empty dict
  tokenize_dict();
}

/*---------------------------------------------------------------------------*/
// Creator function that takes a string reference AND a starting position.
// This allows dictionaries to be read starting from the object locations
// given in the cross-reference (xref) table

dict_builder::dict_builder(shared_ptr<const string> str, size_t pos) :
  s(str), i(pos), bracket(0), keyPending(false), state(PREENTRY)
{
  // check string isn't empty or smaller than the starting position
  // if it is, return an empty dictionary
  if(s->empty() || (i >= s->length())) *this = dict_builder();
  else tokenize_dict();
}

/*---------------------------------------------------------------------------*/

unordered_map<string, string>&& dict_builder::get()
{
  return move(m_Map);
}

/*---------------------------------------------------------------------------*/
// Creator function for empty dictionary

dict_builder::dict_builder()
{
  unordered_map<string, string> Empty;
  m_Map = Empty;
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary(shared_ptr<const string> s)
{
  m_Map = move(dict_builder(s).get());
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary(shared_ptr<const string> s, size_t pos)
{
  m_Map = move(dict_builder(s, pos).get());
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary()
{
  unordered_map<string, string> Empty;
  m_Map = Empty;
}

/*---------------------------------------------------------------------------*/
// A dictionary can be created from an existing map. Not used but appears
// in case required for future feature development

Dictionary::Dictionary(std::unordered_map<string, string> dict)
{
  m_Map = dict;
};

/*---------------------------------------------------------------------------*/
// Simple getter of dictionary contents as a string from given key name

string Dictionary::get_string(const string& Key) const
{
  // A simple map index lookup with square brackets adds the key to
  // m_Map, which we don't want. Using find(key) leaves it unaltered
  auto g = m_Map.find(Key);
  if(g != m_Map.end())
    return g->second;
  // We want an empty string rather than an error if the key isn't found.
  // This allows functions that try to return references, ints, floats etc
  // to return an empty vector so a boolean test of their presence is
  // possible without calling the lexer twice.
  else return "";
}

/*---------------------------------------------------------------------------*/
// Sometimes we just need a boolean check for the presence of a key

bool Dictionary::has_key(const string& Key) const
{
  return m_Map.find(Key) != m_Map.end();
}

/*---------------------------------------------------------------------------*/
// We need to be able to check whether a key's value contains references.
// This should return true if the key is present AND its value contains
// at least one object reference, and should be false in all other cases

bool Dictionary::contains_references(const string& Key) const
{
  return !this->get_references(Key).empty();
}

/*---------------------------------------------------------------------------*/
// Check whether the key's values contains any integers. If a key is present
// AND its value contains ints, return true. Otherwise false.

bool Dictionary::contains_ints(const string& Key) const
{
  return !this->get_ints(Key).empty();
}

/*---------------------------------------------------------------------------*/
// Returns a vector of the object numbers from references found in the
// given key's value. Uses the getObjRefs() global function from utilities.h

vector<int> Dictionary::get_references(const string& Key) const
{
  return parse_references(this->get_string(Key));
}

/*---------------------------------------------------------------------------*/
// Returns a vector of the object numbers from references found in the
// given key's value. Uses the getObjRefs() global function from utilities.h

int Dictionary::get_reference(const string& Key) const
{
  vector<int> all_references = parse_references(this->get_string(Key));
  if(all_references.empty()) throw runtime_error("No reference found");
  return all_references[0];
}
/*---------------------------------------------------------------------------*/
// Returns any integers present in the value string as read by the parse_ints()
// global function defined in utilities.cpp

vector<int> Dictionary::get_ints(const string& Key) const
{
  return parse_ints(this->get_string(Key));
}

/*---------------------------------------------------------------------------*/
// Returns any floats present in the value string as read by the parse_floats()
// global function defined in utilities.cpp

vector<float> Dictionary::get_floats(const string& Key) const
{
  return parse_floats(this->get_string(Key));
}

/*---------------------------------------------------------------------------*/
// This creates a new dictionary object on request if the value string contains
// a subdictionary.

Dictionary Dictionary::get_dictionary(const string& Key) const
{
  // Get the value string
  string dict = this->get_string(Key);

  // Test that it is a dictionary
  if(dict.find("<<") != string::npos)
  {
    // If so, create a new dictionary
    return Dictionary(make_shared<string> (dict));
  }

  // Otherwise return an empty dictionary
  return Dictionary();
}

/*---------------------------------------------------------------------------*/
// Checks whether a subdictionary is present in the value string by looking
// for double angle brackets

bool Dictionary::contains_dictionary(const string& Key) const
{
  string dict = this->get_string(Key);
  return dict.find("<<") != string::npos;
}

/*---------------------------------------------------------------------------*/
// Returns all the keys present in the dictionary using the getKeys() template
// defined in utilities.cpp

vector<string> Dictionary::get_all_keys() const
{
  return getKeys(this->m_Map);
}

/*---------------------------------------------------------------------------*/
// Returns the entire map. This is useful for passing dictionaries out of
// the program, for example in debugging

const std::unordered_map<string, string>& Dictionary::R_out() const
{
  return this->m_Map;
}
