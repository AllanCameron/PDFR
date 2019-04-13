//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Whitespace implementation file                                      //
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

#include "whitespace.h"
#include<memory>
#include<iostream>
using namespace std;

static const float MAXPAGE = 30000;

//---------------------------------------------------------------------------//
// Every vertex of the final polygon surrounding each text element contains
// information about its position. However, to "connect" the vertices so as to
// arrange them in clockwise order, it also needs to know which direction
// the incoming and outgoing edges are "pointing". We do this by working out
// for each vertex whether there is whitespace immediately to the NorthWest,
// NorthEast, SouthEast and SouthWest of the vertex. These are recorded as the
// four lowest order bits in a single "flags" byte; thus a vertex that had
// whitespace to the NorthWest and SouthWest (as it would if it lay along the
// middle of the left edge of a text polygon), would have its flags set to
// 1001 in binary (or 0x9 in hexadecimal, or a byte value of 0x09 provided we
// mask the flag byte with & 0x0f). Since we know that a masked flag value of
// 0x09 must represent a point lying on a left edge, its "incoming" edge must
// be travelling North (since we are concerned with clockwise ordering), and
// its outgoing edge must also be pointing North. To make all this clearer we
// want each Vertex to specify its incoming and outgoing directions. We can
// therefore just look up the four lowest order bytes in each Vertex's flags
// using this unordered map to get the implied direction based on the
// surrounding whitespace.

unordered_map<uint8_t, pair<Direction, Direction>> Whitespace::arrows =
{
  {0x00, {None, None}},   {0x01, {North, West}}, {0x02, {West, South}},
  {0x03, {West, West}},   {0x04, {South, East}}, {0x05, {None, None}},
  {0x06, {South, South}}, {0x07, {South, West}}, {0x08, {East, North}},
  {0x09, {North, North}}, {0x0A, {None, None}},  {0x0B, {West, North}},
  {0x0C, {East, East}},   {0x0D, {North, East}}, {0x0E, {East, South}},
  {0x0F, {None, None}}
};

//---------------------------------------------------------------------------//
// We want to be able to test for equality of spatial positions, but I don't
// trust comparison of floats. This allows a small margin of error when testing
// equality between two floats

bool Whitespace::eq(float a, float b)
{
  float d = b - a;
  return (d > (-0.1) && d < 0.1);
}
//---------------------------------------------------------------------------//
// The constructor takes a const word grouper object. It calls all its
// constructor helpers to get the page dimensions, construct a large number of
// tall vertical strips across the page which do not cross any text elements,
// coalesce these strips into whitespace boxes, remove any boxes that we don't
// want based on their dimensions or position, find the vertices of these
// boxes, calculate which compass directions around the vertices contain text,
// infer the directions a clockwise line would pass through such a vertex,
// then trace round all the vertices, storing every connected loop as a
// polygon surrounding a text element.

Whitespace::Whitespace(textrows wgo): WGO(wgo.m_data), minbox(wgo.minbox),
    m_page({minbox[West], minbox[East], minbox[North], minbox[South], false})
{
  getMaxLineSize();
  pageDimensions();
  makeStrips();
  mergeStrips();
  removeSmall();
  makeVertices();
  tidyVertices();
  tracePolygons();
  makePolygonMap();
  polygonMax();
  removeEngulfed();
  groupText();
}


//---------------------------------------------------------------------------//

void Whitespace::getMaxLineSize()
{
  std::vector<float> fontsizes;
  for(auto& i : WGO)
  {
    fontsizes.push_back(i->size);
  }
  sort(fontsizes.begin(), fontsizes.end());
  max_line_space = fontsizes[fontsizes.size()/2] * 0.3;
}

//---------------------------------------------------------------------------//
// The Whitespace class contains four floats that specify the positions of the
// page edges. It gets these from the cropbox which has been passed to each
// class since the page object was created. This makes it a lot clearer which
// edge we are referring to instead of having to subscript a vector.

void Whitespace::pageDimensions()
{
  for(auto& i : WGO)
  {
    if(i->right > m_page.right) m_page.right += 10.0;
    if(i->left < m_page.left) m_page.left -= 10.0;
    if((i->bottom + i->size) > m_page.top) m_page.top += 10.0;
    if(i->bottom < m_page.bottom) m_page.bottom -= 10.0;
  }
}

//---------------------------------------------------------------------------//
// At various stages we need to clear away some of the whitespace boxes; for
// example, when we merge the initial strips into boxes, we need to drop most
// of the strips. This function allows us to keep only those boxes not flagged
// for deletion

