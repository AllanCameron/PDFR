//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR LineGrouper implementation file                                     //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "line_grouper.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// The LineGrouper constructor takes the WordGrouper output and goes through all
// of its text boxes. If the elements within each box can be grouped together
// into a single logical component, then they are glued together into a logical
// unit. Otherwise, the box is split vertically.

LineGrouper::LineGrouper(PageBox t_text_boxes)
  : text_boxes_(t_text_boxes)
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

//---------------------------------------------------------------------------//
// Since the TextElements are now sorted by reading order, we can compare
// consecutive elements in a textbox to work out whether they belong to the
// same logical group. If they don't, then we call SplitBox_ to seperate them.
//
// This method identifies whether a new line is indented compared the previous
// line.

void LineGrouper::FindBreaks_(TextBox& t_text_box)
{
  // For each TextElement in the TextBox
  for (size_t i = 1; i < t_text_box.size(); ++i)
  {
    if (t_text_box[i]->GetBottom() < t_text_box[i - 1]->GetBottom() && // Below
       t_text_box[i]->GetLeft() - t_text_box[i - 1]->GetLeft() > 0.1) // To left
    {
      SplitBox_(t_text_box, t_text_box[i - 1]->GetBottom());
      break;
    }

  }
}

//---------------------------------------------------------------------------//
// To join lines together properly, we normally want to add a space to seperate
// the word ending the line above and the first word of the line below. However,
// we don't want to add an extra whitespace if the line already ends in one.
// Furthermore, we don't want to add a space between the two fragments of a
// hyphenated word, and instead we should just remove the hyphen.
//
// This method handles these various possibilities

void LineGrouper::LineEndings_(TextBox& t_text_box)
{
  // For each element in the TextBox
  for (size_t i = 0; i < t_text_box.size() - 1; ++i)
  {
    auto& element = t_text_box[i];
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

//---------------------------------------------------------------------------//
// Combines the text elements into a single element with the textbox

void LineGrouper::PasteLines_(TextBox& t_text_box)
{
  for (auto& element : t_text_box)
  {
    if (&element == &(t_text_box[0])) continue;
    t_text_box[0]->ConcatenateUnicode(element->GetGlyph());
  }

  t_text_box.resize(1);
}

//---------------------------------------------------------------------------//
// Divides a TextBox into two by a horizontal line given as a y value

void LineGrouper::SplitBox_(TextBox& t_upper, float t_top_edge)
{
  if (t_upper.empty()) return; // Don't split the box if it's empty

  // Lambda to find elements whose bottom edge is below the cutoff
  auto FindLower = [&](TextPointer text_ptr) -> bool
                   { return text_ptr->GetTop() < t_top_edge; };

  // Gets an iterator to the first element below the cutoff
  auto split_at = find_if(t_upper.begin(), t_upper.end(), FindLower);

  // We won't split the box if all or none of the elements would be moved
  // to a new box
  if (split_at == t_upper.begin() || split_at == t_upper.end()) return;

  // Create a new textbox using a vector of all elements below the cutoff
  // and a down-cast copy of the text box
  TextBox lower(vector<TextPointer>(split_at, t_upper.end()), (Box) t_upper);

  // Now we can erase the lower elements we have just copied from the upper box
  t_upper.erase(split_at, t_upper.end());

  // We also need to readjust the margins of our bounding boxes based on their
  // new contents
  t_upper.SetBottom(t_upper.back()->GetBottom());
  lower.SetTop(lower.front()->GetTop());

  // The upper box has been changed in place, but the lower box needs to be
  // added to our collection as a new member
  text_boxes_.push_back(lower);
}

