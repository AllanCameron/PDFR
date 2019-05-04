//---------------------------------------------------------------------------//
//                                                                           //
// PDFR encoding implementation file                                         //
//                                                                           //
// Copyright (C) 2019 by Allan Cameron                                       //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining     //
// a copy of this software and associated documentation files                //
// (the "Software"), to deal in the Software without restriction, including  //
// without limitation the rights to use, copy, modify, merge, publish,       //
// distribute, sublicense, and/or sell copies of the Software, and to        //
// permit persons to whom the Software is furnished to do so, subject to     //
// the following conditions:                                                 //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.    //
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY      //
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,      //
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE         //
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                    //
//                                                                           //
//---------------------------------------------------------------------------//

#include "encoding.h"

using namespace std;

/*---------------------------------------------------------------------------*/
// The constructor function needs to do three things - get the encoding
// dictionary, get the base encoding, parse any /Differences entry, and parse
// any /ToUnicode entry. The first three are co-ordinated by read_encoding()
// and the last is co-ordinated by map_unicode()

Encoding::Encoding(const Dictionary& font_dictionary, shared_ptr<Document> doc):
  m_font_dictionary(font_dictionary), m_document(doc)
{
  read_encoding();
  map_unicode();
}

/*---------------------------------------------------------------------------*/
// Lexer / parser for /Differences entry of encoding dictionary. Takes the
// /Differences entry as a string and reads its ints as input code points. These
// are mapped to the subsequent glyph name's Unicode value. If more than one
// name follows an int, then sequential code points are mapped to each
// successive name's Unicode value. This requires the static adobe_to_unicode
// map.

void Encoding::read_differences(const string& differences_string)
{
  differences_state state = NEWSYMB; // define starting state
  string buffer {};        // initialise the buffer string

  // The entries vector gives a pair of type : entry for each entity pushed
  // onto the stack by the lexer. We therefore know whether we are dealing with
  // a code point or a name when we parse the stack
  vector<pair<differences_state, string>> entries;

  // The loop now cycles through each character in the differences string,
  // switching states and pushing results onto the stack according to the
  // type of character it encounters. This is quite a small lexer for a small
  // parsing task, so I have kept it as a single function (albeit one with
  // nested switch - case expressions)

  for(auto i : differences_string)
  {
    char n = get_symbol_type(i);  // determine character type
    switch(state)             // state switch
    {
      case NEWSYMB:   switch(n)   // if number switch to NUM, if / to NAME
                      {           // stop if closing bracket, else continue
                        case 'D': buffer = i; state = NUM;      break;
                        case '/': buffer = i; state = NAME;     break;
                        case ']': state = STOP;                 break;
                        default : buffer = "";                  break;
                      };
                      break;

      case NUM:       switch(n)   // write number to buffer, switch to name
                      {           // if char is '/'
                        case 'D': buffer += i ;                 break;
                        case '/': entries.push_back(make_pair(state, buffer));
                                  buffer = i; state = NAME;     break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  buffer = ""; state = NEWSYMB; break;
                      }
                      break;
      case NAME:      switch(n)   // write name unless space or slash etc
                      {
                        case 'L': buffer += i;                  break;
                        case '.': buffer += i;                  break;
                        case 'D': buffer += i;                  break;
                        case '/': entries.push_back(make_pair(state, buffer));
                                  buffer = i ;                  break;
                        case ' ': entries.push_back(make_pair(state, buffer));
                                  buffer = "" ; state = NEWSYMB;break;
                        default:  entries.push_back(make_pair(state, buffer));
                                  state = STOP;                 break;
                      }
                      break;
      default:        break;
    }
    if(state == STOP) break;
  }

  RawChar code_point = 0; // The raw code point to be mapped to Unicode

  // This loop writes the results vector to encoding map
  for(auto& entry : entries)
  {
    // If the vector entry is a number, convert to RawChar
    if(entry.first == NUM) code_point = (RawChar) stoi(entry.second);

    // Otherwise it's a name - convert it to Unicode, write it to the
    // encoding map and post-increment the mapping rawchar in case the next
    // entry is also a name (it will be over-written if it is an int)
    else m_encoding_map[code_point++] = adobe_to_unicode[entry.second];
  }
}

/*---------------------------------------------------------------------------*/
// co-ordinates the reading of the /ToUnicode entry, which points to an
// object whose stream contains a CMap

void Encoding::map_unicode()
{
  // If no /ToUnicode entry, nothing to be done
  if (!m_font_dictionary.contains_references("/ToUnicode")) return;

  // Otherwise, get the reference and get its stream
  int unicode_reference = m_font_dictionary.get_reference("/ToUnicode");

  // Get the text stream of the unicode conversion entry
  string unicode_text(m_document->get_object(unicode_reference)->get_stream());

  // multicarve gets all strings between the bookending strings. These are
  // stored in a vector and are substrates for the processing methods below
  auto bf_chars = multicarve(unicode_text, "beginbfchar", "endbfchar");
  auto bf_ranges = multicarve(unicode_text, "beginbfrange", "endbfrange");

  // if this is a character to character map, process it
  if (!bf_chars.empty())  process_unicode_chars(bf_chars);

  // if this is a character range, process it
  if (!bf_ranges.empty()) process_unicode_range(bf_ranges);
}

