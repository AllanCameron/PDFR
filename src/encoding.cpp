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

#include "utilities.h"
#include "dictionary.h"
#include "object_class.h"
#include "document.h"
#include "encoding.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

/*---------------------------------------------------------------------------*/
// The constructor function needs to do three things - get the encoding
// dictionary, get the base encoding, parse any /Differences entry, and parse
// any /ToUnicode entry. The first three are co-ordinated by read_encoding()
// and the last is co-ordinated by map_unicode()

Encoding::Encoding(shared_ptr<Dictionary> p_font_dictionary,
                   shared_ptr<Document> p_document_ptr)
  : font_dictionary_(p_font_dictionary), document_(p_document_ptr)
{
  ReadEncoding_();
  MapUnicode_();
}

/*---------------------------------------------------------------------------*/

void Encoding::Write_(DifferencesState& p_state, string& p_buffer)
{
  entries_.push_back(make_pair(p_state, p_buffer));
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
// adobe_to_unicode_ map.

void Encoding::ReadDifferenceEntries_()
{

  RawChar code_point = 0; // The raw code point to be mapped to Unicode

   // This loop writes the results vector to encoding map
  for (auto& entry : entries_)
  {
    // If the vector entry is a number, convert to RawChar
    if (entry.first == NUM) code_point = (RawChar) stoi(entry.second);

    // Otherwise it's a name - convert it to Unicode, write it to the
    // encoding map and post-increment the mapping rawchar in case the next
    // entry is also a name (it will be over-written if it is an int)
    else
    {
      auto finder = adobe_to_unicode_.find(entry.second);
      if (finder != adobe_to_unicode_.end())
      {
        encoding_map_[code_point++] = adobe_to_unicode_.at(entry.second);
      }
      else
      {
        if (entry.second.find("/uni") == 0)
        {
          auto unicode_hex = entry.second.substr(4, 4);
          auto new_entry = ConvertHexToRawChar(unicode_hex);
          if (!new_entry.empty()) encoding_map_[code_point++] = new_entry[0];
          else encoding_map_[code_point] = code_point++;
        }
        // Some TrueType fonts have names given as "/Gxx" where xx is the byte
        // number. Ideally, I should read these entries from the font program
        // as /Differences should be ignored with TrueType fonts. However,
        // the examples I have come across have /Differences entries that
        // replicate the "cmap" table of the ttf file.
        else if (entry.second.substr(0, 2) == "/G")
        {
          string hex_string(entry.second.begin() + 2, entry.second.end());
          RawChar value = 0;
          int factor = 1;
          for(int i = (int) hex_string.size() - 1; i >= 0; --i)
          {
            if ((uint8_t) hex_string[i] < 0x3a &&
                (uint8_t) hex_string[i] > 0x2f)
            {
              value += factor * ((uint16_t)(uint8_t) hex_string[i] - 0x30);
              factor *= 16;
              continue;
            }
            if ((uint8_t) hex_string[i] < 0x47 &&
                (uint8_t) hex_string[i] > 0x40)
            {
              value += factor *  ((uint16_t)(uint8_t) hex_string[i] - 0x37);
              factor *= 16;
              continue;
            }
            if ((uint8_t) hex_string[i] < 0x67 &&
                (uint8_t) hex_string[i] > 0x60)
            {
              value += factor *  ((uint16_t)(uint8_t) hex_string[i] - 0x57);
              factor *= 16;
              continue;
            }
            value = 0;
            factor = 1;
          }
          if(value > 0) encoding_map_[code_point++] = value;
          else encoding_map_[code_point] = code_point++;
        }
        else encoding_map_[code_point] = code_point++;

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
  if (!font_dictionary_->ContainsReferences("/ToUnicode")) return;

  // Otherwise, get the reference and get its stream
  int unicode_reference = font_dictionary_->GetReference("/ToUnicode");

  // Get the text stream of the unicode conversion entry
  string unicode_text(document_->GetObject(unicode_reference)->GetStream());

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
        encoding_map_[key] = ConvertHexToRawChar(all_entries[i + 1])[0];
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
        encoding_map_[first + increment] = start + increment;
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
  Dictionary encoding_dictionary = *font_dictionary_;

  // Reads the encoding entry
  string encoding_name = encoding_dictionary.GetString("/Encoding");

  string subtype = encoding_dictionary.GetString("/Subtype");

  // If an encoding dictionary exists, gets it and read the baseencoding entry
  if (font_dictionary_->ContainsReferences("/Encoding"))
  {
    auto encoding_object_number = font_dictionary_->GetReference("/Encoding");
    auto encoding_object_ptr = document_->GetObject(encoding_object_number);
    encoding_dictionary = encoding_object_ptr->GetDictionary();
    if (encoding_dictionary.HasKey("/BaseEncoding"))
    {
      encoding_name = encoding_dictionary.GetString("/BaseEncoding");
    }
  }

  // Now we should have an encoding name to specify our encoding map
  if ( encoding_name == "/WinAnsiEncoding")
  {
    encoding_map_ = winansi_to_unicode_;
  }
  else if (encoding_name == "/MacRomanEncoding")
  {
    encoding_map_ = macroman_to_unicode_;
  }
  else if (encoding_name == "/PDFDocEncoding")
  {
    encoding_map_ = pdfdoc_to_unicode_;
  }

  // If no encoding name is specified, we take a direct RawChar : Unicode map
  else
  {
    for (RawChar raw_char = 0x0000; raw_char < 0x0100; ++raw_char)
    {
      encoding_map_[raw_char] = (Unicode) raw_char;
    }

    if(encoding_name == "" && subtype == "/Type1") HandleTypeOneFont_();
  }
  // Call Differences() if a /Differences entry is found to modify encoding
  if (encoding_dictionary.HasKey("/Differences"))
  {
    base_encoding_ = encoding_dictionary.GetString("/Differences");
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
  auto found = encoding_map_.find(p_raw);
  if (found != encoding_map_.end()) return found->second;
  else return p_raw;
}

/*---------------------------------------------------------------------------*/
// Public function to return a vector that enumerates all the keys in the main
// Rawchar - Unicode map

shared_ptr<unordered_map<RawChar, Unicode>> Encoding::GetEncodingKeys()
{
  return make_shared<unordered_map<RawChar, Unicode>>(encoding_map_);
}


/*---------------------------------------------------------------------------*/
// Handles cases of missing /Encoding entry where this is a type1 font

void Encoding::HandleTypeOneFont_()
{
  if(font_dictionary_->ContainsReferences("/FontDescriptor"))
  {
    auto descriptor_number = font_dictionary_->GetReference("/FontDescriptor");
    auto descriptor_object_ptr = document_->GetObject(descriptor_number);
    auto descriptor_dictionary = descriptor_object_ptr->GetDictionary();
    auto encoding_name = descriptor_dictionary.GetString("/Encoding");
    if(encoding_name == "")
    {
      if(descriptor_dictionary.ContainsReferences("/FontFile"))
      {
        auto fontfile_number = descriptor_dictionary.GetReference("/FontFile");
        auto fontfile_object_ptr = document_->GetObject(fontfile_number);
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
    if (adobe_to_unicode_.find(adobe_name) == adobe_to_unicode_.end())
    {
      string message = "Couldn't find " + adobe_name + " in adobe map";
      throw runtime_error(message);
    }
    encoding_map_[stoi(entry)] = adobe_to_unicode_.at(adobe_name);
  }
}
