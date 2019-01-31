//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR grid implementation file                                            //
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

#include "grid.h"
#include <numeric>
#include <algorithm>
#include <cstdlib>

using namespace std;

static float CLUMP_H = 0.1;
static float CLUMP_V = 0.5;

//---------------------------------------------------------------------------//

grid::grid(const graphic_state& GS) : gs(GS)
{
  gridpoints.reserve(256);
  std::iota(gridpoints.begin(), gridpoints.begin() + 255, 0);
  makegrid();
  compareCells();
  merge();
}

//---------------------------------------------------------------------------//
// This method creates a 16 x 16 grid of equally-sized bins across the page and
// places each row of the GS output into a vector in each bin. The reason for
// doing this is to narrow the search space of potentially adjoining glyphs. The
// naive method would involve comparing the right and bottom edge of every glyph
// to every other glyph. By putting the glyphs into bins, we only need to
// compare the right edge of each glyph with glyphs in the same bin or the bin
// immediately to the right. For completeness, to capture letters with low
// descenders, subscript and superscript letters, we also check the two cells
// above and two cells below each character. This still leaves us with a search
// space of approximately 6/256 * n^2 as opposed to n^2, which is 40 times
// smaller. The grid position is easily stored as a single byte, with the first
// 8 bits representing the x position and the second representing the y.

void grid::makegrid()
{
  minbox = gs.getminbox();
  GSoutput gslist = gs.output();
  float dx = (minbox[2] - minbox[0])/16;
  float dy = (minbox[3] - minbox[1])/16;
  for(unsigned i = 0; i < gslist.width.size(); i++)
  {
    GSrow newrow = {  gslist.left[i],
                      gslist.right[i],
                      gslist.width[i],
                      gslist.bottom[i],
                      gslist.size[i],
                      gslist.fonts[i],
                      {gslist.text[i]},
                      false,
                      make_pair(-1, -1)
                    };

    gridmap[((uint8_t)(newrow.left / dx))  |
            ((uint8_t)(15 - (newrow.bottom / dy)) << 4 )].push_back(newrow);
  }
  for(auto& vec : gridmap)
  {
    std::sort(vec.second.begin(), vec.second.end(), sort_left_right());
  }
}

//---------------------------------------------------------------------------//

std::unordered_map<uint8_t, std::vector<GSrow>> grid::output()
{
  return gridmap;
}

//---------------------------------------------------------------------------//

void grid::compareCells()
{
  for(uint8_t i = 0; i < 16; i++)
  {
    for(uint8_t j = 0; j < 16; j++)
    {
      uint8_t key = i | (j << 4);
      vector<GSrow>& maingroup = gridmap[key];
      if(maingroup.size() > 0)
      {
        for(auto& k : maingroup)
        {
          matchRight(k, key);
          if(j < 15) matchRight(k, (i) | ((j + 1) << 4));
          if(j > 0) matchRight(k, (i) | ((j - 1) << 4));
          if(k.rightjoin.first != -1) continue;
          if(i < 15) matchRight(k, (i + 1) | (j << 4));
          if(i < 15 && j < 15) matchRight(k, (i + 1) | ((j + 1) << 4));
          if(i < 15 && j > 0) matchRight(k, (i + 1) | ((j - 1) << 4));
        }
      }
    }
  }
}

//---------------------------------------------------------------------------//

void grid::matchRight(GSrow& row, uint8_t key)
{
  std::vector<GSrow>& cell = gridmap[key];
  if(cell.empty()) return;
  for(uint16_t i = 0; i < cell.size(); i++)
    if (cell[i].left > row.left &&
        abs(cell[i].bottom - row.bottom) < (CLUMP_V * row.size) &&
        abs(cell[i].left  -  row.right ) < (CLUMP_H * row.size) &&
        cell[i].size == row.size)
    {
      if(row.rightjoin.first == -1)
      {
        row.rightjoin = make_pair((int) key, (int) i);
        continue;
      }
      if(gridmap[row.rightjoin.first][row.rightjoin.second].left >
           cell[i].left)
      {
        gridmap[row.rightjoin.first][row.rightjoin.second].consumed = false;
        row.rightjoin = make_pair((int) key, (int) i);
      }
    }
}

//---------------------------------------------------------------------------//

void grid::merge()
{
  for(uint8_t i = 0; i < 16; i++)
  {
    for(uint8_t j = 0; j < 16; j++)
    {
      uint8_t key = i | (j << 4);
      vector<GSrow>& cell = gridmap[key];
      if(cell.size() == 0) continue;
      for(auto& k : cell)
      {
        if(k.rightjoin.first == -1) continue;
        GSrow& matcher = gridmap[k.rightjoin.first][k.rightjoin.second];
        concat(k.glyph, matcher.glyph);
        matcher.glyph = k.glyph;
        matcher.left = k.left;
        if(k.bottom < matcher.bottom) matcher.bottom = k.bottom;
        k.consumed = true;
      }
    }
  }
}


std::vector<float> grid::getBox()
{
  return minbox;
}