/*---------------------------------------------------------------------------*/
// This method parses the "bfchar" entries in the CMap and adds them to the
// encoding map

void Encoding::process_unicode_chars(vector<string>& bf_chars)
{
  // There may be many entries, so we process each of the given strings
  for(auto& entry : bf_chars)
  {
    // use multicarve() to get ascii-encoded byte representations
    vector<string> all_entries = multicarve(entry, "<", ">");

    // This loop takes the ascii-encoded bytes in the first column and
    // converts them to rawchar. It stores this number as 'key', then
    // converts the ascii-encoded bytes in the second column, converts them
    // to Unicode and makes them the raw key's mapped value in the encoding map
    for(size_t i = 0; i < (all_entries.size() - 1); i += 2)
    {
        RawChar key = convert_hex_to_rawchar(all_entries[i])[0];
        m_encoding_map[key] = convert_hex_to_rawchar(all_entries[i + 1])[0];
    }
  }
}

/*---------------------------------------------------------------------------*/
// This function parses the "bfrange" entries in the CMap and adds them to the
// encoding map. The bfrange comprises three numbers: the first code point to be
// translated in a range of code points, the last code point to be
// translated in the range, and the Unicode point from which to start the
// translation - hence { 1; 4; 10 } would generate {1,10; 2,11; 3,12; 4,13}

void Encoding::process_unicode_range(vector<string>& bf_ranges)
{
  // There may be many entries, so we process each of the given strings
  for(auto& ranges : bf_ranges)
  {
    // Uses multicarve() from utilities.h to get ascii-endoded byte strings
    auto all_entries = multicarve(ranges, "<", ">");

    // Sanity check - there should be at least three entries for a valid range
    if(all_entries.size() < 3) throw runtime_error("No entries in range");

    // Loop to calculate entries and fill encoding map
    for(size_t j = 2; j < all_entries.size(); j += 3)
    {
      // first column == first code point
      RawChar first = convert_hex_to_rawchar(all_entries[j - 2]).at(0);

      // second column == first code point
      RawChar last  = convert_hex_to_rawchar(all_entries[j - 1]).at(0);

      // We call the hex to rawchar function as it outputs uint16
      // However, 'start' is Unicode and we make this explicit
      Unicode start = (Unicode) convert_hex_to_rawchar(all_entries[j]).at(0);

      // Now we can fill the encoding map from the data in the row
      for(int increment = 0; increment <= (last - first); ++increment)
      {
        m_encoding_map[first + increment] = start + increment;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// Extract the encoding dictionary and read off entries for base encoding.
// Calls the Differences lexer if there is a dictionary entry for /Differences

void Encoding::read_encoding()
{
  // start with font dictionary
  Dictionary encoding_dictionary = m_font_dictionary;

  // Read encoding entry
  string encoding_name = encoding_dictionary.get_string("/Encoding");

  // If an encoding dictionary exists, get it and read the baseencoding entry
  if(m_font_dictionary.contains_references("/Encoding"))
  {
    auto encoding_object =
      m_document->get_object(m_font_dictionary.get_reference("/Encoding"));

    encoding_dictionary = encoding_object->get_dictionary();

    if(encoding_dictionary.has_key("/BaseEncoding"))
    {
      encoding_name = encoding_dictionary.get_string("/BaseEncoding");
    }
  }

  if( encoding_name == "/WinAnsiEncoding")
  {
    m_encoding_map = winansi_to_unicode;
  }
  else if(encoding_name == "/MacRomanEncoding")
  {
    m_encoding_map = macroman_to_unicode;
  }
  else if(encoding_name == "/PDFDocEncoding")
  {
    m_encoding_map = pdfdoc_to_unicode;
  }

  else for(RawChar i = 0; i < 256; ++i) m_encoding_map[i] = (Unicode) i;

  // Call Differences() if a /Differences entry is found to modify encoding
  if(encoding_dictionary.has_key("/Differences"))
  {
    m_base_encoding = encoding_dictionary.get_string("/Differences");
    read_differences(m_base_encoding);
  }
}

/*---------------------------------------------------------------------------*/
// This is a simple public interface to the underlying map data. It provides
// a range-checked lookup of a given RawChar. If it finds no Unicode entry
// in the map it returns the original RawChar

Unicode Encoding::interpret(const RawChar& raw)
{
  auto found = m_encoding_map.find(raw);
  if(found != m_encoding_map.end()) return found->second;

  // If no translation found, return the raw character code point
  else return raw;
}

/*---------------------------------------------------------------------------*/
// Public function to return a vector that enumerates all the keys in the main
// Rawchar - Unicode map

shared_ptr<unordered_map<RawChar, Unicode>> Encoding::encoding_keys()
{
  return make_shared<unordered_map<RawChar, Unicode>>(m_encoding_map);
}
