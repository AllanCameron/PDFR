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

Whitespace::Whitespace(const word_grouper& prsr): WG(prsr)
{
  pageDimensions();
  makeStrips();
  mergeStrips();
  removeSmall();
  removeInvaginations();
  removeSingletons();
  makeVertices();
  tidyVertices();
  tracePolygons();
  makePolygonMap();
  polygonMax();
}

//---------------------------------------------------------------------------//
// The Whitespace class contains four floats that specify the positions of the
// page edges. It gets these from the cropbox which has been passed to each
// class since the page object was created. This makes it a lot clearer which
// edge we are referring to instead of having to subscript a vector.

void Whitespace::pageDimensions()
{
  vector<float> pagebox = WG.getBox();
  pageleft   = pagebox[0];
  pageright  = pagebox[2];
  pagebottom = pagebox[1];
  pagetop    = pagebox[3];
}

//---------------------------------------------------------------------------//
// At various stages we need to clear away some of the whitespace boxes; for
// example, when we merge the initial strips into boxes, we need to drop most
// of the strips. This function allows us to keep only those boxes not flagged
// for deletion

void Whitespace::clearDeletedBoxes()
{
  std::vector<WSbox> res; // create new vector for non-flagged boxes
  for(auto& i : ws_boxes) // for each box
    if(!i.deletionFlag)   // if it is not flagged for deletion
      res.push_back(i);   // add it to our new vector
  swap(res, ws_boxes);    // swap our new vector with the original
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
  std::vector<textrow> WGO = WG.output();
  std::vector<float> fontsizes;
  for(auto& i : WGO) fontsizes.push_back(i.size);
  sort(fontsizes.begin(), fontsizes.end());
  midfontsize = fontsizes[fontsizes.size()/2];
  size_t N_elements = WGO.size();
  float pixwidth = (pageright - pageleft) / DIVISIONS;
  std::vector<float> leftEdges {pageleft};
  std::vector<float> rightEdges {pageleft + pixwidth};
  while(leftEdges.size() < DIVISIONS)
  {
    leftEdges.push_back(rightEdges.back());
    rightEdges.push_back(rightEdges.back() + pixwidth);
  }
  for(size_t i = 0; i < DIVISIONS; i++)
  {
    vector<float> topboundaries, bottomboundaries;
    topboundaries = {pagetop};
    for(size_t j = 0; j < N_elements; j++)
    {
      if(WGO[j].left < rightEdges[i] && WGO[j].right > leftEdges[i])
      {
        bottomboundaries.push_back(WGO[j].bottom + WGO[j].size);
        topboundaries.push_back(WGO[j].bottom);
      }
    }
    bottomboundaries.push_back(pagebottom);
    sort(topboundaries.begin(), topboundaries.end(), greater<float>());
    sort(bottomboundaries.begin(), bottomboundaries.end(), greater<float>());
    for(size_t j = 0; j < topboundaries.size(); j++)
      ws_boxes.emplace_back(WSbox{leftEdges[i], rightEdges[i],
                                  topboundaries[j], bottomboundaries[j], false});
  }
}
/*
void Whitespace::makeStrips()
{
  float stripwidth = (pageright - pageleft)/ DIVISIONS; // calculate strip width
  std::vector<textrow> WGO = WG.output();   // get our text elements
  float L_Edge = pageleft;    // first strip starts at left edge
  float R_Edge = pageleft + stripwidth; // and stops one stripwidth to right
  while(R_Edge <= pageright) // while we are still on the page...
  {
    vector<float> tops, bottoms;        // create top/bottom bounds for boxes.
    tops = {pagetop};                   // First box starts at top of the page.
    for(auto& i : WGO)                        // Now for each text element on
      if(i.left < R_Edge && i.right > L_Edge) // the page...
      {                                       // if it obstructs our strip,
        bottoms.push_back(i.bottom + i.size); // store its upper and lower
        tops.push_back(i.bottom);             // bounds as bottom and top of
      }                                       // whitespace boxes
    bottoms.push_back(pagebottom);      // Last box ends at bottom of page.
    sort(tops.begin(), tops.end(), greater<float>()); // Sort all the tops
    sort(bottoms.begin(), bottoms.end(), greater<float>()); // and the bottoms
    for(size_t i = 0; i < tops.size(); i++) // Now create boxes for our strip
      ws_boxes.emplace_back(WSbox{L_Edge, R_Edge, tops[i], bottoms[i], false});
    L_Edge = R_Edge;      // Move to our next strip by incrementing the
    R_Edge += stripwidth; // left and right edges by the strip width
  }
}
*/
//---------------------------------------------------------------------------//
// Now we have covered all the whitespace, but we have an awful lot of tall
// thin boxes to look through to find edges that sit against a text element.
// Most edges will sit against other edges of whitespace boxes, and we want
// to merge these boxes as long as merging them forms a rectangle. We therefore
// look at every box's left edge. If it is identical in position and length to
// another box's right edge

