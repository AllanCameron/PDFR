//----------------------------------------------------------------------------//
//                                                                            //
//  PDFR LineGrouper implementation file                                      //
//                                                                            //
//  Copyright (C) 2018 - 2019 by Allan Cameron                                //
//                                                                            //
//  Licensed under the MIT license - see https://mit-license.org              //
//  or the LICENSE file in the project root directory                         //
//                                                                            //
//----------------------------------------------------------------------------//

#include<algorithm>
#include<unordered_map>
#include<memory>
#include<stdexcept>
#include "line_grouper.h"

//----------------------------------------------------------------------------//

using namespace std;

//----------------------------------------------------------------------------//
// The LineGrouper constructor takes the WordGrouper output and goes through all
// of its text boxes. If the elements within each box can be grouped together
// into a single logical component, then they are glued together into a logical
// unit. Otherwise, the box is split vertically.

LineGrouper::LineGrouper(PageBox text_boxes)
  : text_boxes_(text_boxes)
{
  size_t i = 0;

  // If there are no textboxes, there is nothing to do.
  if ( !text_boxes_.empty())
  {
    while (i < text_boxes_.size())
    {
      // There is no point processing a textbox with 0 or 1 elements.
      if (text_boxes_[i].size() < 2){++i; continue;}

      // Ensures the text elements are in the correct reading order in the box.
      sort(text_boxes_[i].begin(), text_boxes_[i].end(), ReadingOrder_());

      // Finds logical breaks within the text box and splits if needed.
      FindBreaks_(text_boxes_[i]);

      // After splitting, there may only be 1 element left in the box.
      if (text_boxes_[i].size() < 2){++i; continue;}

      // Makes sure the lines have correct final character before being pasted
      LineEndings_(text_boxes_[i]);

      // Pastes the text elements together
      PasteLines_(text_boxes_[i++]);
    }
  }
};

//----------------------------------------------------------------------------//
// Since the TextElements are now sorted by reading order, we can compare
// consecutive elements in a textbox to work out whether they belong to the
// same logical group. If they don't, then we call SplitBox_ to seperate them.
//
// This method identifies whether a new line is indented compared the previous
// line.

void LineGrouper::FindBreaks_(TextBox& text_box)
{
  // For each TextElement in the TextBox
  for (size_t i = 1; i < text_box.size(); ++i)
  {
    if (text_box[i]->GetBottom() < text_box[i - 1]->GetBottom() && // Below
        text_box[i]->GetLeft() - text_box[i - 1]->GetLeft() > 0.1) // To left
    {
      auto slice_at = text_box[i - 1]->GetBottom();
      auto&& new_box = text_box.SplitIntoTopAndBottom(slice_at);
      if (!new_box.empty()) text_boxes_.push_back(new_box);
      break;
    }
  }
}

//----------------------------------------------------------------------------//
// To join lines together properly, we normally want to add a space to seperate
// the word ending the line above and the first word of the line below. However,
// we don't want to add an extra whitespace if the line already ends in one.
// Furthermore, we don't want to add a space between the two fragments of a
// hyphenated word, and instead we should just remove the hyphen.
//
// This method handles these various possibilities

void LineGrouper::LineEndings_(TextBox& text_box)
{
  // For each element in the TextBox
  for (size_t i = 0; i < text_box.size() - 1; ++i)
  {
    auto& element = text_box[i];
    switch (element->GetGlyph().back())
    {
      case 0x0020:                          break;
      case 0x00A0:                          break;
      case 0x002d: element->PopLastGlyph(); break;
      case 0x2010: element->PopLastGlyph(); break;
      case 0x2011: element->PopLastGlyph(); break;
      case 0x2012: element->PopLastGlyph(); break;
      case 0x2013: element->PopLastGlyph(); break;
      case 0x2014: element->PopLastGlyph(); break;
      case 0x2015: element->PopLastGlyph(); break;
      default:     element->AddSpace();
    }
  }
}

//----------------------------------------------------------------------------//
// Combines the text elements into a single element with the textbox

void LineGrouper::PasteLines_(TextBox& text_box)
{
  for (auto& element : text_box)
  {
    if (&element != &(text_box[0]))
    {
      text_box[0]->ConcatenateUnicode(element->GetGlyph());
    }
  }
  text_box.resize(1);
}


