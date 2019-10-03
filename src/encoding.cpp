//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Encoding implementation file                                        //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "encoding.h"

using namespace std;

/*---------------------------------------------------------------------------*/
// The constructor function needs to do three things - get the encoding
// dictionary, get the base encoding, parse any /Differences entry, and parse
// any /ToUnicode entry. The first three are co-ordinated by read_encoding()
// and the last is co-ordinated by map_unicode()

Encoding::Encoding(shared_ptr<Dictionary> p_font_dictionary,
                   shared_ptr<Document> p_document_ptr)
  : m_font_dictionary(p_font_dictionary), m_document_ptr(p_document_ptr)
{
  ReadEncoding_();
  MapUnicode_();
}

/*---------------------------------------------------------------------------*/

void Encoding::Write_(DifferencesState& p_state, string& p_buffer)
{
  m_entries.push_back(make_pair(p_state, p_buffer));
}

/*---------------------------------------------------------------------------*/
// Lexer for /Differences entry of encoding dictionary. Takes the
// /Differences entry as a string and reads its ints as input code points.

void Encoding::ReadDifferences_(const string& p_differences_string)
{
  DifferencesState state = NEWSYMB; // define starting state
  string buffer {};        // initialise the buffer string

  // The loop now cycles through each character in the differences string,
  // switching states and pushing results onto the stack according to the
  // type of character it encounters. This is quite a small lexer for a small
  // parsing task, so I have kept it as a single function (albeit one with
  // nested switch - case expressions)

  for (auto i : p_differences_string)
  {
    char n = GetSymbolType(i);  // determine character type
    switch (state)             // state switch
    {
      case NEWSYMB:   switch (n)   // if number switch to NUM, if / to NAME
                      {           // stop if closing bracket, else continue
                        case 'D': buffer = i; state = NUM;       break;
                        case '/': buffer = i; state = NAME;      break;
                        case ']': state = STOP;                  break;
                        default : buffer = "";                   break;
                      };
                      break;

      case NUM:       switch (n)   // write number to buffer, switch to name
                      {           // if char is '/'
                        case 'D': buffer += i ;                  break;
                        case '/': Write_(state, buffer);
                                  buffer = i; state = NAME;      break;
                        default:  Write_(state, buffer);
                                  buffer = ""; state = NEWSYMB;  break;
                      }
                      break;
      case NAME:      switch (n)   // write name unless space or slash etc
                      {
                        case 'L': buffer += i;                   break;
                        case '.': buffer += i;                   break;
                        case 'D': buffer += i;                   break;
                        case '/': Write_(state, buffer);
                                  buffer = i ;                   break;
                        case ' ': Write_(state, buffer);
                                  buffer = "" ; state = NEWSYMB; break;
                        default:  Write_(state, buffer);
                                  state = STOP;                  break;
                      }
                      break;
      default:        break;
    }
    if (state == STOP) break;
  }
  ReadDifferenceEntries_();
}

/*---------------------------------------------------------------------------*/
// Parser for /Differences entry. Maps char points to glyph name's Unicode
// value. If more than one name follows an int, then sequential code points are
// mapped to each successive name's Unicode value. This requires the static
// sm_adobe_to_unicode map.