void Whitespace::mergeStrips()
{
  for(auto& i : ws_boxes) // for each box
  {
    for(auto& j :ws_boxes) // compare to every other box
    { // if its right edge matches the left edge of the compared box
      if(j.left == i.right && j.top == i.top && j.bottom == i.bottom)
      {
        j.left = i.left; // merge the two by making the left edges match...
        i.deletionFlag = true; // ... and deleting the leftmost box
        break;
      }
    }
  }
  clearDeletedBoxes();
}

//---------------------------------------------------------------------------//

void Whitespace::removeSmall()
{
  for(auto& i : ws_boxes)
    if(i.top != pagetop && i.bottom != pagebottom &&
       i.left != pageleft && i.right != pageright &&
       (i.top - i.bottom) < 0.7 * (midfontsize)) i.deletionFlag = true;

  clearDeletedBoxes();
}

//---------------------------------------------------------------------------//

void Whitespace::removeInvaginations()
{
  for(auto& row : ws_boxes)
  {
    bool hasLeftMatch, hasRightMatch;
    hasLeftMatch = hasRightMatch = false;
    shared_ptr<WSbox> leftmatch, rightmatch;
    for(auto& maybe : ws_boxes)
    {
      if(maybe.right == row.left &&
         (maybe.top == row.top || maybe.bottom == row.bottom))
      {
        leftmatch = std::make_shared<WSbox>(maybe);
        hasRightMatch = true;
      }
      if(maybe.left == row.right &&
         (maybe.top == row.top || maybe.bottom == row.bottom))
      {
        rightmatch = std::make_shared<WSbox>(maybe);
        hasRightMatch = true;
      }
    }
    if(hasLeftMatch && hasRightMatch)
    {
      if((row.right - row.left) < (1.5 * midfontsize) &&
         (row.top - row.bottom) > (leftmatch->top - leftmatch->bottom) &&
          rightmatch->top == leftmatch->bottom &&
          rightmatch->bottom == leftmatch->top)
      {
        rightmatch->left = leftmatch->left;
        row.deletionFlag = true;
        leftmatch->deletionFlag = true;
      }
    }
  }
  clearDeletedBoxes();
}

//---------------------------------------------------------------------------//

void Whitespace::removeSingletons()
{
  for(auto& i : ws_boxes)
    for(auto& j : ws_boxes)
      if((j.bottom == i.top && j.left < i.right && j.right > i.left) ||
         (i.bottom == j.top && j.left < i.right && j.right > i.left) ||
         (j.left == i.right && j.bottom < i.top && j.top > i.bottom) ||
         (i.left == j.right && j.bottom < i.top && j.top > i.bottom))
          i.deletionFlag = false; else i.deletionFlag = true;
  //clearDeletedBoxes();
}

//---------------------------------------------------------------------------//

void Whitespace::makeVertices()
{
  for(auto& i : ws_boxes)
  {
    for(int j = 0; j < 4; j++)
    {
      float x, y;
      uint8_t flags = 0;
      if(j == 0){x = i.left; y = i.top; flags = 0x02;}
      if(j == 1){x = i.right; y = i.top; flags = 0x01;}
      if(j == 2){x = i.left; y = i.bottom; flags = 0x04;}
      if(j == 3){x = i.right; y = i.bottom; flags = 0x08;}
      for(auto& k : ws_boxes)
      {
        if(k.right >= x && k.left < x && k.top > y && k.bottom <= y)
          flags = flags | 0x08;
        if(k.right > x && k.left <= x && k.top > y && k.bottom <= y)
          flags = flags | 0x04;
        if(k.right > x && k.left <= x && k.top >= y && k.bottom < y)
          flags = flags | 0x02;
        if(k.right >= x && k.left < x && k.top >= y && k.bottom < y)
          flags = flags | 0x01;
      }
      vertices.push_back(Vertex{x, y, flags, None, None, 0, 0});
    }
  }
}

//---------------------------------------------------------------------------//

void Whitespace::tidyVertices()
{
  for(auto& i : vertices) // remove if 0, 2 adjacent or 4 sides
    if(i.flags % 3 == 0) i.flags = i.flags | 0x80;
  std::vector<Vertex> res;
  for(auto& i : vertices)
    if((i.flags & 0x80) == 0) res.push_back(i);
  swap(res, vertices);
  for(auto& i : vertices)
  {
    i.InDir = arrows[i.flags & 0x0f].first;
    i.OutDir = arrows[i.flags & 0x0f].second;
  }
}

