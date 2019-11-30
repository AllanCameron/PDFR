//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Dictionary implementation file                                      //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "dictionary.h"
#include<iostream>

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
 * The string which is fed in for parsing is read by the Reader class defined
 * in utilities.h. It is basically a string iterator with independent start
 * and stop positions which move according to the instructions given to it by
 * the dictionary builder and can output the intervening substring on request.
 *
 * The state is described by an enum, and the character of interest is tested
 * for its type - either letter, digit, whitespace or miscellaneous using the
 * GetSymbolType function defined in utilities.cpp. Any miscellaneous chars
 * that need to be handled by a specific state can be, because
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
 *
 * I have put the class declaration as well as the implementation of the
 * DictionaryBuilder in this file, as the DictionaryBuilder is only used to
 * implement Dictionary creation. It thus has no seperate user interface.
 */
//---------------------------------------------------------------------------//


class DictionaryBuilder
{
 public:
  using StringPointer = std::shared_ptr<const std::string>;

  DictionaryBuilder(StringPointer dictionary_string_ptr);
  DictionaryBuilder(StringPointer dictionary_string_ptr, size_t start_position);
  DictionaryBuilder();
  std::unordered_map<std::string, std::string>&& Get();

 private:
  enum DictionaryState  {PREENTRY, QUERYCLOSE, VALUE,    MAYBE,
                         START,    KEY,        PREVALUE, DSTRING,
                         ARRAYVAL, QUERYDICT,  SUBDICT,  CLOSE,   THE_END};

  int bracket_;               // Stores the nesting level of angle brackets
  bool key_pending_;          // flag that indicates a key name has been read
  Reader buf_;
  std::string pending_key_;   // name of key waiting for a value
  DictionaryState state_;     // current state of fsm
  std::unordered_map<std::string, std::string> map_; // data holder

  // Private functions
  void TokenizeDictionary_(); // co-ordinates the lexer
  void SetKey_(DictionaryState);          //----//
  void AssignValue_(DictionaryState);           //
  void HandleMaybe_();                          //
  void HandleStart_();                          //
  void HandleKey_();                            //
  void HandlePrevalue_();                       //--> functions to
  void HandleValue_();                          //    handle lexer states
  void HandleArrayValue_();                     //
  void HandleString_();                         //
  void HandleQueryDictionary_();                //
  void HandleSubdictionary_();                  //
  void HandleClose_();                    //----//

  // A couple of inlined functions to abbreviate common tasks
  char GetChar() const {return buf_.GetChar();}
  bool CharIs(char p_char) const {return buf_.GetChar() == p_char;}
};

/*---------------------------------------------------------------------------*/
// Constructor. Takes a string pointer so big strings can be passed
// cheaply. This version starts at the beginning of the given string

DictionaryBuilder::DictionaryBuilder(shared_ptr<const string> p_ptr)
  : bracket_(0), key_pending_(false), buf_(Reader(p_ptr)), state_(PREENTRY)
{
  // Empty string -> empty dictionary
  // Otherwise use the lexer to build the dictionary
  if (p_ptr->empty()) *this = DictionaryBuilder();
  else TokenizeDictionary_();
}

/*---------------------------------------------------------------------------*/
// Constructor that takes a string reference AND a starting position.
// This allows dictionaries to be read starting from the object locations
// given in the cross-reference (xref) table

DictionaryBuilder::DictionaryBuilder(StringPointer p_ptr, size_t p_offset)
  : bracket_(0), key_pending_(false),
    buf_(Reader(p_ptr, p_offset)), state_(PREENTRY)
{
  // Checks string isn't empty or smaller than the starting position
  // if it is, returns an empty dictionary
  if (p_ptr->empty()) *this = DictionaryBuilder();
  else TokenizeDictionary_();
}

/*---------------------------------------------------------------------------*/
// This is the main loop which iterates through the string, reads the char,
// finds its type and then runs the subroutine that deals with the current
// state of the machine.

