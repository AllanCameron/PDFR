//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR word_grouper implementation file                                    //
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

#include "word_grouper.h"

using namespace std;

//---------------------------------------------------------------------------//
// This "magic number" is an integer that specifies how many glyphs need to
// line up to infer an aligned column on the page.

constexpr int EDGECOUNT = 4;

//---------------------------------------------------------------------------//
// Constructor for word_grouper class. This takes the output from letter_grouper
// and finds its column edges, then joins elligible words together as long as
// they do not belong to different columns.

word_grouper::word_grouper(textbox&& g): m_textbox(move(g))
{
  findEdges();
  assignEdges();
  findRightMatch();
};

//---------------------------------------------------------------------------//
// This returns a single text box of the page for further processing if needed

textbox& word_grouper::output()
{
  return m_textbox;
}

//---------------------------------------------------------------------------//
// This returns a "gridoutput" object, which is a struct of several vectors of
// the same length, essentially a transposed version of allRows. This allows
// the results of word_grouper to be passed out to an API if no further
// processing is required.

text_table word_grouper::out()
{
  return text_table(m_textbox);
}

//---------------------------------------------------------------------------//
// Makes a table of supplied vector of floats. Multiplies them by 10 and
// casts to int as a way of rounding to 1 decimal place. It then removes any
// keys whose counts are less than EDGECOUNT, so the remaining keys are the
// positions we wish to identify as possible edges. Since the maps we want to
// return are data members of the class, we need to pass the map we wish to
// create by reference.

void word_grouper::tabulate(const vector<float>& supplied_vector,
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
// This uses the tabulate function to find left, right and centre-aligned text
// elements on the page.

void word_grouper::findEdges()
{
  // Create vectors of left and right edges of text elements
  vector<float> left, right;
  for(auto& element : m_textbox)
  {
    left.push_back(element->get_left());
    right.push_back(element->get_right());
  }

  // Create a vector of midpoints of text elements
  vector<float> midvec;
  for(size_t i = 0; i < right.size(); ++i)
  {
    midvec.emplace_back((right[i] + left[i]) / 2);
  }

  // Use tabulate to find left and right edges as well as midpoints
  tabulate(left, m_leftEdges);
  tabulate(right, m_rightEdges);
  tabulate(midvec, m_mids);
}

//---------------------------------------------------------------------------//
// Now we need to "tell" each element whether it is a left, right or centre
// aligned element so it "knows" which side(s), if any, are eligible to join
// other elements

void word_grouper::assignEdges()
{
  for(auto& element : m_textbox)
  {
    int left_int = element->get_left() * 10;
    int right_int = element->get_right() * 10;
    int mid_int = (element->get_right() + element->get_left()) * 5;

    // Non-unique left edge - assume column edge
    if(m_leftEdges.find(left_int) != m_leftEdges.end())
    {
      element->make_left_edge();
    }

    // Non-unique right edge - assume column edge
    if(m_rightEdges.find(right_int) != m_rightEdges.end())
    {
      element->make_right_edge();
    }

    // Non-unique centre value - assume centred column
    if(m_mids.find(mid_int) != m_mids.end())
    {
      element->make_centred();
    }
  }
}

//---------------------------------------------------------------------------//
// It's a bit naughty for a function to do two things instead of one, but these
// two things are easier / quicker done in a single loop. Go through each text
// item and check whether it is elligible for joining to another element. If it
// is, find the most appropriate match to its right that is elligible and stick
// the two together.

void word_grouper::findRightMatch()
{
  // Handle empty data
  if(m_textbox.empty()) throw runtime_error("empty data");

  for(auto element = m_textbox.begin(); element != m_textbox.end(); ++element)
  {
    // Check the row is elligible for matching
    if( (*element)->is_consumed()) continue;

    // If elligible, check every other word for the best match
    for(auto other = element; other != m_textbox.end(); ++other)
    {
      // Don't match against itself
      if(element == other) continue;

      // These text_element functions are quite complex in themselves
      if((*element)->is_elligible_to_join(**other))
      {
        (*element)->join_words(**other);
        --element;  // Keep matching same element until no otehr matches found
        break;
      }
    }
  }
}
