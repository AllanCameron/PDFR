//----------------------------------------------------------------------------//
//                                                                            //
//  PDFR LetterGrouper implementation file                                    //
//                                                                            //
//  Copyright (C) 2018 - 2019 by Allan Cameron                                //
//                                                                            //
//  Licensed under the MIT license - see https://mit-license.org              //
//  or the LICENSE file in the project root directory                         //
//                                                                            //
//----------------------------------------------------------------------------//

#include<algorithm>
#include "letter_grouper.h"

//----------------------------------------------------------------------------//

using namespace std;

//----------------------------------------------------------------------------//
// Output words without first joining them into lines

TextTable LetterGrouper::Out()
{
  return TextTable(*text_box_);
}

//----------------------------------------------------------------------------//
// The LetterGrouper constructor calls three subroutines. These split the
// page into an easily addressable 16 x 16 grid, find glyphs in close proximity
// to each other, and glue them together, respectively.

LetterGrouper::LetterGrouper(std::unique_ptr<TextBox> p_text_box)
  : text_box_(move(p_text_box))
{
  text_box_->RemoveDuplicates();
  MakeGrid_();     // Split the glyphs into 256 cells to reduce search space
  CompareCells_(); // Find adjacent glyphs
  Merge_();        // Glue adjacent glyphs together
}

//----------------------------------------------------------------------------//
// This method creates a 16 x 16 grid of equally-sized bins across the page and
// places each TextElement from the parser into a vector in each bin. The
// reason for doing this is speed up the search of potentially adjoining glyphs.
// The naive method would compare the right and bottom edge of every glyph
// to every other glyph. By putting the glyphs into bins, we only need to
// compare the right edge of each glyph with glyphs in the same bin or the bin
// immediately to the right. For completeness, to capture letters with low
// descenders, subscript and superscript letters, we also check the two cells
// above and two cells below each character. This still leaves us with a search
// space of approximately 6/256 * n^2 as opposed to n^2, which is 40 times
// smaller. The grid position is easily stored as a single byte, with the first
// 4 bits representing the x position and the second representing the y.

void LetterGrouper::MakeGrid_()
{
  // Define the grid's cells' widths and heights
  float width =  (text_box_->Width())  / 16,
        height = (text_box_->Height()) / 16;

  // For each glyph
  for (auto element : *text_box_)
  {
    // Calculate the row and column number the glyph's bottom left corner is in
    // There will be exactly 16 rows and columns, each numbered 0-15 (4 bits)
    uint8_t column = (element->GetLeft() - text_box_->GetLeft()) / width;
    uint8_t row = 15 - (element->GetBottom() - text_box_->GetBottom()) / height;

    // Convert the two 4-bit row and column numbers to a single byte
    uint8_t index = (row << 4) | column;

    // Append a pointer to the glyph's TextElement to the vector in that cell
    grid_[index].push_back(element);
  }
}

//----------------------------------------------------------------------------//
// Allows the main data object to be output after calculations done

std::unique_ptr<TextBox> LetterGrouper::Output()
{
  // This lambda is used to find text_ptrs that aren't flagged for deletion
  auto extant    = [&](const TextPointer& elem) -> bool
                      { return !(elem->IsConsumed());};

  // This lambda defines a TextPointer sort from left to right
  auto left_sort = [ ](const TextPointer& a, const TextPointer& b) -> bool
                      { return a->GetLeft() < b->GetLeft();};

  // Now copy all the text_ptrs from the grid to a vector
  vector<TextPointer> v;
  for (auto& cell : grid_)
  {
    copy_if(cell.second.begin(), cell.second.end(), back_inserter(v), extant);
  }

  sort(v.begin(), v.end(), left_sort); // Sort left to right
  text_box_->SwapData(v);              // Swap rather than copy result
  return move(text_box_);
}

//----------------------------------------------------------------------------//
// This method co-ordinates the proximity matching of individual glyphs to
// stick them together into words. It does so by taking each glyph and comparing
// its left side, right side and bottom edge against other glyphs. Rather than
// doing this for every other glyph on the page, it only compares each glyph to
// nearby glyphs. These are those found in the same cell and the cells to the
// North and South. If there is no match found in these it will also look in
// the cells at the NorthEast, East and SouthEast.

