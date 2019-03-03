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
constexpr float MAX_SPACE = 0;
constexpr float ONE_SPACE = 0;
constexpr float H_MAX = 0.7;

//---------------------------------------------------------------------------//
// Constructor for word_grouper class. This takes the output from letter_grouper
// and finds its column edges, then joins elligible words together as long as
// they do not belong to different columns.

word_grouper::word_grouper(letter_grouper* g): theGrid(g)
{
  // get the output from letter_grouper
  std::unordered_map<uint8_t, std::vector<textrow>> gridout = theGrid->output();

  // This block fills "allRows" by taking the ouput from letter_grouper and
  // placing each word with its associated size and position (a textrow) from it
  // into a vector
  vector<textrow> tmpvec;
  for(uint8_t i = 0; i < 255; i++)
    concat(tmpvec, gridout[i]);
  concat(tmpvec, gridout[0xff]); // otherwise loop will overflow infinitely
  for(auto& i : tmpvec)
    if(!i.consumed) allRows.emplace_back(move(i)); // no consumed entries please

  findEdges();
  assignEdges();
  findRightMatch();
};

//---------------------------------------------------------------------------//
// This returns a vector of textrows for continued processing if needed

std::vector<textrow> word_grouper::output()
{
  return allRows;
}

//---------------------------------------------------------------------------//
// This returns a "gridoutput" object, which is a struct of several vectors of
// the same length, essentially a transposed version of allRows. This allows
// the results of word_grouper to be passed out to an API if no further
// processing is required.

gridoutput word_grouper::out()
{
  std::vector<float> left, right, width, bottom, size; // temporary
  std::vector<std::string> font, text;                 //  vectors
  for(auto& j : allRows) // for each of allRows...
  {
    if(!j.consumed) // unless it is consumed...
    {
      text.push_back(utf({j.glyph}));    //---//
      left.push_back(j.left);                 //
      right.push_back(j.right);               //
      size.push_back(j.size);                 //--> fill the temporary vectors
      bottom.push_back(j.bottom);             //
      font.push_back(j.font);                 //
      width.push_back(j.right - j.left); //---//
    }
  }
  // and return them in an API-consumable format
  return gridoutput{left, right, width, bottom, size, font, text };
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
  for (auto i : a) // for each member of supplied vector
  {
    int j = 10 * i;   // multiply by 10
    if(b.find(j) == b.end())
      b[j] = 1; // if that value has not already been found, add it with count 1
    else b[j]++; // otherwise increment its count
  }
  for(auto& i : getKeys(b)) // for each key in the resulting table
    if(b[i] < EDGECOUNT) // if its value is below the number needed for a column
      b.erase(b.find(i)) ;// delete it
}

//---------------------------------------------------------------------------//
// This uses the tabulate function to find left, right and centre-aligned text
// elements on the page.

void word_grouper::findEdges()
{
  gridoutput GO = theGrid->out(); // uses the gridoutput from letter_grouper
  tabulate(GO.left, leftEdges); // use tabulate to get left edges
  tabulate(GO.right, rightEdges); // use tabulate to get right edges
  vector<float> midvec; //  create a vector of midpoints of text element --//
  for(size_t i = 0; i < GO.right.size(); i++)                              //
    midvec.emplace_back((GO.right[i] + GO.left[i])/2);  //-----------------//
  tabulate(midvec, mids); // use tabulate to find centre-aligned items
}

//---------------------------------------------------------------------------//
// Now we need to "tell" each element whether it is a left, right or centre
// aligned element so it "knows" which side(s), if any, are eligible to join
// other elements

void word_grouper::assignEdges()
{
  for(auto& i : allRows) // for each word
  {
    if(leftEdges.find((int) (i.left * 10)) != leftEdges.end())
      i.isLeftEdge = true; // Non-unique left edge - assume column edge
    if(rightEdges.find((int) (i.right * 10)) != rightEdges.end())
      i.isRightEdge = true; // Non-unique right edge - assume column edge
    if(mids.find((int) ((i.right + i.left) * 5)) != mids.end())
      i.isMid = true; // Non-unique centre value - assume centred column
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
  for(auto i = allRows.begin(); i != allRows.end(); i++) // for each word
  {
    if( i->consumed) continue; // check elligible

    for(auto& j : allRows) // if so, look at every other word for a match
    {
      if(j.consumed) continue; // ignore words that have already been joined
      if(j.left < i->right) continue; // ignore words to the left
      if(j.bottom - i->bottom > H_MAX * i->size) continue; // match elements
      if(i->bottom - j.bottom > H_MAX * i->size) continue; // on same "line"
      if(j.left - i->right > MAX_SPACE * i->size) continue; // too far right
      if((j.isLeftEdge || j.isMid) &&
         (j.left - i->right > ONE_SPACE * i->size))
        continue; // ignore if not elligble for join
      if((i->isRightEdge || i->isMid ) &&
         (j.left - i->right > ONE_SPACE * i->size))
        continue;
      // The element is elligible for joining
      i->glyph.push_back(0x0020); // add a space
      // if the gap is wide enough, add two spaces
      if(j.left - i->right > ONE_SPACE * i->size) i->glyph.push_back(0x0020);
      concat(i->glyph, j.glyph); // stick contents together
      i->right = j.right; // the right edge is now the rightmost edge
      i->isRightEdge = j.isRightEdge; // as are the right edge's properties
      if(i->size < j.size) i->size = j.size;
      i->width = i->right - i->left; // update the width
      j.consumed = true; // the element on the right is now consumed
      i--; // The element we have just matched now has different characteristics
           // so may be matched by a different element - match it again until
           // no further matches are found.
    }
  }
}