void Whitespace::cleanAndSortBoxes()
{
  // Define a lambda to identify boxes flagged for deletion
  auto del = [&](WSbox x){return x.deletionFlag;};

  // Define a lambda to sort boxes left to right
  auto l = [](const WSbox& a, const WSbox& b) -> bool {return a.left < b.left;};

  // Move boxes for deletion to back of vector, starting at returned iterator
  auto junk = remove_if(ws_boxes.begin(), ws_boxes.end(), del);

  // Erase the back of the vector containing the boxes flagged for deletion
  ws_boxes.erase(junk, ws_boxes.end());

  // Sort the remaining boxes left to right
  sort(ws_boxes.begin(), ws_boxes.end(), l);
}

//---------------------------------------------------------------------------//
// The first step in the algorithm proper is to split the page horizontally into
// a large number of equal-width thin strips. The exact number is specified by
// the DIVISIONS "magic number" specified in the header file. We just divide
// the page width by this number to get the strip width, then create vectors
// of the left edge and right edge of each strip to cycle through. For each
// strip, we look at every text element on the page and work out whether it
// will collide with our strip. We then take the tops and bottoms of each text
// element that collides and put them in order from the top to the bottom of
// the page. Assuming there is some whitespace at the top and bottom of a page,
// then if we have n objects that collide with our strip, we can define
// n + 1 boxes that lie within our strip but do not contain any text elements.
// If we repeat this for each strip on the page, we will have a set of boxes
// which cover all the whitespace on the page.

void Whitespace::makeStrips()
{
  // Find strip widths
  float pixwidth = m_page.width() / DIVISIONS;

  // The first strip starts at the left edge and stops one stripwidth to right
  float L_Edge = m_page.left;
  float R_Edge = L_Edge + pixwidth;

  // For each of these page divisions
  for(size_t i = 0; i < DIVISIONS; ++i)
  {
    // Create top/bottom bounds for boxes at top/bottom of page.
    vector<float> tops = {m_page.top};
    vector<float> bottoms = {m_page.bottom};

    // Now for each text element on the page
    for(const auto& j : WGO)
    {
      // If it obstructs our strip, store its upper and lower bounds
      if(j->left < R_Edge && j->right > L_Edge)
      {
        bottoms.push_back(j->bottom + j->size);
        tops.push_back(j->bottom);
      }
    }

    // Reverse sort the tops and bottoms
    sort(tops.begin(), tops.end(), greater<float>());
    sort(bottoms.begin(), bottoms.end(), greater<float>());

    // Now create boxes from our strip
    for(size_t j = 0; j < tops.size(); j++)
    {
      ws_boxes.emplace_back(WSbox{L_Edge, R_Edge, tops[j], bottoms[j], false});
    }

    // Move along to next strip.
    L_Edge = R_Edge;
    R_Edge += pixwidth;
  }
}

//---------------------------------------------------------------------------//
// Now we have covered all the whitespace, but we have an awful lot of tall
// thin boxes to look through to find edges that sit against a text element.
// Most edges will sit against other edges of whitespace boxes, and we want
// to merge these boxes as long as merging them forms a rectangle. We therefore
// look at every box's left edge. If it is identical in position and length to
// another box's right edge

