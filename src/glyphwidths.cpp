//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GlyphWidths implementation file                                     //
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
#include "glyphwidths.h"

//---------------------------------------------------------------------------//
// Where a glyph's width in missing and there is no default width, we need a
// "default" default width. Set as a macro for easy changing and to avoid
// a "magic constant" in the program.

#define DEFAULT_WIDTH 500;

//---------------------------------------------------------------------------//

using namespace std;

/*---------------------------------------------------------------------------*/
// Constructor taking a reference to the main font dictionary and a pointer to
// the containing document. If the font is a core font, get the widths from
// built in static corefont tables. Otherwise find and interpret widths.

GlyphWidths::GlyphWidths
  (shared_ptr<Dictionary> t_font_dictionary, shared_ptr<Document> t_document_ptr)
  : font_dictionary_(t_font_dictionary),
    document_(t_document_ptr),
    base_font_(font_dictionary_->GetString("/BaseFont"))
{
  ReadCoreFont_();
  if (width_map_.empty()) ReadWidthTable_();
}

/*---------------------------------------------------------------------------*/
// The two main ways to get glyph widths from a font's dictionary are directly
// under the /Widths entry, or under the /DescendantFonts dictionary. This
// method calls the appropriate parser depending on the entries in the font
// dictionary

void GlyphWidths::ReadWidthTable_()
{
  // If widths entry specified, use this by calling parsewidths method
  if (font_dictionary_->HasKey("/Widths"))
  {
    ParseWidths_();
  }

  // otherwise look in descendants using parseDescendants method
  else if (font_dictionary_->ContainsReferences("/DescendantFonts"))
  {
    ParseDescendants_();
  }

  // otherwise we have no font widths specified and need to use default values
}

/*---------------------------------------------------------------------------*/
// This method is only called when a /Widths entry is found in the font
// dictionary. It looks for a /FirstChar entry which specifies the code point
// to which the first width in the array applies. The rest of the array then
// refers to sequential code points after this.

void GlyphWidths::ParseWidths_()
{
  // Usually widths given as ints, but can be floats
  vector<float> width_array;

  // If the font dictionary contains no /Firstchar, we'll default to zero
  RawChar first_character = 0x0000;

  // Otherwise we read the firstchar entry
  if (font_dictionary_->ContainsInts("/FirstChar"))
  {
    first_character = font_dictionary_->GetInts("/FirstChar")[0];
  }
  // Annoyingly, widths sometimes contains a pointer to another object that
  // contains the width array, either in a stream or as a 'naked object'.
  // Note that contents of a naked object are stored as the object's 'stream'.

  // Handle /widths being a reference to another object
  if (font_dictionary_->ContainsReferences("/Widths"))
  {
    auto width_object_number = font_dictionary_->GetReference("/Widths");
    auto width_object_ptr = document_->GetObject(width_object_number);

    // Get the referenced object's stream
    string width_object_stream(width_object_ptr->GetStream());

    // Get the numbers from the width array in the stream
    width_array = ParseFloats(width_object_stream);
  }
  // If /Widths is not a reference get the widths directly
  else  width_array = font_dictionary_->GetFloats("/Widths");

  // If a width array was found
  if (!width_array.empty())
  {
    // The widths represent post-Unicode translation widths
    this->width_is_pre_interpretation_ = true;

    // Now we can fill the width map from the width array
    for (size_t index = 0; index < width_array.size(); ++index)
    {
      width_map_[first_character + index] = (int) width_array[index];
    }
  }
}

/*---------------------------------------------------------------------------*/
// If the font is is CIDKeyed (type 0) font, it will inherit from a descendant
// font with its own object dictionary. This should contain a /W entry that is
// an array of widths for given ranges of code points and needs to be
// interpreted by its own lexer, also included as a method in this class

void GlyphWidths::ParseDescendants_()
{
  // get a pointer to the /Descendantfonts object
  auto descendant_number = font_dictionary_->GetReference("/DescendantFonts");
  auto descendant_object = document_->GetObject(descendant_number);

  // Extract its dictionary and its stream
  Dictionary descendant_dictionary = descendant_object->GetDictionary();
  string descendant_stream(descendant_object->GetStream());

  // Handle /Descendantfonts being just a reference to another object
  if (!ParseReferences(descendant_stream).empty())
  {
    descendant_number = ParseReferences(descendant_stream)[0];
    descendant_object = document_->GetObject(descendant_number);
    descendant_dictionary = descendant_object->GetDictionary();
  }

  // We now look for the /W key and if it is found parse its contents
  if (descendant_dictionary.HasKey("/W"))
  {
    // We will fill this string with width array when found
    string width_string;

    // sometimes the /W entry only contains a pointer to the containing object
    if (descendant_dictionary.ContainsReferences("/W"))
    {
      auto width_object_number = descendant_dictionary.GetReference("/W");
      width_string = document_->GetObject(width_object_number)->GetStream();
    }

    // otherwise we assume /W contains the widths needed
    else width_string = descendant_dictionary.GetString("/W");

    // in either case width_string should now contain the /W array which we
    // now need to parse using our lexer method
    ParseWidthArray_(width_string);

    // The widths obtained apply to the RawChars, not to post-conversion Unicode
    this->width_is_pre_interpretation_ = true;
  }
}

/*---------------------------------------------------------------------------*/
// The constructor includes a string passed from the "BaseFont" entry
// of the encoding dictionary.

