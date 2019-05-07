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
 * GetSymbolType function defined in utilities.cpp. Any miscellaneous chars
 * that need to be handled by a specific state can be done so because
 * GetSymbolType returns the original char if it is not a letter, digit or
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

class DictionaryBuilder
{
 public:
  DictionaryBuilder(std::shared_ptr<const std::string>);
  DictionaryBuilder(std::shared_ptr<const std::string>, size_t);
  DictionaryBuilder();
  std::unordered_map<std::string, std::string>&& Get();

 private:
  enum DictionaryState   {PREENTRY,
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
  std::shared_ptr<const std::string> string_ptr_; // pointer to the string
  char char_;           // The actual character being read
  size_t char_num_; // the string's iterator which is passed between functions
  int bracket_;      // integer to store the nesting level of angle brackets
  bool key_pending_;  // flag that indicates a key name has been read
  std::string buffer_;  // string to hold characters in memory until needed
  std::string pending_key_; // name of key waiting for a value
  DictionaryState state_;     // current state of fsm
  std::unordered_map<std::string, std::string> map_; // data holder

  // Private functions
  void TokenizeDictionary(); // co-ordinates the lexer
  void SetKey(std::string, DictionaryState); //----//
  void AssignValue(std::string, DictionaryState);  //
  void HandleMaybe(char);                          //
  void HandleStart(char);                          //
  void HandleKey(char);                            //
  void HandlePrevalue(char);                       //--> functions to
  void HandleValue(char);                          //    handle lexer states
  void HandleArrayValue(char);                     //
  void HandleString(char);                         //
  void HandleQueryDictionary(char);                //
  void HandleSubdictionary(char);                  //
  void HandleClose(char);                    //----//
};

/*---------------------------------------------------------------------------*/
// This is the main loop which iterates through the string, reads the char,
// finds its type and then runs the subroutine that deals with the current
// state of the machine.

void DictionaryBuilder::TokenizeDictionary()
{
  // The lexer would go through an entire string without halting if it didn't
  // come across a dictionary. To prevent this in the event of a massive file,
  // set a limit on how far the lexer will read into a string
  size_t maxlen = char_num_ + MAX_DICT_LEN;

  // Main loop : read next char from string and pass to state handling function
  while (char_num_ < string_ptr_->length() && char_num_ < maxlen)
  {
    char_ = (*string_ptr_)[char_num_];

     // determine char type at start of each loop
    char input_char = GetSymbolType(char_);
    switch (state_)
    {
      case PREENTRY:    if (input_char == '<') state_ = MAYBE;  break;
      case MAYBE:       HandleMaybe(input_char);                break;
      case START:       HandleStart(input_char);                break;
      case KEY:         HandleKey(input_char);                  break;
      case PREVALUE:    HandlePrevalue(input_char);             break;
      case VALUE:       HandleValue(input_char);                break;
      case ARRAYVAL:    HandleArrayValue(input_char);           break;
      case DSTRING:     HandleString(input_char);               break;
      case QUERYDICT:   HandleQueryDictionary(input_char);      break;
      case SUBDICT:     HandleSubdictionary(input_char);        break;
      case QUERYCLOSE:  if (input_char == '>') state_ = CLOSE;
                        else state_ = START;                    break;
      case CLOSE:       HandleClose(input_char);                break;
      case THE_END:     return; // Stops the loop iff end state reached
    }

    // Don't forget to increment...
    ++char_num_;
  }
}

/*---------------------------------------------------------------------------*/
// Often, and confusingly, a pdf Name is given as the value to be stored.
// This function determines whether a pdf name is to be used as the key or the
// value of a key:value pair. It does this by reading whether a key is expected
// or not using the key pending flag. If there is a key waiting for a value, the
// key pending flag is true and this function knows to write the name as a value
// to the data map. Otherwise, it knows the name it has just read is intended
// as a key name. In either case the buffer is read and reset and the keypending
// flag is flipped. Although this code is short, it is efficient and used a lot
// so needs its own function.

void DictionaryBuilder::SetKey(string t_buffer, DictionaryState t_state)
{
  // If no key is awaiting value, store name as a key
  if(!key_pending_) pending_key_ = buffer_;

  // Otherwise the name is a value so store it
  else map_[pending_key_] = buffer_;

  // Flip the buffer flag in either case
  key_pending_ = !key_pending_;

  // Set buffer and state as needed
  buffer_ = t_buffer;
  state_  = t_state;
}

/*---------------------------------------------------------------------------*/
// The pattern of assigning a value to a waiting key name crops up often
// enough to warrant this function to reduce duplication and error

void DictionaryBuilder::AssignValue(string t_buffer, DictionaryState t_state)
{
  map_[pending_key_] = buffer_; // Contents of buffer assigned to key name
  key_pending_ = false;         // No key pending - ready for a new key

  // Update buffer and state with given parameters
  buffer_ = t_buffer;
  state_  = t_state;
}

/*---------------------------------------------------------------------------*/
// Handles the KEY state of the lexer. Reads characters and digits as a name
// along with a few other legal characters. Otherwise knows it is at the end
// of the name value and switches to the appropriate state depending on the
// next character

void DictionaryBuilder::HandleKey(char t_input_char)
{
  switch (t_input_char)
  {
    case 'L': buffer_ += char_;                 break;
    case 'D': buffer_ += char_;                 break;
    case '+': buffer_ += char_;                 break;
    case '-': buffer_ += char_;                 break;
    case '_': buffer_ += char_;                 break;
    case '/': SetKey("/",       KEY);          break; // A new name has begun
    case ' ': SetKey("",   PREVALUE);          break; // await next new value
    case '(': SetKey("(",   DSTRING);          break; // must be a string
    case '[': SetKey("[",  ARRAYVAL);          break; // must be an array
    case '<': SetKey("",  QUERYDICT);          break; // probably a dictionary
    case '>': SetKey("", QUERYCLOSE);          break; // likely end of dict.
  }
}

/*---------------------------------------------------------------------------*/
// In this state, the lexer is waiting for the dictionary to start. It has
// just come across a '<' and knows it has encountered a dictionary if the
// next char is also a '<'. Otherwise it returns to a waiting state.

void DictionaryBuilder::HandleMaybe(char t_input_char)
{
  if (t_input_char == '<')
  {
    state_ = START;
  }
  else
  {
    buffer_.clear();
    state_ = PREENTRY;
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just entered a dictionary. It should start with a key name,
// indicated by a '/'. If not, it will wait until it finds one or finds the
// end of the dictionary

void DictionaryBuilder::HandleStart(char t_input_char)
{
  switch (t_input_char)
  {
    case '/': buffer_ += '/'; state_ = KEY; break; // should always be so
    case '>': state_ = QUERYCLOSE;          break; // empty dictionary
    default :                               break; // linebreaks etc - wait
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just read a key name and now expects a value

void DictionaryBuilder::HandlePrevalue(char t_input_char)
{
  switch (t_input_char)
  {
    case ' ': state_ = PREVALUE;                  break; // still waiting
    case '<': state_ = QUERYDICT;                 break; // probable dict value
    case '>': state_ = QUERYCLOSE;                break; // ?end of dictionary
    case '/': state_ = KEY;      buffer_ = '/';   break; // value is a name
    case '[': state_ = ARRAYVAL; buffer_ = '[';   break; // value is an array
    default : state_ = VALUE;    buffer_ = char_; break; // any other value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is now reading a value. It will do so until a special character
// is reached representing a new data type

void DictionaryBuilder::HandleValue(char t_input_char)
{
  switch (t_input_char)
  {
    case '/': AssignValue("/", KEY);       break; // end of value; new key
    case '<': AssignValue("", QUERYDICT);  break; // may be new subdict
    case '>': AssignValue("", QUERYCLOSE); break; // probable end of dict
    case ' ': buffer_ += ' ';              break; // whitespace in value
    default : buffer_ += char_;            break; // keep writing value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in an array. It will blindly copy the array until it gets
// to the matching closing bracket

void DictionaryBuilder::HandleArrayValue(char t_input_char)
{
  buffer_ += char_;
  if (t_input_char == ']') AssignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer is now in a string; it blindly copies until finding a closing
// bracket.

void DictionaryBuilder::HandleString(char t_input_char)
{
  buffer_ += char_;
  if (t_input_char == ')') AssignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer has come across an angle bracket and needs to decide whether it
// is now in a subdictionary

void DictionaryBuilder::HandleQueryDictionary(char t_input_char)
{
  if (t_input_char == '<') // now entering a subdictionary
  {
    buffer_ = "<<"; // keep the angle brackets so a subdict is recognised later
    state_ = SUBDICT;
    bracket_ = 2;     // Record the nesting level to prevent early halting at >>
  }
  else
  {
    buffer_ = "";
    state_ = START; // the single angle bracket wasn't recognised; start again
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in a subdictionary. It needs to know if it comes across
// further subdictionaries so it knows what level of nesting it is at.
// It does not otherwise process the subdictionary - the whole string can
// be used as the basis for a further dictionary object if required

void DictionaryBuilder::HandleSubdictionary(char t_input_char)
{
  switch (t_input_char)
  {
    case '<': buffer_ += char_; bracket_ ++; break; // keep track of nesting
    case '>': buffer_ += char_; bracket_ --; break; // keep track of nesting
    default:  buffer_ += char_;              break; // keep on writing
  }

  // If bracket count falls to 0 we are out of the subdictionary
  if (bracket_ == 0) AssignValue("", START);
}

/*---------------------------------------------------------------------------*/
// The lexer has come out of the dictionary. It now checks whether a stream is
// present and if so records its position in the string used to create the
// dictionary object

void DictionaryBuilder::HandleClose(char t_input_char)
{
  switch (t_input_char)
  {
    // Ignore any whitespace.
    case ' ': state_ = CLOSE; break;

    // Is this a letter and is there enough space to contain the word "stream"?
    case 'L': if (char_num_ < string_ptr_->length() - 7)
              {
                 // OK, so is it "stream"?
                if(string_ptr_->substr(char_num_, 6) == "stream")
                {
                  int ex = 7;
                  while (GetSymbolType((*string_ptr_)[char_num_ + ex]) == ' ')
                    ex++; // read the whitespace characters after word "stream"
                  // Now store the location of the start of the stream
                  map_["stream"] = to_string(char_num_ + ex);
                }
              }
              state_ = THE_END; // stream or not, we are done
    default:  state_ = THE_END; // no stream, we are done
  }
}

/*---------------------------------------------------------------------------*/
// Constructor. Takes a string pointer so big strings can be passed
// cheaply. This version starts at the beginning of the given string

DictionaryBuilder::DictionaryBuilder(shared_ptr<const string> t_string_ptr) :
  string_ptr_(t_string_ptr),
  char_num_(0),
  bracket_(0),
  key_pending_(false),
  state_(PREENTRY)
{
  // Empty string -> empty dictionary
  if (string_ptr_->empty()) *this = DictionaryBuilder();

  // Otherwise use the lexer to build the dictionary
  TokenizeDictionary();
}

/*---------------------------------------------------------------------------*/
// Constructor that takes a string reference AND a starting position.
// This allows dictionaries to be read starting from the object locations
// given in the cross-reference (xref) table

DictionaryBuilder::DictionaryBuilder(shared_ptr<const string> t_string_ptr,
                                     size_t t_offset) :
  string_ptr_(t_string_ptr),
  char_num_(t_offset),
  bracket_(0),
  key_pending_(false),
  state_(PREENTRY)
{
  // check string isn't empty or smaller than the starting position
  // if it is, return an empty dictionary
  if (string_ptr_->empty() || (char_num_ >= string_ptr_->length()))
  {
    *this = DictionaryBuilder();
  }
  else
  {
    TokenizeDictionary();
  }
}

/*---------------------------------------------------------------------------*/

unordered_map<string, string>&& DictionaryBuilder::Get()
{
  return move(map_);
}

/*---------------------------------------------------------------------------*/
// Constructor for empty dictionary

DictionaryBuilder::DictionaryBuilder()
{
  unordered_map<string, string> Empty;
  map_ = Empty;
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary(shared_ptr<const string> t_string_ptr)
{
  map_ = move(DictionaryBuilder(t_string_ptr).Get());
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary(shared_ptr<const string> t_string_ptr, size_t t_offset)
{
  map_ = move(DictionaryBuilder(t_string_ptr, t_offset).Get());
}

/*---------------------------------------------------------------------------*/

Dictionary::Dictionary()
{
  unordered_map<string, string> Empty;
  map_ = Empty;
}

/*---------------------------------------------------------------------------*/
// A dictionary can be created from an existing map. Not used but appears
// in case required for future feature development

Dictionary::Dictionary(std::unordered_map<string, string> t_map)
{
  map_ = t_map;
};

/*---------------------------------------------------------------------------*/
// Simple getter of dictionary contents as a string from given key name

string Dictionary::GetString(const string& t_key) const
{
  // A simple map index lookup with square brackets adds the key to
  // map_, which we don't want. Using find(key) leaves it unaltered
  auto finder = map_.find(t_key);
  if (finder != map_.end()) return finder->second;

  // We want an empty string rather than an error if the key isn't found.
  // This allows functions that try to return references, ints, floats etc
  // to return an empty vector so a boolean test of their presence is
  // possible without calling the lexer twice.
  return string();
}

/*---------------------------------------------------------------------------*/
// Sometimes we just need a boolean check for the presence of a key

bool Dictionary::HasKey(const string& t_key) const
{
  return map_.find(t_key) != map_.end();
}

/*---------------------------------------------------------------------------*/
// We need to be able to check whether a key's value contains references.
// This should return true if the key is present AND its value contains
// at least one object reference, and should be false in all other cases

bool Dictionary::ContainsReferences(const string& t_key) const
{
  return !this->GetReferences(t_key).empty();
}

/*---------------------------------------------------------------------------*/
// Check whether the key's values contains any integers. If a key is present
// AND its value contains ints, return true. Otherwise false.

bool Dictionary::ContainsInts(const string& t_key) const
{
  return !this->GetInts(t_key).empty();
}

/*---------------------------------------------------------------------------*/
// Returns a vector of the object numbers from references found in the
// given key's value. Uses the getObjRefs() global function from utilities.h

vector<int> Dictionary::GetReferences(const string& t_key) const
{
  return ParseReferences(this->GetString(t_key));
}

/*---------------------------------------------------------------------------*/
// Returns a vector of the object numbers from references found in the
// given key's value. Uses the getObjRefs() global function from utilities.h

int Dictionary::GetReference(const string& t_key) const
{
  vector<int> all_references = ParseReferences(this->GetString(t_key));
  if (all_references.empty()) throw runtime_error("No reference found");
  return all_references[0];
}
/*---------------------------------------------------------------------------*/
// Returns any integers present in the value string as read by the ParseInts()
// global function defined in utilities.cpp

vector<int> Dictionary::GetInts(const string& t_key) const
{
  return ParseInts(this->GetString(t_key));
}

/*---------------------------------------------------------------------------*/
// Returns any floats present in the value string as read by the ParseFloats()
// global function defined in utilities.cpp

vector<float> Dictionary::GetFloats(const string& t_key) const
{
  return ParseFloats(this->GetString(t_key));
}

/*---------------------------------------------------------------------------*/
// This creates a new dictionary object on request if the value string contains
// a subdictionary.

Dictionary Dictionary::GetDictionary(const string& t_key) const
{
  // Get the value string
  string dict = this->GetString(t_key);

  // Test that it is a dictionary
  if (dict.find("<<") != string::npos)
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

bool Dictionary::ContainsDictionary(const string& t_key) const
{
  string dict = this->GetString(t_key);
  return dict.find("<<") != string::npos;
}

/*---------------------------------------------------------------------------*/
// Returns all the keys present in the dictionary using the getKeys() template
// defined in utilities.cpp

vector<string> Dictionary::GetAllKeys() const
{
  return GetKeys(this->map_);
}

/*---------------------------------------------------------------------------*/
// Returns the entire map. This is useful for passing dictionaries out of
// the program, for example in debugging

std::unordered_map<string, string> Dictionary::GetMap() const
{
  return this->map_;
}