//---------------------------------------------------------------------------//

void Whitespace::tracePolygons()
{
  for(auto& i : vertices)
  {
    switch(i.OutDir)
    {
    case North: {float min_y = pagetop + 100.0; size_t bestMatch = 0;
                for(size_t j = 0; j < vertices.size(); j++)
                {
                  if(eq(vertices[j].x, i.x) &&
                     vertices[j].InDir == North &&
                     vertices[j].y > i.y && vertices[j].y < min_y)
                    {
                      bestMatch = j;
                      min_y = vertices[j].y;
                    }
                }
                if(!eq(min_y, pagetop + 100.0)) i.points_to = bestMatch;
                else i.flags = i.flags | 0x80;
                break;}
    case South: {float max_y = pagebottom - 100.0; size_t bestMatch = 0;
                for(size_t j = 0; j < vertices.size(); j++)
                {
                  if(eq(vertices[j].x, i.x) &&
                     vertices[j].InDir == South &&
                     vertices[j].y < i.y && vertices[j].y > max_y)
                    {
                      bestMatch = j;
                      max_y = vertices[j].y;
                    }
                }
                if(!eq(max_y, pagebottom - 100.0)) i.points_to = bestMatch;
                else i.flags = i.flags | 0x80;
                break;}
    case East:  {float min_x = pageright + 100.0; size_t bestMatch = 0;
                for(size_t j = 0; j < vertices.size(); j++)
                {
                  if(eq(vertices[j].y, i.y) &&
                     vertices[j].InDir == East &&
                     vertices[j].x > i.x && vertices[j].x < min_x)
                    {
                      bestMatch = j ;
                      min_x = vertices[j].x;
                    }
                }
                if(!eq(min_x, pageright + 100.0)) i.points_to = bestMatch;
                else i.flags = i.flags | 0x80;
                break;}
    case West:  {float max_x = pageleft - 100.0; size_t bestMatch = 0;
                for(size_t j = 0; j < vertices.size(); j++)
                {
                  if(eq(vertices[j].y, i.y) &&
                     vertices[j].InDir == West &&
                     vertices[j].x < i.x && vertices[j].x > max_x)
                    {
                      bestMatch = j;
                      max_x = vertices[j].x;
                    }
                }
                if(!eq(max_x, pageleft - 100.0)) i.points_to = bestMatch;
                else i.flags = i.flags | 0x80;
                break;}
    default:    i.flags = i.flags | 0x80;
    }
  }
}


//---------------------------------------------------------------------------//

void Whitespace::makePolygonMap()
{
  size_t polygonNumber = 1;
  for(size_t i = 0 ; i < vertices.size(); i++)
  {
    if(vertices[i].group != 0) continue;
    size_t j = i;
    while(vertices[j].group == 0)
    {
      vertices[j].group = polygonNumber;
      if(polygonMap.find(polygonNumber) == polygonMap.end())
        polygonMap[polygonNumber] = {vertices[j]};
      else
        polygonMap[polygonNumber].push_back(vertices[j]);
      j = vertices[j].points_to;
    }
    polygonNumber++;
  }
}

//---------------------------------------------------------------------------//

unordered_map<size_t, vector<Vertex>> Whitespace::output() const
{
  return polygonMap;
}

//---------------------------------------------------------------------------//

std::vector<WSbox> Whitespace::ws_box_out() const
{
  return ws_boxes;
}

//---------------------------------------------------------------------------//

void Whitespace::polygonMax()
{
  ws_boxes.clear();
  for(auto& i : polygonMap)
  {
    float xmin, xmax, ymin, ymax;
    xmin = ymin = 30000.0;
    xmax = ymax = -30000.0;
    for(auto& j : i.second)
    {
      if(j.x < xmin) xmin = j.x;
      if(j.x > xmax) xmax = j.x;
      if(j.y < ymin) ymin = j.y;
      if(j.y > ymax) ymax = j.y;
    }
    if(eq(xmin, pageleft) && eq(xmax, pageright) &&
       eq(ymax, pagetop) && eq(ymin, pagebottom)) continue;
    else ws_boxes.push_back(WSbox{xmin, xmax, ymax, ymin, false});
  }
  for(auto & i : ws_boxes)
    for(auto & j : ws_boxes)
      if(i.bottom >= j.bottom && i.top <= j.top &&
         i.left >= j.left && i.right <= j.right &&
         !(i.bottom == j.bottom && i.top == j.top &&
         i.left == j.left && i.right == j.right )) i.deletionFlag = true;
  clearDeletedBoxes();
}
