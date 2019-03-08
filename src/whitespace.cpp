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

Whitespace::Whitespace(const word_grouper& prsr): WG(prsr), WGO(WG.output())
{
  std::vector<float> fontsizes;
  for(auto& i : WGO) fontsizes.push_back(i.size);
  sort(fontsizes.begin(), fontsizes.end());
  midfontsize = fontsizes[fontsizes.size()/2];
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
}

//---------------------------------------------------------------------------//
// The Whitespace class contains four floats that specify the positions of the
// page edges. It gets these from the cropbox which has been passed to each
// class since the page object was created. This makes it a lot clearer which
// edge we are referring to instead of having to subscript a vector.

void Whitespace::pageDimensions()
{
  vector<float> pagebox = WG.getBox();
  pageleft   = pagebox[West];
  pageright  = pagebox[East];
  pagebottom = pagebox[South];
  pagetop    = pagebox[North];
}

//---------------------------------------------------------------------------//
// At various stages we need to clear away some of the whitespace boxes; for
// example, when we merge the initial strips into boxes, we need to drop most
// of the strips. This function allows us to keep only those boxes not flagged
// for deletion

void Whitespace::clearDeletedBoxes()
{
  ws_boxes.erase(std::remove_if(ws_boxes.begin(), ws_boxes.end(),
    [&](WSbox x)
    {
      return x.deletionFlag;
    }), ws_boxes.end());
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
  float pixwidth = (pageright - pageleft) / DIVISIONS; // find strip widths
  float L_Edge = pageleft;              // first strip starts at left edge
  float R_Edge = pageleft + pixwidth;  // and stops one stripwidth to right
  for(size_t i = 0; i < DIVISIONS; i++) // for each of our divisions
  {
    vector<float> tops = {pagetop};       // create top/bottom bounds for boxes.
    vector<float> bottoms = {pagebottom}; // First box starts at top of the page
    for(const auto& j : WGO)     // Now for each text element on the page
      if(j.left < R_Edge && j.right > L_Edge) // if it obstructs our strip,
      {
        bottoms.push_back(j.bottom + j.size);   // store its upper and lower
        tops.push_back(j.bottom);               // bounds as bottom and top of
      }                                         // whitespace boxes
    sort(tops.begin(), tops.end(), greater<float>()); // reverse sort the tops
    sort(bottoms.begin(), bottoms.end(), greater<float>()); // and the bottoms
    for(size_t j = 0; j < tops.size(); j++) // Now create boxes for our strip
      ws_boxes.emplace_back(WSbox{L_Edge, R_Edge, tops[j], bottoms[j], false});
    L_Edge = R_Edge;    // move along to next strip
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
  size_t nboxes = ws_boxes.size();  // count the boxes
  for(size_t i = 0; i < nboxes; i++) // for each box
    for(size_t j = i; j < nboxes; j++) // compare to every other box
    { // if its right edge matches the left edge of the compared box
      if(ws_boxes[j].left   == ws_boxes[i].right &&
         ws_boxes[j].top    == ws_boxes[i].top &&
         ws_boxes[j].bottom == ws_boxes[i].bottom)
      {
        ws_boxes[j].left = ws_boxes[i].left; // merge the left edges
        ws_boxes[i].deletionFlag = true; // ... and delete the leftmost box
        break; // There can be only one match - skip to next iteration
      }
      // since boxes are ordered, if no adjacent boxes match this, skip to next
      if(ws_boxes[j].left > ws_boxes[i].right) break;
    }
  clearDeletedBoxes(); // now we can remove boxes flagged for deletion
}

//---------------------------------------------------------------------------//
// We don't want to call the spacing between consecutive lines of a paragraph
// whitespace - this removes boxes that are not tall enough to count

void Whitespace::removeSmall()
{
  for(auto& i : ws_boxes) // for each box
    if(i.top != pagetop && i.bottom != pagebottom && // if not at the edge
       i.left != pageleft && i.right != pageright && // of a page and less than
       (i.top - i.bottom) < 0.3 * (midfontsize))     // a rule-of-thumb constant
      i.deletionFlag = true;                         // mark for deletion...
  clearDeletedBoxes();                               // ...and delete
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
      // we know at least one direction has whitespace by virtue of the
      // position of the vertex on the box, so we populate the flag with it,
      // along with the x, y co-ordinates of the vertex
      if(j == 0){x = i.left; y = i.top; flags = 0x02;} // top left vertex
      if(j == 1){x = i.right; y = i.top; flags = 0x01;} // top right vertex
      if(j == 2){x = i.left; y = i.bottom; flags = 0x04;} // bottom right vertex
      if(j == 3){x = i.right; y = i.bottom; flags = 0x08;} // bottom left vertex
      for(auto& k : ws_boxes) // now compare the other boxes to find neighbours
      {                       // and add the flags as needed
        if(k.right >= x && k.left < x && k.top > y && k.bottom <= y)
          flags = flags | 0x08; // NW
        if(k.right > x && k.left <= x && k.top > y && k.bottom <= y)
          flags = flags | 0x04; // NE
        if(k.right > x && k.left <= x && k.top >= y && k.bottom < y)
          flags = flags | 0x02; // SE
        if(k.right >= x && k.left < x && k.top >= y && k.bottom < y)
          flags = flags | 0x01; // SW
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
  std::vector<Vertex> res;  // temporary vector
  for(auto& i : vertices)   // for each vertex
    if((i.flags % 3) != 0)  // if not 0, 2 or 4 corners have whitespace
      res.push_back(i);     // add vertex to temporary vector
  swap(res, vertices);      // Swap temp vector into vertices
  for(auto& i : vertices)   // Look up the implied direction of the edges that
  {                         // enter and exit the vertex using "arrows" map
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
  for(auto& i : vertices) // for every vertex
  {
    vector<float> pagebox = WG.getBox(); // Allows us to get initialEdge
    // Use the Direction enum as an int to get points beyond page edges
    float initialEdge = pagebox[i.Out] + (2 * (i.Out / 2) - 1) * 100;
    float edge = initialEdge; // "edge" keeps track of the nearest vertex
    for(auto& j : vertices)  // now for every other vertex
      if((i.Out == North && j.x == i.x && // if North of North-pointing vertex
          j.In  == North && j.y >  i.y && j.y < edge) || // and closest yet or
         (i.Out == South && j.x == i.x && // South of South-pointing vertex
          j.In  == South && j.y <  i.y && j.y > edge) || // and closest yet or
         (i.Out == East  && j.y == i.y && // if East of East-pointing vertex
          j.In  == East  && j.x >  i.x && j.x < edge) ||// and closest yet or
         (i.Out == West  && j.y == i.y && // if West of West-pointing vertex
          j.In  == West  && j.x <  i.x && j.x > edge)  ) // and closest yet
      {
        i.points_to = &j - &(vertices[0]); // i provisionally points to j
        // Now we update "edge" to make it the closest yet
        if(i.Out == North || i.Out == South) edge = j.y; else edge = j.x;
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
  size_t polygonNumber = 1; // we'll label our polygons with size_t's
  for(size_t i = 0 ; i < vertices.size(); i++) // for each vertex...
  {
    if(vertices[i].group != 0) continue; // this vertex is taken. Move along...
    size_t j = i; // we're starting at the first unlabelled vertex
    while(vertices[j].group == 0) // while we're not back at the first vertex
    {
      vertices[j].group = polygonNumber; // label the vertex with polygon #
      // if this is a new polygon number that we haven't mapped yet...
      if(polygonMap.find(polygonNumber) == polygonMap.end())
        polygonMap[polygonNumber] = {vertices[j]}; // start a new vector
      else
        polygonMap[polygonNumber].push_back(vertices[j]); // else add to current
      j = vertices[j].points_to; // now our loop jumps to next clockwise vertex
    }
    polygonNumber++; // we've come back to start of polygon - start a new one
  }
}

//---------------------------------------------------------------------------//
// Simple getter for the polygon map

unordered_map<size_t, vector<Vertex>> Whitespace::output() const
{
  return polygonMap;
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
  ws_boxes.clear(); // we're going to recycle ws_boxes for our text boxes
  for(auto& i : polygonMap)
  {
    // define floats that will shrink and invert to fit our text box
    float xmin = MAXPAGE, xmax = -MAXPAGE, ymin = MAXPAGE, ymax = -MAXPAGE;
    for(auto& j : i.second)
    {
      if(j.x < xmin) xmin = j.x;  // move left edge to leftmost point
      if(j.x > xmax) xmax = j.x;  // move right edge to rightmost point
      if(j.y < ymin) ymin = j.y;  // move bottom edge to lowest point
      if(j.y > ymax) ymax = j.y;  // move top edge to highest point
    }
    if(!(eq(xmin, pageleft) && eq(xmax, pageright) &&
       eq(ymax, pagetop) && eq(ymin, pagebottom))) // if box != page itself
      ws_boxes.push_back(WSbox{xmin, xmax, ymax, ymin, false}); // create box
  }
}

//---------------------------------------------------------------------------//
// We have created a bunch of boxes containing text, but some boxes will
// completely contain other boxes. We want to remove the inner boxes to get our
// final set of content boxes.

void Whitespace::removeEngulfed()
{
  for(auto & i : ws_boxes)  // for each box
    for(auto & j : ws_boxes) // check whether another box engulfs it
      if(i.bottom >= j.bottom && i.top <= j.top &&
         i.left >= j.left && i.right <= j.right &&
         !(i.bottom == j.bottom && i.top == j.top &&
         i.left == j.left && i.right == j.right ))
        i.deletionFlag = true; // if so, mark for deletion
  clearDeletedBoxes(); // deleted boxes marked for deletion
}
