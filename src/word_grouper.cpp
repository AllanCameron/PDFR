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

word_grouper::word_grouper(textrows&& g): allRows(move(g))
{
  findEdges();
  assignEdges();
  findRightMatch();
};

//---------------------------------------------------------------------------//
// This returns a vector of textrows for continued processing if needed

textrows& word_grouper::output()
{
  return allRows;
}

//---------------------------------------------------------------------------//
// This returns a "gridoutput" object, which is a struct of several vectors of
// the same length, essentially a transposed version of allRows. This allows
// the results of word_grouper to be passed out to an API if no further
// processing is required.

GSoutput word_grouper::out()
{
  return allRows.transpose();
}

//---------------------------------------------------------------------------//
// Makes a table of supplied vector of floats. Multiplies them by 10 and
// casts to int as a way of rounding to 1 decimal place. It then removes any
// keys whose counts are less than EDGECOUNT, so the remaining keys are the
// positions we wish to identify as possible edges. Since the maps we want to
// return are data members of the class, we need to pass the map we wish to
// create by reference.

void word_grouper::tabulate(const vector<float>& a,
                            unordered_map<int, size_t>& b)
{
  // Take each member of the supplied vector
  for (const auto& i : a)
  {
    // Multiply it by 10 and use it as a key in the map with value 1
    auto k = b.insert(std::pair<int, size_t>((int) 10 * i, 1));

    // If the key already exists in the map, increment the value by 1
    if(!k.second) k.first->second++;
  }

  // Now take each key in the resulting map
  for(auto i = b.begin(); i != b.end(); )
  {
    // if value is below the number needed to declare a column, delete it
    if(i->second < EDGECOUNT)
    {
      b.erase(i++);
    }
    else ++i;
  }
}

//---------------------------------------------------------------------------//
// This uses the tabulate function to find left, right and centre-aligned text
// elements on the page.

void word_grouper::findEdges()
{
  // Create vectors of left and right edges of text elements
  vector<float> left, right;
  for(auto& i : allRows)
  {
    left.push_back(i->left);
    right.push_back(i->right);
  }

  // Create a vector of midpoints of text elements
  vector<float> midvec;
  for(size_t i = 0; i < right.size(); ++i)
  {
    midvec.emplace_back((right[i] + left[i])/2);
  }

  // Use tabulate to find left and right edges as well as midpoints
  tabulate(left, leftEdges);
  tabulate(right, rightEdges);
  tabulate(midvec, mids);
}

//---------------------------------------------------------------------------//
// Now we need to "tell" each element whether it is a left, right or centre
// aligned element so it "knows" which side(s), if any, are eligible to join
// other elements

void word_grouper::assignEdges()
{
  for(auto& i : allRows)
  {
    // Non-unique left edge - assume column edge
    if(leftEdges.find((int) (i->left * 10)) != leftEdges.end())
    {
      i->isLeftEdge = true;
    }

    // Non-unique right edge - assume column edge
    if(rightEdges.find((int) (i->right * 10)) != rightEdges.end())
    {
      i->isRightEdge = true;
    }

    // Non-unique centre value - assume centred column
    if(mids.find((int) ((i->right + i->left) * 5)) != mids.end())
    {
      i->isMid = true;
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
  if(allRows.m_data.empty())
  {
    throw runtime_error("empty data");
  }

  for(int k = 0; k < (int) allRows.m_data.size(); k++)
  {
    // Create a reference name for the row in question
    auto& i = allRows.m_data[k];

    // Check the row is elligible for matching
    if( i->consumed)
    {
      continue;
    }

    // If elligible, check every other word for the best match
    for(int m = 0; m < (int) allRows.m_data.size(); m++)
    {
      // Dont' match against itself
      if(m == k) continue;

      // Create a reference name for the row in question
      auto& j = allRows.m_data[m];

      // Ignore words that have already been joined
      if(j->consumed) continue;

      // Ignore words to the left
      if(j->left < i->right) continue;

      // Only match elements on the same "line"
      if(j->bottom - i->bottom > 0.7 * i->size) continue;
      if(i->bottom - j->bottom > 0.7 * i->size) continue;

      // Ignore if too far right
      if(j->left - i->right > 2 * i->size) continue;

      // Ignore if not elligble for join
      if((j->isLeftEdge  || j->isMid) && (j->left - i->right > 0.51 * i->size)||
         (i->isRightEdge || i->isMid) && (j->left - i->right > 0.51 * i->size))
        continue;

      // The element is elligible for joining - start by adding a space to i
      i->glyph.push_back(0x0020);

      // If the gap is wide enough, add two spaces
      if(j->left - i->right > 1 * i->size) i->glyph.push_back(0x0020);

      // Stick contents together
      concat(i->glyph, j->glyph);

      // The rightmost glyph's right edge properties are also copied over
      i->right = j->right;
      i->isRightEdge = j->isRightEdge;

      // The word will take up the size of its largest glyph
      i->size = max(i->size, j->size);

      // Update the width
      i->width = i->right - i->left;

      // The element on the right is now consumed
      j->consumed = true;

      // The element we have just matched now has different characteristics
      // so may be matched by a different element - match it again until
      // no further matches are found.
      --k;

      break;
    }
  }
}