void DictionaryBuilder::TokenizeDictionary_()
{
  // The lexer would go through an entire string without halting if it didn't
  // come across a dictionary. To prevent this in the event of a massive file,
  // set a limit on how far the lexer will read into a string
  size_t maxlen = std::min(buf_.first() + MAX_DICT_LEN, buf_.size()) ;

  // Main loop : read next char from string and pass to state handling function
  while (buf_.last() < maxlen)
  {
    // Now chooses the correct state handling function based on current state
    switch (state_)
    {
      case PREENTRY:    state_ = CharIs('<')? MAYBE : state_; break;
      case MAYBE:       HandleMaybe_();                       break;
      case START:       HandleStart_();                       break;
      case KEY:         HandleKey_();                         break;
      case PREVALUE:    HandlePrevalue_();                    break;
      case VALUE:       HandleValue_();                       break;
      case ARRAYVAL:    HandleArrayValue_();                  break;
      case DSTRING:     HandleString_();                      break;
      case QUERYDICT:   HandleQueryDictionary_();             break;
      case SUBDICT:     HandleSubdictionary_();               break;
      case QUERYCLOSE:  state_ = CharIs('>')? CLOSE : START;  break;
      case CLOSE:       HandleClose_();                       break;
      case THE_END:     return; // Stops the loop iff end state reached
    }
    ++buf_;  // This is a while loop - don't forget to increment.
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

void DictionaryBuilder::SetKey_(DictionaryState p_state)
{
  // If no key is awaiting value, store name as a key
  if (!key_pending_) pending_key_ = buf_.Contents();

  // Otherwise the name is a value so store it
  else map_[pending_key_] = buf_.Contents();

  // Flip the buffer flag in either case
  key_pending_ = !key_pending_;

  // Set buffer and state as needed
  buf_.Clear();
  state_  = p_state;
}

/*---------------------------------------------------------------------------*/
// The pattern of assigning a value to a waiting key name crops up often
// enough to warrant this function to reduce duplication and error

void DictionaryBuilder::AssignValue_(DictionaryState p_state)
{
  map_[pending_key_] = buf_.Contents(); // Contents of buffer assigned to key
  key_pending_ = false;                 // No key pending - ready for a new key

  // Update buffer and state with given parameters
  buf_.Clear();
  state_  = p_state;
}

/*---------------------------------------------------------------------------*/
// Handles the KEY state of the lexer. Reads characters and digits as a name
// along with a few other legal characters. Otherwise knows it is at the end
// of the name value and switches to the appropriate state depending on the
// next character

void DictionaryBuilder::HandleKey_()
{
  switch (GetSymbolType(GetChar()))
  {
    case '/': SetKey_(KEY);         break; // A new name has begun
    case ' ': SetKey_(PREVALUE);    break; // await next new value
    case '(': SetKey_(DSTRING);     break; // must be a string
    case '[': SetKey_(ARRAYVAL);    break; // must be an array
    case '<': SetKey_(QUERYDICT);   break; // probably a dictionary
    case '>': SetKey_(QUERYCLOSE);  break; // likely end of dict.
    default:                        break;
  }
}

/*---------------------------------------------------------------------------*/
// In this state, the lexer is waiting for the dictionary to start. It has
// just come across a '<' and knows it has encountered a dictionary if the
// next char is also a '<'. Otherwise it returns to a waiting state.

void DictionaryBuilder::HandleMaybe_()
{
  if (CharIs('<')) state_ = START;
  else
  {
    buf_.Clear();
    state_ = PREENTRY;
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just entered a dictionary. It should start with a key name,
// indicated by a '/'. If not, it will wait until it finds one or finds the
// end of the dictionary

void DictionaryBuilder::HandleStart_()
{
  buf_.Clear();
  switch (GetSymbolType(GetChar()))
  {
    case '/': state_ = KEY;                 break; // should always be so
    case '>': state_ = QUERYCLOSE;          break; // empty dictionary
    default : buf_.Clear();                 break; // linebreaks etc - wait
  }
}

/*---------------------------------------------------------------------------*/
// The lexer has just read a key name and now expects a value

void DictionaryBuilder::HandlePrevalue_()
{
  switch (GetSymbolType(GetChar()))
  {
    case ' ': state_ = PREVALUE;               break; // still waiting
    case '<': state_ = QUERYDICT;              break; // probable dict value
    case '>': state_ = QUERYCLOSE;             break; // ?end of dictionary
    case '(': SetKey_(DSTRING);                break; // must be a string
    case '/': state_ = KEY;      buf_.Clear(); break; // value is a name
    case '[': state_ = ARRAYVAL; buf_.Clear(); break; // value is an array
    default : state_ = VALUE;    buf_.Clear(); break; // any other value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is now reading a value. It will do so until a special character
// is reached representing a new data type

void DictionaryBuilder::HandleValue_()
{
  switch (GetSymbolType(GetChar()))
  {
    case '/': AssignValue_(KEY);        break; // end of value; new key
    case '<': AssignValue_(QUERYDICT);  break; // may be new subdict
    case '>': AssignValue_(QUERYCLOSE); break; // probable end of dict
    default :                           break; // keep writing value
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in an array. It will blindly copy the array until it gets
// to the matching closing bracket unless there is a closing bracket within
// a string to handle

void DictionaryBuilder::HandleArrayValue_()
{
  bool in_substring = false;
  bool escape_state = false;
  int depth = 1;

  while (depth)
  {
    if (!escape_state && CharIs('('))   in_substring = true;
    if (!escape_state && CharIs(')'))   in_substring = false;
    if (!in_substring && !escape_state && CharIs(']')) --depth;
    if (!in_substring && !escape_state && CharIs('[')) ++depth;
    if (!escape_state && CharIs('\\'))  escape_state = true;
    else escape_state = false;
    ++buf_;
  }

  AssignValue_(START);
  --buf_;
}

/*---------------------------------------------------------------------------*/
// The lexer is now in a string; it blindly copies until finding a closing
// bracket.

void DictionaryBuilder::HandleString_()
{
  if (CharIs('\\')) ++buf_;
  else if (CharIs(')')) {++buf_; AssignValue_(START); --buf_;}
}

/*---------------------------------------------------------------------------*/
// The lexer has come across an angle bracket and needs to decide whether it
// is now in a subdictionary

void DictionaryBuilder::HandleQueryDictionary_()
{
  if (CharIs('<'))  // Now entering a subdictionary
  {
    state_ = SUBDICT;
    bracket_ += 1;           // Record nesting level so exiting subdictionary
  }                         // doesn't cause early halting of parser
  else
  {
    buf_.Clear();
    state_ = START;         // Not a dictionary; start again
  }
}

/*---------------------------------------------------------------------------*/
// The lexer is in a subdictionary. It needs to know if it comes across
// further subdictionaries so it knows what level of nesting it is at.
// It does not otherwise process the subdictionary - the whole string can
// be used as the basis for a further dictionary object if required

void DictionaryBuilder::HandleSubdictionary_()
{
  switch (GetSymbolType(GetChar()))
  {
    case '<': ++buf_; if (CharIs('<')) ++bracket_;                  break;
    case '>': ++buf_; if (CharIs('>') && bracket_ > 0) --bracket_;  break;
    default:                                                        break;
  }

  // If bracket count falls to 0 we are out of the subdictionary
  if (bracket_ == 0) {++buf_; AssignValue_(START); --buf_;}
}

/*---------------------------------------------------------------------------*/
// The lexer has come out of the dictionary. It now checks whether a stream is
// present and if so records its position in the string used to create the
// dictionary object

void DictionaryBuilder::HandleClose_()
{
  buf_.Clear();
  switch (GetSymbolType(GetChar()))
  {
    // Ignore any whitespace.
    case ' ': state_ = CLOSE; break;

    // Is this a letter and is there enough space to contain the word "stream"?
    case 'L': if (buf_.size() - buf_.last() > 7)
              {
                // OK, so is it "stream"?
                if (buf_.StartsString("stream"))
                {
                  for(int i = 0; i < 6; ++i) ++buf_;
                  if (CharIs('\r')) ++buf_;
                  if (!CharIs('\n'))
                  {
                    throw runtime_error("Unexpected character before stream.");
                  }
                  ++buf_;
                  // Now store the location of the start of the stream
                  map_["stream"] = to_string(buf_.last());
                }
              }
              state_ = THE_END; // stream or not, we are done
    default:  state_ = THE_END; // no stream, we are done
  }
}

/*---------------------------------------------------------------------------*/
// Once the DictionaryBuilder has finished writing its map, we want to move it
// without copying into the Dictionary as its main data member

inline unordered_map<string, string>&& DictionaryBuilder::Get()
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
// The Dictionary constructor takes a string pointer and uses it to make its
// data member using a temporary DictionaryBuilder

Dictionary::Dictionary(shared_ptr<const string> p_string_ptr)
{
  map_ = move(DictionaryBuilder(p_string_ptr).Get());
}

/*---------------------------------------------------------------------------*/
// This alternative Dictionary constructor uses the same method as the normal
// constructor, but starts at a given offset in the supplied string.

Dictionary::Dictionary(shared_ptr<const string> p_string_ptr, size_t p_offset)
{
  map_ = move(DictionaryBuilder(p_string_ptr, p_offset).Get());
}

/*---------------------------------------------------------------------------*/
// Simple getter of dictionary contents as a string from given key name

string Dictionary::GetString(const string& p_key) const
{
  // A simple map index lookup with square brackets adds the key to
  // map_, which we don't want. Using find(key) leaves it unaltered
  auto finder = map_.find(p_key);
  if (finder != map_.end()) return finder->second;

  // We want an empty string rather than an error if the key isn't found.
  // This allows functions that try to return references, ints, floats etc
  // to return an empty vector so a boolean test of their presence is
  // possible without calling the lexer twice.
  return string();
}


/*---------------------------------------------------------------------------*/
// Returns a single object number from any reference found in the
// given key's value. Uses a global function from utilities.h

int Dictionary::GetReference(const string& p_key) const
{
  vector<int> all_references = ParseReferences(this->GetString(p_key));
  if (all_references.empty()) throw runtime_error("No reference found");
  return all_references[0];
}

/*---------------------------------------------------------------------------*/
// This creates a new dictionary object on request if the value string contains
// a subdictionary.

Dictionary Dictionary::GetDictionary(const string& p_key) const
{
  // Gets the value string
  string possible_sub_dictionary = this->GetString(p_key);

  // Tests that it is a dictionary
  if (possible_sub_dictionary.find("<<") != string::npos)
  {
    // If so, creates a new dictionary
    return Dictionary(make_shared<string> (possible_sub_dictionary));
  }

  // Otherwise returns an empty dictionary
  return Dictionary();
}

/*---------------------------------------------------------------------------*/
// Mainly for debugging. Prints all key:value pairs to the console

void Dictionary::PrettyPrint() const
{
  auto key_names = GetKeys(this->GetMap());
  for(auto key_name : key_names)
  {
    auto entry = this->GetString(key_name);
    cout << key_name << " : ";
    cout << entry;
    cout << endl;
  }
}
