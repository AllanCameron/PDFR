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

bool Whitespace::eq(float a, float b)
{
  float d = b - a;
  return (d > (-0.1) && d < 0.1);
}
//---------------------------------------------------------------------------//

Whitespace::Whitespace(const parser& prsr): GS(prsr)
{
  vector<float> pagebox = GS.getminbox();
  pageleft = pagebox[0];
  pageright = pagebox[2];
  pagebottom = pagebox[1];
  pagetop = pagebox[3];
  pagewidth = pageright - pageleft;
  pixwidth = pagewidth / DIVISIONS;
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

void Whitespace::clearDeletedBoxes()
{
  std::vector<WSbox> res;
  for(auto& i : ws_boxes)
    if(!i.deletionFlag) res.push_back(i);
  swap(res, ws_boxes);
}

//---------------------------------------------------------------------------//

void Whitespace::makeStrips()
{
  GSoutput* GSO = GS.output();
  size_t N_elements = GSO->left.size();
  std::vector<float> leftEdges {pageleft};
  std::vector<float> rightEdges {pageleft + pixwidth};
  std::vector<float> fontsizes = GSO->size;
  sort(fontsizes.begin(), fontsizes.end());
  minfontsize = fontsizes[0];
  maxfontsize = fontsizes.back();
  midfontsize = fontsizes[fontsizes.size()/2];
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
      if(GSO->left[j] < rightEdges[i] && GSO->right[j] > leftEdges[i])
      {
        bottomboundaries.push_back(GSO->bottom[j] + GSO->size[j]);
        topboundaries.push_back(GSO->bottom[j]);
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

//---------------------------------------------------------------------------//

void Whitespace::mergeStrips()
{
  for(auto& i : ws_boxes)
  {
    for(auto& j :ws_boxes)
    {
      if(j.left == i.right && j.top == i.top && j.bottom == i.bottom)
      {
        j.left = i.left;
        i.deletionFlag = true;
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


std::vector<WSbox> Whitespace::ws_box_out() const
{
  return ws_boxes;
}


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