void GlyphWidths::ReadCoreFont_()
{
  // If the font is one of the 14 core fonts, the widths are already specified.
  // Note that these widths represent the widths of the actual Unicode glyphs
  // so any encoding differences should take place before the widths are
  // interpreted. This is not the case where /Differences or a specific
  // /Width map is included - in these cases the widths refer to the glyphs
  // that will result from the given RawChar codes. This is therefore flagged
  // by the boolean widthsFromCharCodes.

  if (base_font_.find("/Courier") != string::npos) width_map_ = courier_widths_;
  else if (base_font_ == "/Symbol") width_map_ = symbol_widths_;
  else if (base_font_ == "/Times-Bold") width_map_ = times_bold_widths_;
  else if (base_font_ == "/Times-Italic") width_map_ = times_italic_widths_;
  else if (base_font_ == "/Times-Roman") width_map_ = times_roman_widths_;
  else if (base_font_ == "/ZapfDingbats") width_map_ = dingbats_widths_;
  else if (base_font_ == "/Helvetica-Oblique" ||
           base_font_ == "/Helvetica") width_map_ = helvetica_widths_;
  else if (base_font_ == "/Helvetica-Boldoblique" ||
           base_font_ == "/Helvetica-Bold") width_map_ = helvetica_bold_widths_;
  else if (base_font_ == "/Times-BoldItalic")
    width_map_ = times_bold_italic_widths_;

  // No unicode -> width mapping - using RawChar
  else width_is_pre_interpretation_ = true;
}

/*---------------------------------------------------------------------------*/
// Getter. Finds the width for a given character code. If it is not specified
// returns the default width specified in the macro at the top of this file

int GlyphWidths::GetWidth(const RawChar& raw)
{
  // Look up the supplied rawChar
  auto found = width_map_.find(raw);
  if (found != width_map_.end()) return found->second;

  // No width found - return the default width
  else return DEFAULT_WIDTH;
}

/*---------------------------------------------------------------------------*/
// Simple public getter function that returns the mapped character codes as
// a vector without their associated widths

vector<RawChar> GlyphWidths::WidthKeys()
{
  return GetKeys(this->width_map_);
}

/*---------------------------------------------------------------------------*/
// Yet another lexer. This one is specialised to read the "/W" entry of type0
// fonts. These are an array containing arrays of widths. Each sub-array
// is preceeded by the code point to which the first width in the sub-array
// applies, after which the widths apply to consecutive values after the first
// code point. Hence the string "[3[100 200 150] 10[250 300]]" should be
// interpreted as mapping {{3, 100}, {4, 200}, {5, 150}, {10, 250}, {11, 300}}

void GlyphWidths::ParseWidthArray_(const string& t_width_string)
{
  // If the width string is empty, there's nothing to be done
  if (t_width_string.empty()) return;

  // These variables maintain state during the lexer process:
  WidthState state = NEWSYMB; // Uses enum to keep track of state of lexer
  string buffer {};           // Stores strings waiting to be turned into ints
  vector<int> number_buffer,  // Stores ints until we know what they are for
              first_chars;    // A vector of the starting code points for widths
  vector<vector<int>> result; // Each first_char has an int vector of widths

  // Main loop - Iterates through all the characters in t_width_string
  for (const auto& current_char : t_width_string)
  {
    // If opening of array not first character, simply wait for '['
    if (state == NEWSYMB)
    {
      if (current_char == '[') state = INARRAY;
      continue;
    }

    // Handle the main array. Either read a character code or find a subarray
    if (state == INARRAY)
    {
      switch (GetSymbolType(current_char))
      {
      case 'D' : buffer += current_char; break;

      case '[' : state = INSUBARRAY;
                 if (!buffer.empty())
                 {
                   number_buffer.push_back(stoi(buffer));
                   if (number_buffer.size() == 1)
                     first_chars.push_back(number_buffer[0]);
                   else
                     result.push_back(number_buffer);
                 }
                 buffer.clear();
                 number_buffer.clear();
                 break;

      case ' ' : if (!buffer.empty())
                   number_buffer.push_back(stoi(buffer));
                buffer.clear(); break;

      case ']': state = END;
                if (!buffer.empty())
                {
                  number_buffer.push_back(stoi(buffer));
                  if (number_buffer.size() == 1)
                    first_chars.push_back(number_buffer[0]);
                  else result.push_back(number_buffer);
                }
                buffer.clear();
                number_buffer.clear();
                break;

      default: throw (string("Error parsing string ") + t_width_string);
      }
      continue;
    }

    // Handle the insubarray state: read numbers as a vector of widths
    if (state == INSUBARRAY)
    {
      switch (GetSymbolType(current_char))
      {
      case ' ': if (!buffer.empty()) number_buffer.push_back(stoi(buffer));
                buffer.clear();
                break;

      case ']': state = INARRAY;  // exited from subarray
                if (!buffer.empty()) number_buffer.push_back(stoi(buffer));
                result.push_back(number_buffer);
                number_buffer.clear();
                buffer.clear(); break;

      case 'D': buffer += current_char; break; // read actual width number

      default: throw (string("Error parsing string ") + t_width_string);
      }

      continue;
    }

    if (state == END) break;
  }

  // We now parse the results of the lexer.
  // First we make sure that the starting character codes are equal in length
  // to the number of width arrays, and that neither is empty
  if ((first_chars.size() == result.size()) && !first_chars.empty() )
  {
    // Now loops through the vectors and marries char codes to widths
    for (size_t i = 0; i < first_chars.size(); ++i)
    {
      // Skips any character code that doesn't have an associated width array
      if (!result[i].empty())
      {
        // Now for each member of the width array...
        for (size_t j = 0; j < result[i].size(); j++)
        {
          // ...map sequential values of char codes to stated widths
          width_map_[(RawChar) first_chars[i] + j] = (int) result[i][j];
        }
      }
    }
  }
}
