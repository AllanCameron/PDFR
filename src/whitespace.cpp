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

using namespace std;

static const float MAXPAGE = 30000;
static const float MAX_LINE_FACTOR = 0.3;

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

Whitespace::Whitespace(TextBox word_grouper_output):
  m_page_text(move(word_grouper_output))
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
}


//---------------------------------------------------------------------------//

void Whitespace::getMaxLineSize()
{
  std::vector<float> fontsizes;
  for(auto& i : m_page_text)
  {
    fontsizes.push_back(i->GetSize());
  }
  sort(fontsizes.begin(), fontsizes.end());
  max_line_space = fontsizes[fontsizes.size()/2] * MAX_LINE_FACTOR;
}

//---------------------------------------------------------------------------//
// The Whitespace class contains four floats that specify the positions of the
// page edges. It gets these from the cropbox which has been passed to each
// class since the page object was created. This makes it a lot clearer which
// edge we are referring to instead of having to subscript a vector.

void Whitespace::pageDimensions()
{
  for(auto& i : m_page_text)
  {
    if(i->GetRight() > m_page_text.GetRight())
    {
      m_page_text.SetRight(m_page_text.GetRight() + 10.0);
    }

    if(i->GetLeft() < m_page_text.GetLeft())
    {
      m_page_text.SetLeft(m_page_text.GetLeft() - 10.0);
    }

    if((i->GetBottom() + i->GetSize()) > m_page_text.GetTop())
    {
      m_page_text.SetTop(m_page_text.GetTop() + 10.0);
    }

    if(i->GetBottom() < m_page_text.GetBottom())
    {
      m_page_text.SetBottom(m_page_text.GetBottom() - 10.0);
    }
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
  auto del = [&](Box lambda_box){return lambda_box.IsConsumed();};

  // Define a lambda to sort boxes left to right
  auto l = [](const Box& lambda_box_lhs, const Box& lambda_box_rhs) -> bool {
    return lambda_box_lhs.GetLeft() < lambda_box_rhs.GetLeft();
    };

  // Define a lambda to sort boxes top to bottom
  auto t = [](const Box& lambda_box_lhs, const Box& lambda_box_rhs) -> bool {
    return lambda_box_rhs.GetTop() < lambda_box_lhs.GetTop();
    };

  // Move boxes for deletion to back of vector, starting at returned iterator
  auto junk = remove_if(m_boxes.begin(), m_boxes.end(), del);

  // Erase the back of the vector containing the boxes flagged for deletion
  m_boxes.erase(junk, m_boxes.end());

  // Sort the remaining boxes top to bottom then left to right
  sort(m_boxes.begin(), m_boxes.end(), t);
  stable_sort(m_boxes.begin(), m_boxes.end(), l);
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
  float pixwidth = m_page_text.Width() / DIVISIONS;

  // The first strip starts at the left edge and stops one stripwidth to right
  float L_Edge = m_page_text.GetLeft();
  float R_Edge = L_Edge + pixwidth;

  // For each of these page divisions
  for (size_t i = 0; i < DIVISIONS; ++i)
  {
    // Create top/bottom bounds for boxes at top/bottom of page.
    vector<float> tops = {m_page_text.GetTop()};
    vector<float> bottoms = {m_page_text.GetBottom()};

    // Now for each text element on the page
    for(const auto& j : m_page_text)
    {
      // If it obstructs our strip, store its upper and lower bounds
      if(j->GetLeft() < R_Edge && j->GetRight() > L_Edge)
      {
        bottoms.push_back(j->GetTop());
        tops.push_back(j->GetBottom());
      }
    }

    // Reverse sort the tops and bottoms
    sort(tops.begin(), tops.end(), greater<float>());
    sort(bottoms.begin(), bottoms.end(), greater<float>());

    // Now create boxes from our strip
    for(size_t j = 0; j < tops.size(); j++)
    {
      m_boxes.emplace_back(Box(L_Edge, R_Edge, tops[j], bottoms[j]));
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
  for(auto left_box = m_boxes.begin(); left_box != m_boxes.end(); ++left_box)
  {
    for(auto right_box = left_box; right_box != m_boxes.end(); ++right_box)
    {
      // Since boxes are ordered, if right_box is beyond left_box, break inner
      // loop and move to next left_box
      if(right_box->IsBeyond(*left_box)) break;

      // If tested box is adjacent to test box, merge left edges and delete
      if(right_box->IsAdjacent(*left_box))
      {
        right_box->Merge(*left_box);
        break; // There can be only one match - skip to next left_box
      }
    }
  }
  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// We don't want to call the spacing between consecutive lines of a paragraph
// whitespace - this removes boxes that are not tall enough to count

void Whitespace::removeSmall()
{
  for(auto& box : m_boxes)
  {
    // Remove only undeleted boxes who are not at the page border and are short
    if(!box.IsConsumed()            &&
       !box.SharesEdge(m_page_text) &&
       box.Height() < max_line_space  )
    {
      box.Consume();
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
   // For each whitespace box
  for(auto& owner_box : m_boxes)
  {
    // For each of its four vertices
    for(int corner_number = 0; corner_number < 4; ++corner_number)
    {
      // Create a vertex shared pointer object at this corner
      auto this_corner = owner_box.GetVertex(corner_number);

      // Now compare the other boxes to find neighbours and flag as needed
      for(auto& other_box : m_boxes)
      {
        other_box.RecordImpingementOn(*this_corner);

        // Since m_boxes are sorted, skip to next corner if no effect possible.
        if(other_box.IsBeyond(owner_box)) continue;
      }
      // Now we can push our vertex to the list
      m_vertices.push_back(this_corner);
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
  vector<shared_ptr<Vertex>> tmp_vertices;

  // For each vertex, if 1 or 3 corners have whitespace, push to result
  for(auto& corner : m_vertices)
  {
    if((corner->flags_ % 3) != 0)
    {
      tmp_vertices.push_back(corner);
    }
  }

  // Swap rather than copy vector into m_vertices member
  swap(tmp_vertices, m_vertices);
}

//---------------------------------------------------------------------------//
// Now we have the vertices and the arrows pointing into and out of them,
// we can trace the polygons of which they form the perimeters. We take each
// vertex, find its out direction then follow where it points until we come to
// the next vertex on this line. By identifying the index of this vertex, we
// know to which vertex our test vertex points.

void Whitespace::tracePolygons()
{
  for(auto& i : m_vertices)
  {
    // Use the Direction enum as an int to get points beyond page edges
    float outer_edge = m_page_text.edge(i->Out()) + (2*(i->Out()/2) - 1) * 100;

    // "edge" keeps track of the nearest vertex
    float edge = outer_edge;

    // Now for every other vertex
    for(auto& j : m_vertices)
    {
      if(i->IsCloserThan(*j, edge))
      {
        // i provisionally points to j
        i->points_to_ = &j - &(m_vertices[0]);

        // Now we update "edge" to make it the closest yet
        if(i->Out() == North || i->Out() == South)
        {
          edge = j->y_;
        }
        else
        {
          edge = j->x_;
        }
      }
    }
    // If the closest yet is not on the page, mark vertex for deletion
    if(edge - outer_edge < 0.1 && outer_edge - edge < 0.1) i->flags_ |= 0x80;
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
  for(size_t i = 0 ; i < m_vertices.size(); ++i)
  {
    // If this vertex is taken, move along.
    if(m_vertices[i]->group_) continue;

    // we now know we're at the first unlabelled vertex
    size_t j = i;

    // While we're not back at the first vertex of our set
    while (m_vertices[j]->group_ == 0)
    {
      // Label the vertex with polygon number
      m_vertices[j]->group_ = polygonNumber;

      // If this is a new polygon number, start a new vector
      if(polygonMap.find(polygonNumber) == polygonMap.end())
      {
        polygonMap[polygonNumber] = {m_vertices[j]};
      }
      // Otherwise we append it to the set we are creating
      else polygonMap[polygonNumber].push_back(m_vertices[j]);

      // Our loop now jumps to next clockwise vertex
      j = m_vertices[j]->points_to_;
    }

    // we've come back to start of polygon - start a new one
    polygonNumber++;
  }
}

//---------------------------------------------------------------------------//
// Simple getter for the whitespace boxes - useful for debugging

std::vector<Box> Whitespace::ws_box_out() const
{
  return m_boxes;
}

//---------------------------------------------------------------------------//
// Now we have our polygons. Perhaps the simplest way of dealing with them is
// to find their bounding rectangles. This does that by finding minmax x and y.
// Clearly, we don't want the entire page to be a bounding box, as this will
// go on to engulf all the other boxes. Therefore if we find that a box matches
// the page box, we don't include this.

void Whitespace::polygonMax()
{
  // we're going to recycle m_boxes for our text boxes
  m_boxes.clear();

  // For each polygon
  for(auto& shape : polygonMap)
  {
    // Define floats that will shrink and invert to fit our text box
    Box bounding_box(MAXPAGE, -MAXPAGE, -MAXPAGE, MAXPAGE);

    // Shrink and invert the edges of our bounding box
    for(auto& corner : shape.second)
    {
      bounding_box.ExpandBoxToIncludeVertex(*corner);
    }

    // if the box is not the page itself, append this polygon to our result
    if(!bounding_box.IsApproximatelySameAs(m_page_text))
    {
      m_boxes.emplace_back(move(bounding_box));
    }
  }

  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// We have created a bunch of boxes containing text, but some boxes will
// completely contain other boxes. We want to remove the inner boxes to get our
// final set of content boxes.

void Whitespace::removeEngulfed()
{
  // For each box check whether it engulfs another box
  for(auto outer = m_boxes.begin(); outer != m_boxes.end(); ++outer)
  {
    if(outer->IsConsumed()) continue;

    for(auto inner = outer; inner != m_boxes.end(); ++inner)
    {
      if(inner->IsBeyond(*outer)) break;
      if(inner->IsConsumed())     continue;
      if(outer->Engulfs(*inner))   inner->Consume();
    }
  }

  cleanAndSortBoxes();
}

//---------------------------------------------------------------------------//
// Finally we need to group our text items together in the text boxes for
// joining and analysis.

vector<TextBox> Whitespace::output()
{
  vector<TextBox> res;
  for(auto& box : m_boxes)
  {
    vector<TextPointer> text_vec;
    int start_at = 0;
    for(auto text_it = m_page_text.begin() + start_at;
             text_it != m_page_text.end(); ++text_it)
    {
      if (box.Engulfs(**text_it) && !(*text_it)->IsConsumed())
      {
        text_vec.push_back(*text_it);
        start_at = distance(m_page_text.begin(), text_it);
      }
      if((*text_it)->IsBeyond(box)) break;
    }
    res.emplace_back(TextBox(move(text_vec), move(box)));
  }

  return res;
}
