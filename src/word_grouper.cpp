//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR WordGrouper implementation file                                    //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "word_grouper.h"

using namespace std;

//---------------------------------------------------------------------------//
// This "magic number" is an integer that specifies how many glyphs need to
// line up to infer an aligned column on the page.

constexpr int EDGECOUNT = 4;

//---------------------------------------------------------------------------//
// Constructor for WordGrouper class. This takes the output from LetterGrouper
// and finds its column edges, then joins elligible words together as long as
// they do not belong to different columns.

WordGrouper::WordGrouper(std::unique_ptr<TextBox> text_box)
  : text_box_(move(text_box))
{
  FindEdges_();
  AssignEdges_();
  FindRightMatch_();
};

//---------------------------------------------------------------------------//
// Makes a table of supplied vector of floats. Multiplies them by 10 and
// casts to int as a way of rounding to 1 decimal place. It then removes any
// keys whose counts are less than EDGECOUNT, so the remaining keys are the
// positions we wish to identify as possible edges. Since the maps we want to
// return are data members of the class, we need to pass the map we wish to
// create by reference.

void WordGrouper::Tabulate_(const vector<float>& supplied_vector,
                            unordered_map<int, size_t>& table   )
{
  // Take each member of the supplied vector
  for (const auto& element : supplied_vector)
  {
    // Multiply it by 10 and use it as a key in the map with value 1
    auto inserter = table.insert(pair<int, size_t>((int) 10 * element, 1));

    // If the key already exists in the map, increment the value by 1
    if (!inserter.second) inserter.first->second++;
  }

  // Now take each key in the resulting map
  for (auto key_value_pair = table.begin(); key_value_pair != table.end(); )
  {
    // if value is below the number needed to declare a column, delete it
    if (key_value_pair->second < EDGECOUNT)
    {
      table.erase(key_value_pair++);
    }
    else ++key_value_pair;
  }
}

//---------------------------------------------------------------------------//
// This uses the Tabulate function to find left, right and centre-aligned text
// elements on the page.

void WordGrouper::FindEdges_()
{
  // Create vectors of left and right edges of text elements
  vector<float> left, right, midvec;
  left.reserve(text_box_->size());
  right.reserve(text_box_->size());
  midvec.reserve(text_box_->size());

  for (auto& element : *text_box_)
  {
    left.push_back(element->GetLeft());
    right.push_back(element->GetRight());
    midvec.push_back((left.back() + right.back()) / 2);
  }


  // Use Tabulate to find left and right edges as well as midpoints
  Tabulate_(left,   left_edges_);
  Tabulate_(right,  right_edges_);
  Tabulate_(midvec, mids_);
}

//---------------------------------------------------------------------------//
// Now we need to "tell" each element whether it is a left, right or centre
// aligned element so it "knows" which side(s), if any, are eligible to join
// other elements

void WordGrouper::AssignEdges_()
{
  for (auto& element : *text_box_)
  {
    int left_int = element->GetLeft() * 10;
    int right_int = element->GetRight() * 10;
    int mid_int = (element->GetRight() + element->GetLeft()) * 5;

    // Non-unique left edge - assume column edge
    if (left_edges_.find(left_int) != left_edges_.end())
    {
      element->MakeLeftEdge();
    }

    // Non-unique right edge - assume column edge
    if (right_edges_.find(right_int) != right_edges_.end())
    {
      element->MakeRightEdge();
    }

    // Non-unique centre value - assume centred column
    if (mids_.find(mid_int) != mids_.end())
    {
      element->MakeCentred();
    }
  }
}

//---------------------------------------------------------------------------//
// It's a bit naughty for a function to do two things instead of one, but these
// two things are easier / quicker done in a single loop. Go through each text
// item and check whether it is elligible for joining to another element. If it
// is, find the most appropriate match to its right that is elligible and stick
// the two together.

void WordGrouper::FindRightMatch_()
{
  // Handle empty data
  if (text_box_->empty()) throw runtime_error("empty data");

  for (auto element = text_box_->begin(); element != text_box_->end(); ++element)
  {
    // Check the row is elligible for matching
    if ((*element)->IsConsumed()) continue;

    // If elligible, check every other word for the best match
    for (auto other = element; other != text_box_->end(); ++other)
    {
      // Don't match against itself
      if (element == other) continue;

      // These TextElement functions are quite complex in themselves
      if ((*element)->IsElligibleToJoin(**other))
      {
        (*element)->JoinWords(**other);
        --element;  // Keep matching same element until no other matches found
        break;
      }
    }
  }
}

//---------------------------------------------------------------------------//

TextTable WordGrouper::Out() const { return TextTable(*text_box_);}