void Whitespace::mergeStrips()
{
  size_t nboxes = ws_boxes.size();

  for(size_t i = 0; i < nboxes; ++i)
  {
    for(size_t j = i; j < nboxes; ++j)
    {
      // if its right edge matches the left edge of the compared box
      auto& m = ws_boxes[j];
      auto& c = ws_boxes[i];

      // If tested box is adjacent to test box, merge left edges and delete
      if(m.is_adjacent(c))
      {
        m.left = c.left;
        c.remove();
        break; // There can be only one match - skip to next iteration
      }

      // since boxes are ordered, if no adjacent boxes match this, skip to next
      if(m.left > c.right) break;
    }
  }
  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// We don't want to call the spacing between consecutive lines of a paragraph
// whitespace - this removes boxes that are not tall enough to count

void Whitespace::removeSmall()
{
  for(auto& i : ws_boxes)
  {
    // Remove only undeleted boxes who are not at the page border and are short
    if(!i.deletionFlag && !i.shares_edge(m_page) && i.height() < max_line_space)
    {
      i.remove();
    }
  }
  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// Now we have the whitespace we want, but it is actually the bits of the
// document that are not covered by whitespace that we need. We therefore need
// to find the edges of the whitespace boxes and trace around them. We'll do
// this by first identifying the vertices of the whitespace boxes. We then
// want to know which of the four directions NW, NE, SE, SW lying immediately
// adjacent contain whitespace. As a space-saving trick, we use bit flags
// for this - 1000 is NW, 0100 is NE, 0010 is SE, 0001 is SW. We can therefore
// use the | operator to combine the bits, so that 1111 would be a vertex
// surrounded by whitespace, whereas 0011 would have whitespace below it but
// not above it.

void Whitespace::makeVertices()
{
  for(auto& i : ws_boxes) // for each whitespace box
  {
    for(int j = 0; j < 4; j++) // for each of its four vertices
    {
      float x, y;        // empty x, y co-ordinates of vertex
      uint8_t flags = 0; // create an empty flag byte

      // We know at least one direction has whitespace by virtue of the
      // position of the vertex on the box, so we populate the flag with it,
      // along with the x, y co-ordinates of the vertex
      switch(j)
      {
        case 0 : x = i.left;  y = i.top;    flags = 0x02; break;
        case 1 : x = i.right; y = i.top;    flags = 0x01; break;
        case 2 : x = i.left;  y = i.bottom; flags = 0x04; break;
        case 3 : x = i.right; y = i.bottom; flags = 0x08; break;
      }

      // Now compare the other boxes to find neighbours and flag as needed
      for(auto& k : ws_boxes)
      {
        if(k.is_NW_of(x, y)) flags |= 0x08; // NW
        if(k.is_NE_of(x, y)) flags |= 0x04; // NE
        if(k.is_SE_of(x, y)) flags |= 0x02; // SE
        if(k.is_SW_of(x, y)) flags |= 0x01; // SW
        if(k.left > i.right) continue;
      }
      // Now we can create our vertex object and add it to the list
      vertices.emplace_back(Vertex{x, y, flags, None, None, 0, 0});
    }
  }
}

//---------------------------------------------------------------------------//
// Since we have a flag byte on the vertices with 4 unused bits, we use the
// high order bit as a deletion flag. Since we are only interested in
// vertices that are on corners (those in the middle of edges are redundant),
// and those on two opposite corners can't easily be defined, we are only
// interested in those with one or three compass directions containing
// whitespace. It turns out that this means we want to drop any whose flags
// are 0 modulo 3.

void Whitespace::tidyVertices()
{
  std::vector<Vertex> res;

  // For each vertex, if 1 or 3 corners have whitespace, push to result
  for(auto& i : vertices)
  {
    if((i.flags % 3) != 0)
    {
      res.push_back(i);
    }
  }

  // Swap rather than copy vector into vertices member
  swap(res, vertices);

  // Look up the implied direction of the edges that enter and exit the vertex
  for(auto& i : vertices)
  {
    i.In  = arrows[i.flags & 0x0f].first;
    i.Out = arrows[i.flags & 0x0f].second;
  }
}

//---------------------------------------------------------------------------//
// Now we have the vertices and the arrows pointing into and out of them,
// we can trace the polygons of which they form the perimeters. We take each
// vertex, find its out direction then follow where it points until we come to
// the next vertex on this line. By identifying the index of this vertex, we
// know to which vertex our test vertex points.

void Whitespace::tracePolygons()
{
  for(auto& i : vertices)
  {
    // Use the Direction enum as an int to get points beyond page edges
    float initialEdge = minbox[i.Out] + (2 * (i.Out / 2) - 1) * 100;

    // "edge" keeps track of the nearest vertex
    float edge = initialEdge;

    // Now for every other vertex
    for(auto& j : vertices)
    {
      if(i.is_closer_than(j, edge))
      {
        // i provisionally points to j
        i.points_to = &j - &(vertices[0]);

        // Now we update "edge" to make it the closest yet
        if(i.Out == North || i.Out == South)
        {
          edge = j.y;
        }
        else
        {
          edge = j.x;
        }
      }
    }
    // If the closest yet is not on the page, mark vertex for deletion
    if(eq(edge, initialEdge)) i.flags = i.flags | 0x80;
  }
}

//---------------------------------------------------------------------------//
// We can now start at any vertex and label it as being a member of our first
// polygon, copy it into a new vector then jump to the vertex it points to.
// If we then label this and push it into our new vector, jump to the next
// vertex and continue doing this, we will end up with a clockwise vector of
// vertices describing our polygon. We'll know we're back to the start if we
// come across a vertex labelled as a member of the current polygon vector.
// When this happens, we add our new polygon vector to a map with an appropriate
// label. We next iterate through our initial "master" vector of vertices until
// we come to an unflagged vertex. By setting a new label for our next polygon
// and following the same procedure as before, we identify the next polygon and
// so on until all the vertices in our master vector are flagged as belonging
// to a particular polygon, and we have a complete map of all the polygons
// on our page. This actually takes much longer to describe than it does to
// write the function...

void Whitespace::makePolygonMap()
{
  // we'll label our polygons with size_t's, but this is arbitrary
  size_t polygonNumber = 1;

  // For each vertex...
  for(size_t i = 0 ; i < vertices.size(); ++i)
  {
    // If this vertex is taken, move along.
    if(vertices[i].group != 0) continue;

    // we now know we're at the first unlabelled vertex
    size_t j = i;

    // While we're not back at the first vertex of our set
    while(vertices[j].group == 0)
    {
      // Label the vertex with polygon number
      vertices[j].group = polygonNumber;

      // If this is a new polygon number, start a new vector
      if(polygonMap.find(polygonNumber) == polygonMap.end())
      {
        polygonMap[polygonNumber] = {vertices[j]};
      }
      // Otherwise we append it to the set we are creating
      else
      {
        polygonMap[polygonNumber].push_back(vertices[j]);
      }

      // Our loop now jumps to next clockwise vertex
      j = vertices[j].points_to;
    }

    // we've come back to start of polygon - start a new one
    polygonNumber++;
  }
}

//---------------------------------------------------------------------------//
// Simple getter for the output vector

vector<pair<WSbox, vector<text_ptr>>> Whitespace::output() const
{
  return groups;
}

//---------------------------------------------------------------------------//
// Simple getter for the whitespace boxes - useful for debugging

std::vector<WSbox> Whitespace::ws_box_out() const
{
  return ws_boxes;
}

//---------------------------------------------------------------------------//
// Now we have our polygons. Perhaps the simplest way of dealing with them is
// to find their bounding rectangles. This does that by finding minmax x and y

void Whitespace::polygonMax()
{
  // we're going to recycle ws_boxes for our text boxes
  ws_boxes.clear();

  // For each polygon
  for(auto& i : polygonMap)
  {
    // Define floats that will shrink and invert to fit our text box
    float xmin = MAXPAGE, xmax = -MAXPAGE, ymin = MAXPAGE, ymax = -MAXPAGE;

    // Shrink and invert the edges of our bounding box
    for(auto& j : i.second)
    {
      if(j.x < xmin) xmin = j.x;  // move left edge to leftmost point
      if(j.x > xmax) xmax = j.x;  // move right edge to rightmost point
      if(j.y < ymin) ymin = j.y;  // move bottom edge to lowest point
      if(j.y > ymax) ymax = j.y;  // move top edge to highest point
    }

    // if the box is not the page itself, append this polygon to our result
    if(!(eq(xmin, m_page.left) && eq(xmax, m_page.right) &&
         eq(ymax, m_page.top) && eq(ymin, m_page.bottom)))
    {
      ws_boxes.push_back(WSbox{xmin, xmax, ymax, ymin, false});
    }
  }
}

//---------------------------------------------------------------------------//
// We have created a bunch of boxes containing text, but some boxes will
// completely contain other boxes. We want to remove the inner boxes to get our
// final set of content boxes.

void Whitespace::removeEngulfed()
{
  // For each box check whether another box engulfs it
  for(auto & i : ws_boxes)
  {
    for(auto & j : ws_boxes)
    {
      if(i.bottom >= j.bottom && i.top <= j.top &&
         i.left >= j.left && i.right <= j.right &&
         !(i.bottom == j.bottom && i.top == j.top &&
         i.left == j.left && i.right == j.right ))
      {
        i.remove(); // if so, mark for deletion
      }
    }
  }

  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// Finally we need to group our text items together in the text boxes for
// joining and analysis.

void Whitespace::groupText()
{
  for(auto& i : ws_boxes)
  {
    vector<text_ptr> textcontent;
    for(auto& j : WGO)
    {
      if(j->left >= i.left && j->right <= i.right &&
         j->bottom >= i.bottom && (j->bottom ) <= i.top &&
         !j->consumed)
        textcontent.push_back(j);
    }
    groups.emplace_back(pair<WSbox, vector<text_ptr>>(i, textcontent));
  }
}
