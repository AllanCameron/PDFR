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
#include<numeric>
#include <algorithm>

using namespace std;

//---------------------------------------------------------------------------//

grid::grid(const graphic_state& GS) : gs(GS)
{
  gridpoints.reserve(256);
  std::iota(gridpoints.begin(), gridpoints.begin() + 255, 0);
  makegrid();
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
                      false
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