void Encoding::ReadDifferenceEntries_()
{

  RawChar code_point = 0; // The raw code point to be mapped to Unicode

   // This loop writes the results vector to encoding map
  for (auto& entry : m_entries)
  {
    // If the vector entry is a number, convert to RawChar
    if (entry.first == NUM) code_point = (RawChar) stoi(entry.second);

    // Otherwise it's a name - convert it to Unicode, write it to the
    // encoding map and post-increment the mapping rawchar in case the next
    // entry is also a name (it will be over-written if it is an int)
    else
    {
      auto finder = sm_adobe_to_unicode.find(entry.second);
      if (finder != sm_adobe_to_unicode.end())
      {
        m_encoding_map[code_point++] = sm_adobe_to_unicode.at(entry.second);
      }
      else
      {
        if (entry.second.find("/uni") == 0)
        {
          auto unicode_hex = entry.second.substr(4, 4);
          auto new_entry = ConvertHexToRawChar(unicode_hex);
          if (!new_entry.empty()) m_encoding_map[code_point++] = new_entry[0];
          else m_encoding_map[code_point] = code_point++;
        }
        else m_encoding_map[code_point] = code_point++;

      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// co-ordinates the reading of the /ToUnicode entry, which points to an
// object whose stream contains a CMap

void Encoding::MapUnicode_()
{
  // If no /ToUnicode entry, nothing to be done
  if (!m_font_dictionary->ContainsReferences_("/ToUnicode")) return;

  // Otherwise, get the reference and get its stream
  int uni_reference = m_font_dictionary->GetReference_("/ToUnicode");

  // Get the text stream of the unicode conversion entry
  string unicode_text(m_document_ptr->GetObject(uni_reference)->GetStream());

  // MultiCarve gets all strings between the bookending strings. These are
  // stored in a vector and are substrates for the processing methods below
  auto bf_chars = MultiCarve(unicode_text, "beginbfchar", "endbfchar");
  auto bf_ranges = MultiCarve(unicode_text, "beginbfrange", "endbfrange");

  // if this is a character to character map, process it
  if (!bf_chars.empty())  ProcessUnicodeChars_(bf_chars);

  // if this is a character range, process it
  if (!bf_ranges.empty()) ProcessUnicodeRange_(bf_ranges);
}

/*---------------------------------------------------------------------------*/
// This method parses the "bfchar" entries in the CMap and adds them to the
// encoding map

void Encoding::ProcessUnicodeChars_(vector<string>& p_bf_chars)
{
  // There may be many entries, so we process each of the given strings
  for (auto& entry : p_bf_chars)
  {
    // use MultiCarve() to get ascii-encoded byte representations
    vector<string> all_entries = MultiCarve(entry, "<", ">");

    // This loop takes the ascii-encoded bytes in the first column and
    // converts them to rawchar. It stores this number as 'key', then
    // converts the ascii-encoded bytes in the second column, converts them
    // to Unicode and makes them the raw key's mapped value in the encoding map
    for (size_t i = 0; i < (all_entries.size() - 1); i += 2)
    {
        RawChar key = ConvertHexToRawChar(all_entries[i])[0];
        m_encoding_map[key] = ConvertHexToRawChar(all_entries[i + 1])[0];
    }
  }
}

/*---------------------------------------------------------------------------*/
// This function parses the "bfrange" entries in the CMap and adds them to the
// encoding map. The bfrange comprises three numbers: the first code point to be
// translated in a range of code points, the last code point to be
// translated in the range, and the Unicode point from which to start the
// translation - hence { 1; 4; 10 } would generate {1,10; 2,11; 3,12; 4,13}

void Encoding::ProcessUnicodeRange_(vector<string>& p_bf_ranges)
{
  // There may be many entries, so we process each of the given strings
  for (auto& ranges : p_bf_ranges)
  {
    // Uses MultiCarve() from utilities.h to get ascii-endoded byte strings
    auto all_entries = MultiCarve(ranges, "<", ">");

    // Sanity check - there should be at least three entries for a valid range
    if (all_entries.size() < 3) throw runtime_error("No entries in range");

    // Loop to calculate entries and fill encoding map
    for (size_t j = 2; j < all_entries.size(); j += 3)
    {
      // first column == first code point; second column == last code point
      RawChar first = ConvertHexToRawChar(all_entries[j - 2]).at(0);
      RawChar last  = ConvertHexToRawChar(all_entries[j - 1]).at(0);

      // We call the ConvertHexToRawchar function as it outputs uint16
      // However, 'start' is Unicode and we make this explicit
      Unicode start = (Unicode) ConvertHexToRawChar(all_entries[j]).at(0);

      // Now we can fill the encoding map from the data in the row
      for (int increment = 0; increment <= (last - first); ++increment)
      {
        m_encoding_map[first + increment] = start + increment;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// Extract the encoding dictionary and read off entries for base encoding.
// Calls the Differences lexer if there is a dictionary entry for /Differences

void Encoding::ReadEncoding_()
{
  // Starts with private font dictionary member
  Dictionary encoding_dictionary = *m_font_dictionary;

  // Reads the encoding entry
  string encoding_name = encoding_dictionary.GetString_("/Encoding");

  string subtype = encoding_dictionary.GetString_("/Subtype");

  // If an encoding dictionary exists, gets it and read the baseencoding entry
  if (m_font_dictionary->ContainsReferences_("/Encoding"))
  {
    auto encoding_reference = m_font_dictionary->GetReference_("/Encoding");
    auto encoding_object_ptr = m_document_ptr->GetObject(encoding_reference);
    encoding_dictionary = encoding_object_ptr->GetDictionary();
    if (encoding_dictionary.HasKey_("/BaseEncoding"))
    {
      encoding_name = encoding_dictionary.GetString_("/BaseEncoding");
    }
  }

  // Now we should have an encoding name to specify our encoding map
  if ( encoding_name == "/WinAnsiEncoding")
  {
    m_encoding_map = sm_winansi_to_unicode;
  }
  else if (encoding_name == "/MacRomanEncoding")
  {
    m_encoding_map = sm_macroman_to_unicode;
  }
  else if (encoding_name == "/PDFDocEncoding")
  {
    m_encoding_map = sm_pdfdoc_to_unicode;
  }

  // If no encoding name is specified, we take a direct RawChar : Unicode map
  else
  {
    for (RawChar raw_char = 0x0000; raw_char < 0x0100; ++raw_char)
    {
      m_encoding_map[raw_char] = (Unicode) raw_char;
    }

    if(encoding_name == "" && subtype == "/Type1") HandleTypeOneFont_();
  }
  // Call Differences() if a /Differences entry is found to modify encoding
  if (encoding_dictionary.HasKey_("/Differences"))
  {
    base_encoding_ = encoding_dictionary.GetString_("/Differences");
    ReadDifferences_(base_encoding_);
  }
}

/*---------------------------------------------------------------------------*/
// This is a simple public interface to the underlying map data. It provides
// a range-checked lookup of a given RawChar. If it finds no Unicode entry
// in the map it returns the original RawChar

Unicode Encoding::Interpret(const RawChar& p_raw)
{
  // If no translation found, return the raw character code point
  auto found = m_encoding_map.find(p_raw);
  if (found != m_encoding_map.end()) return found->second;
  else return p_raw;
}

/*---------------------------------------------------------------------------*/
// Public function to return a vector that enumerates all the keys in the main
// Rawchar - Unicode map

shared_ptr<unordered_map<RawChar, Unicode>> Encoding::GetEncodingKeys()
{
  return make_shared<unordered_map<RawChar, Unicode>>(m_encoding_map);
}


/*---------------------------------------------------------------------------*/
// Handles cases of missing /Encoding entry where this is a type1 font

void Encoding::HandleTypeOneFont_()
{
  if(m_font_dictionary->ContainsReferences_("/FontDescriptor"))
  {
    auto descript_number = m_font_dictionary->GetReference_("/FontDescriptor");
    auto descriptor_object_ptr = m_document_ptr->GetObject(descript_number);
    auto descriptor_dictionary = descriptor_object_ptr->GetDictionary();
    auto encoding_name = descriptor_dictionary.GetString_("/Encoding");
    if(encoding_name == "")
    {
      if(descriptor_dictionary.ContainsReferences_("/FontFile"))
      {
        auto fontfile_number = descriptor_dictionary.GetReference_("/FontFile");
        auto fontfile_object_ptr = m_document_ptr->GetObject(fontfile_number);
        auto fontfile_string = fontfile_object_ptr->GetStream();
        ParseTypeOneFont_(fontfile_string);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// Parses a type 1 font file

void Encoding::ParseTypeOneFont_(std::string p_fontfile_string)
{
  string the_end =  "currentdict end";
  auto fontfile_listing = CarveOut(p_fontfile_string, "/Encoding", the_end);
  auto entries = MultiCarve(fontfile_listing, "dup ", " put");
  for(auto entry : entries)
  {
    auto name_start  = entry.find("/");
    auto adobe_name = entry.substr(name_start, entry.size() - name_start);
    if (sm_adobe_to_unicode.find(adobe_name) == sm_adobe_to_unicode.end())
    {
      string message = "Couldn't find " + adobe_name + " in adobe map";
      throw runtime_error(message);
    }
    m_encoding_map[stoi(entry)] = sm_adobe_to_unicode.at(adobe_name);
  }
}
