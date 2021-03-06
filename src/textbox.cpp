//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextElement implementation file                                     //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "box.h"
#include "font.h"
#include "textbox.h"

using namespace std;

//---------------------------------------------------------------------------//
// Converts TextBox to TextTable
TextTable::TextTable(const TextBox& text_box):
Box((Box) text_box)
{
  for (auto ptr = text_box.cbegin(); ptr != text_box.cend(); ++ptr)
  {
    auto& element = *ptr;
    if (!element->IsConsumed())
    {
      this->text_.push_back(element->Utf());
      this->lefts_.push_back(element->GetLeft());
      this->bottoms_.push_back(element->GetBottom());
      this->rights_.push_back(element->GetRight());
      this->fonts_.push_back(element->GetFontName());
      this->tops_.push_back(element->GetTop());
      this->sizes_.push_back(element->GetSize());
    }
  }
}

//---------------------------------------------------------------------------//

void TextBox::RemoveDuplicates()
{
  for (auto this_row = data_.begin(); this_row != data_.end(); ++this_row)
  {
    if ((*this_row)->IsConsumed()) continue;
    for (auto other_row = this_row; other_row != data_.end(); ++other_row)
    {
      if (other_row == this_row) continue;

      if (**other_row == **this_row)
      {
        (*other_row)->Consume();
      }
    }
  }
}

//---------------------------------------------------------------------------//
// Join another text table to this one

void TextTable::Join(TextTable& other)
{
  this->Merge(other);
  Concatenate(this->text_,    other.text_);
  Concatenate(this->lefts_,   other.lefts_);
  Concatenate(this->bottoms_, other.bottoms_);
  Concatenate(this->rights_,  other.rights_);
  Concatenate(this->fonts_,   other.fonts_);
  Concatenate(this->tops_,    other.tops_);
}


//----------------------------------------------------------------------------//
// Divides a TextBox into two by a horizontal line given as a y value

TextBox TextBox::SplitIntoTopAndBottom(float top_edge)
{
  if (this->empty()) return TextBox(); // Don't split the box if it's empty

  // Lambda to find elements whose bottom edge is below the cutoff
  auto FindLower = [&](TextPointer text_ptr) -> bool
                   { return text_ptr->GetTop() < top_edge; };

  // Gets an iterator to the first element below the cutoff
  auto split_at = find_if(this->begin(), this->end(), FindLower);

  // We won't split the box if all or none of the elements would be moved
  // to a new box
  if (split_at == this->begin() || split_at == this->end()) return TextBox();

  // Create a new textbox using a vector of all elements below the cutoff
  // and a down-cast copy of the text box
  std::vector<TextPointer> lower_contents {split_at, this->end()};
  auto lower = TextBox(std::move(lower_contents), (Box) *this);

  // Now we can erase the lower elements we have just copied from the upper box
  this->erase(split_at, this->end());

  // We also need to readjust the margins of our bounding boxes based on their
  // new contents
  this->SetBottom(this->back()->GetBottom());
  lower.SetTop(lower.front()->GetTop());

  // The upper box has been changed in place,
  return lower;
}

//----------------------------------------------------------------------------//
// Divides a TextBox into two by a vertical line given as an x value

TextBox TextBox::SplitIntoLeftAndRight(float left_edge)
{
  if (this->empty()) return TextBox(); // Don't split the box if it's empty

    // This lambda defines a TextPointer sort from left to right
  auto LeftSort = [ ](const TextPointer& a, const TextPointer& b) -> bool
                  { return a->GetLeft() < b->GetLeft(); };

  std::stable_sort(this->begin(), this->end(), LeftSort);

  // Lambda to find elements whose left edge is below the cutoff
  auto FindLeftMost = [&](TextPointer text_ptr) -> bool
                   { return text_ptr->GetLeft() < left_edge; };

  // Gets an iterator to the first element right of the cutoff
  auto split_at = find_if(this->begin(), this->end(), FindLeftMost);

  // We won't split the box if all or none of the elements would be moved
  // to a new box
  if (split_at == this->begin() || split_at == this->end()) return TextBox();

  // Create a new textbox using a vector of all elements below the cutoff
  // and a down-cast copy of the text box
  std::vector<TextPointer> rightmost_contents {split_at, this->end()};
  auto rightmost = TextBox(std::move(rightmost_contents), (Box) *this);

  // Now we can erase the lower elements we have just copied from the upper box
  this->erase(split_at, this->end());

  // We also need to readjust the margins of our bounding boxes based on their
  // new contents
  this->SetRight(this->back()->GetRight());
  rightmost.SetLeft(rightmost.front()->GetTop());

  // The upper box has been changed in place,
  return rightmost;
}