void LetterGrouper::CompareCells_()
{
  // For each of 16 columns of cells on page
  for (uint8_t column = 0; column < 16; ++column)
  {
    // For each row in the column
    for (uint8_t row = 0; row < 16; ++row)
    {
      // Get the 1-byte address of the cell
      uint8_t key = column | (row << 4);

      // Get a reference to the cell's contents
      vector<TextPointer>& maingroup = grid_[key];

      if (maingroup.empty()) continue; // Empty cell - nothing to be done

      for (auto& element : maingroup)
      {
        // We want to look for potential joins in up to six cells around our
        // text element : the same cell, the cell to the East, the cell North,
        // the cell South, and the cells to the NorthEast and SouthEast. If our
        // element is on a cell in the North, South or East side of the grid
        // however, there may be no cell in one or more of these locations and
        // we don't want to look for non-existent cells.
        //
        // We therefore define an index (0 to 5) representing each of these
        // 6 relative locations and loop through them with bounds checking.
        // The 6 cells are numbered thus:
        //
        //        -----------------------------------------
        //        |                  |                    |
        //        | 0 = North cell   | 3 = NorthEast cell |
        //        |                  |                    |
        //        -----------------------------------------
        //        |                  |                    |
        //        | 1 = index cell   | 4 = East cell      |
        //        |                  |                    |
        //        -----------------------------------------
        //        |                  |                    |
        //        | 2 = South cell   | 5 = SouthEast cell |
        //        |                  |                    |
        //        -----------------------------------------
        //
        for (int index = 0; index < 6; ++index)
        {
          // If the cell is on the Eastmost side, can't look further East
          if (column == 15 &&  index > 2) break;

          // If we already have a match, you won't find a better one to the East
          if (element->HasJoin() && index == 3 ) break;

          // If we're on the Northmost row, don't look Northwards
          if (row == 0 && (index % 3 == 2)) continue;

          // If we're on the Southmost row, don't look Southwards
          if (row == 15 && (index % 3 == 1)) continue;

          // Having bounds checked, we're free to look up the cell we want
          // and match its contents to this cell
          uint8_t cell = (column + (index / 3)) | ((row + index % 3 - 1) << 4);
          MatchRight_(element, cell);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------//
// This is the algorithm that finds adjoining glyphs. Each glyph is addressable
// by its cell and the order it appears in the vector of glyphs contained in
// that cell.

void LetterGrouper::MatchRight_(TextPointer p_element, uint8_t p_key)
{

  auto& cell = grid_[p_key];  // The key is the address of the cell in the grid.
  if (cell.empty()) return;  // some cells are empty - nothing to do

  for (auto& other : cell)   // For each glyph in the cell
  {
    // If in good position to be next glyph
    if (p_element->IsAdjoiningLetter(*other))
    {

      if (*other == *p_element) p_element->Consume(); // Consume if identical.
      if (other->IsConsumed()) continue;          // Skip if consumed.

      if (!p_element->HasJoin())
      {
        p_element->SetJoin(other);
        continue;                // Don't bother checking next statement
      }

      if (p_element->GetJoin()->GetLeft() > other->GetLeft())
      {
        p_element->SetJoin(other); // Join if this is better than existing match
      }

    }
  }
}

//----------------------------------------------------------------------------//
// Takes each glyph and sticks it onto any right-adjoining glyph, updating the
// the latter's size and position parameters and declaring the leftward glyph
// "consumed"

void LetterGrouper::Merge_()
{
  // For each cell in the grid
  for (size_t index = 0; index < 256; ++index)
  {
    //                      Convert index to grid location:                   //
    //                v-- row number --v    v-- column number --v             //
    uint8_t location =  16 * (index % 16)  +      index / 16;

    // Get the cell's contents
    vector<TextPointer>& cell = grid_[location];

    // If the cell is empty there's nothing to do
    if (cell.empty()) continue;

    // For each glyph in the cell, if consumed or no join found, skip to next
    // glyph. Otherwise, join the two glyphs
    for (auto& element : cell)
    {
      if (element->IsConsumed() || !element->HasJoin()) continue; // Skip
      auto matcher = element->GetJoin(); // Look up the right-matching glyph
      element->MergeLetters(*matcher);   // Merge the two glyphs
    }
  }
}
