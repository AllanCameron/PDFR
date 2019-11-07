//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Whitespace implementation file                                      //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "whitespace.h"

using namespace std;

//---------------------------------------------------------------------------//

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

Whitespace::Whitespace(std::unique_ptr<TextBox> p_word_grouper_output):
  text_box_(move(p_word_grouper_output))
{
  PageDimensions_();
  MakeStrips_();
  MergeStrips_();
  RemoveSmall_();
  MakeVertices_();
  TidyVertices_();
  TracePolygons_();
  MakePolygonMap_();
  PolygonMax_();
  RemoveEngulfed_();
}



//---------------------------------------------------------------------------//
// The Whitespace class contains four floats that specify the positions of the
// page edges. It gets these from the cropbox which has been passed to each
// class since the page object was created. This makes it a lot clearer which
// edge we are referring to instead of having to subscript a vector.

void Whitespace::PageDimensions_()
{
  auto page_right  = text_box_->GetRight();
  auto page_left   = text_box_->GetLeft();
  auto page_top    = text_box_->GetTop();
  auto page_bottom = text_box_->GetBottom();

  for (auto& element : *text_box_)
  {

    if (element->GetRight() > page_right)
    {
      text_box_->SetRight(page_right + 10.0);
    }

    if (element->GetLeft() < page_left)
    {
      text_box_->SetLeft(page_left - 10.0);
    }

    if ((element->GetBottom() + element->GetSize()) > page_top)
    {
      text_box_->SetTop(page_top + 10.0);
    }

    if (element->GetBottom() < page_bottom)
    {
      text_box_->SetBottom(page_bottom - 10.0);
    }
  }
}

//---------------------------------------------------------------------------//
// At various stages we need to clear away some of the whitespace boxes; for
// example, when we merge the initial strips into boxes, we need to drop most
// of the strips. This function allows us to keep only those boxes not flagged
// for deletion

void Whitespace::CleanAndSortBoxes_()
{
  // Define a lambda to identify boxes flagged for deletion
  auto del =  [&](const Box& lambda_box) -> bool
              {
                return lambda_box.IsConsumed();
              };

  // Define a lambda to sort boxes left to right
  auto hsort =  [](const Box& lambda_box_lhs, const Box& lambda_box_rhs) -> bool
            {
              return lambda_box_lhs.GetLeft() < lambda_box_rhs.GetLeft();
            };

  // Define a lambda to sort boxes top to bottom
  auto vsort =  [](const Box& lambda_box_lhs, const Box& lambda_box_rhs) -> bool
            {
              return lambda_box_rhs.GetTop() < lambda_box_lhs.GetTop();
            };

  // Move boxes for deletion to back of vector, starting at returned iterator
  auto junk = remove_if(boxes_.begin(), boxes_.end(), del);

  // Erase the back of the vector containing the boxes flagged for deletion
  boxes_.erase(junk, boxes_.end());

  // Sort the remaining boxes top to bottom then left to right
  sort(boxes_.begin(), boxes_.end(), vsort);
  stable_sort(boxes_.begin(), boxes_.end(), hsort);
}

//---------------------------------------------------------------------------//
// The first step in the algorithm proper is to split the page horizontally into
// a large number of equal-width thin strips. The exact number is specified by
// the DIVISIONS_ "magic number" specified in the header file. We just divide
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

void Whitespace::MakeStrips_()
{
  // Find strip widths
  float pixwidth = text_box_->Width() / DIVISIONS_;

  // The first strip starts at the left edge and stops one stripwidth to right
  float L_Edge = text_box_->GetLeft();
  float R_Edge = L_Edge + pixwidth;

  // For each of these page divisions
  for (size_t i = 0; i < DIVISIONS_; ++i)
  {
    // Create top/bottom bounds for boxes at top/bottom of page.
    vector<float> tops    = {text_box_->GetTop()};
    vector<float> bottoms = {text_box_->GetBottom()};

    // Now for each text element on the page
    for (const auto& j : *text_box_)
    {
      // If it obstructs our strip, store its upper and lower bounds
      if (j->GetLeft() < R_Edge && j->GetRight() > L_Edge)
      {
        bottoms.push_back(j->GetTop());
        tops.push_back(j->GetBottom());
      }
    }

    // Reverse sort the tops and bottoms
    sort(tops.begin(), tops.end(), greater<float>());
    sort(bottoms.begin(), bottoms.end(), greater<float>());

    // Now create boxes from our strip
    for (size_t j = 0; j < tops.size(); j++)
    {
      boxes_.emplace_back(Box(L_Edge, R_Edge, tops[j], bottoms[j]));
    }

    // Move along to next strip.
    L_Edge  = R_Edge;
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

void Whitespace::MergeStrips_()
{
  for (auto left_box = boxes_.begin(); left_box != boxes_.end(); ++left_box)
  {
    for (auto right_box = left_box; right_box != boxes_.end(); ++right_box)
    {
      // Since boxes are ordered, if right_box is beyond left_box, break inner
      // loop and move to next left_box
      if (right_box->IsBeyond(*left_box)) break;

      // If tested box is adjacent to test box, merge left edges and delete
      if (right_box->IsAdjacent(*left_box))
      {
        right_box->Merge(*left_box);
        break; // There can be only one match - skip to next left_box
      }
    }
  }

  CleanAndSortBoxes_();
}

//---------------------------------------------------------------------------//
// We don't want to call the spacing between consecutive lines of a paragraph
// whitespace - this removes boxes that are not tall enough to count

void Whitespace::RemoveSmall_()
{
  std::vector<float> font_sizes;

  for (auto& element : *text_box_) font_sizes.push_back(element->GetSize());

  sort(font_sizes.begin(), font_sizes.end());
  float max_line_space = font_sizes[font_sizes.size()/2] * MAX_LINE_FACTOR;

  for (auto& box : boxes_)
  {
    // Remove only undeleted boxes who are not at the page border and are short
    if (!box.IsConsumed()              &&
       !box.SharesEdge(*text_box_)     &&
       box.Height() < max_line_space  )
    {
      box.Consume();
    }
  }

  CleanAndSortBoxes_();
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

void Whitespace::MakeVertices_()
{
   // For each whitespace box
  for (auto& owner_box : boxes_)
  {
    // For each of its four vertices
    for (int corner_number = 0; corner_number < 4; ++corner_number)
    {
      // Create a vertex shared pointer object at this corner
      auto this_corner = owner_box.GetVertex(corner_number);

      // Now compare the other boxes to find neighbours and flag as needed
      for (auto& other_box : boxes_)
      {
        other_box.RecordImpingementOn(*this_corner);

        // Since boxes_ are sorted, skip to next corner if no effect possible.
        if (other_box.IsBeyond(owner_box)) continue;
      }
      // Now we can push our vertex to the list
      vertices_.push_back(this_corner);
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

void Whitespace::TidyVertices_()
{
  vector<shared_ptr<Vertex>> temporary_vertices;

  // For each vertex, if 1 or 3 corners have whitespace, push to result
  for (auto& corner : vertices_)
  {
    if ((corner->GetFlags() % 3) != 0) temporary_vertices.push_back(corner);
  }

  // Swap rather than copy vector into vertices_ member
  swap(temporary_vertices, vertices_);
}

//---------------------------------------------------------------------------//
// Now we have the vertices and the arrows pointing into and out of them,
// we can trace the polygons of which they form the perimeters. We take each
// vertex, find its out direction then follow where it points until we come to
// the next vertex on this line. By identifying the index of this vertex, we
// know to which vertex our test vertex points.

void Whitespace::TracePolygons_()
{
  for (auto& vertex : vertices_)
  {
    // Use the Direction enum as an int to get points beyond page edges
    float outer_edge = text_box_->Edge(vertex->Out()) +
                       (2 * (vertex->Out() / 2) - 1) * 100;

    // "edge" keeps track of the nearest vertex
    float edge = outer_edge;

    // Now for every other vertex
    for (auto& other_vertex : vertices_)
    {
      if (vertex->IsCloserThan(*other_vertex, edge))
      {
        // Vertex provisionally points to other_vertex
        vertex->PointAt(&other_vertex - &(vertices_[0]));

        // Now we update "edge" to make it the closest yet
        if (vertex->Out() == North || vertex->Out() == South)
        {
          edge = other_vertex->GetY();
        }
        else
        {
          edge = other_vertex->GetX();
        }
      }
    }

    // If the closest yet is not on the page, mark vertex for deletion
    if (edge - outer_edge < 0.1 && outer_edge - edge < 0.1)
    {
      vertex->SetFlags(0x80);
    }
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

void Whitespace::MakePolygonMap_()
{
  // we'll label our polygons with size_t's, but this is arbitrary
  size_t polygon_number = 1;

  // For each vertex...
  for (size_t i = 0; i < vertices_.size(); ++i)
  {
    // If this vertex is taken, move along.
    if (vertices_[i]->GetGroup()) continue;

    // we now know we're at the first unlabelled vertex
    size_t j = i;

    // While we're not back at the first vertex of our set
    while (vertices_[j]->GetGroup() == 0)
    {
      // Label the vertex with polygon number
      vertices_[j]->SetGroup(polygon_number);

      // If this is a new polygon number, start a new vector
      if (polygonmap_.find(polygon_number) == polygonmap_.end())
      {
        polygonmap_[polygon_number] = {vertices_[j]};
      }
      // Otherwise we append it to the set we are creating
      else polygonmap_[polygon_number].push_back(vertices_[j]);

      // Our loop now jumps to next clockwise vertex
      j = vertices_[j]->PointsTo();
    }

    // we've come back to start of polygon - start a new one
    polygon_number++;
  }
}

//---------------------------------------------------------------------------//
// Simple getter for the whitespace boxes - useful for debugging

std::vector<Box> Whitespace::WSBoxOut() const
{
  return boxes_;
}

//---------------------------------------------------------------------------//
// Now we have our polygons. Perhaps the simplest way of dealing with them is
// to find their bounding rectangles. This does that by finding minmax x and y.
// Clearly, we don't want the entire page to be a bounding box, as this will
// go on to engulf all the other boxes. Therefore if we find that a box matches
// the page box, we don't include this.

void Whitespace::PolygonMax_()
{
  // We're going to recycle boxes_ for our text boxes
  boxes_.clear();

  // For each polygon
  for (auto& shape : polygonmap_)
  {
    if(shape.second.empty()) continue;

    // Define floats that will expand outwards to fit our text box
    float min_left   = shape.second[0]->GetX(),
          max_right  = shape.second[0]->GetX(),
          min_bottom = shape.second[0]->GetY(),
          max_top    = shape.second[0]->GetY();

    // Shrink and invert the edges of our bounding box
    for (auto& corner : shape.second)
    {
      min_left   = min(min_left, corner->GetX());
      max_right  = max(max_right, corner->GetX());
      min_bottom = min(min_bottom, corner->GetY());
      max_top    = max(max_top, corner->GetY());
    }

    auto bounding_box = Box(min_left, max_right, max_top, min_bottom);
    // If the box is not the page itself, append this polygon to our result
    if (!bounding_box.IsApproximatelySameAs(*text_box_))
    {
      boxes_.emplace_back(move(bounding_box));
    }
  }

  CleanAndSortBoxes_();
}

//---------------------------------------------------------------------------//
// We have created a bunch of boxes containing text, but some boxes will
// completely contain other boxes. We want to remove the inner boxes to get our
// final set of content boxes.

void Whitespace::RemoveEngulfed_()
{
  // For each box check whether it engulfs another box
  for (auto outer = boxes_.begin(); outer != boxes_.end(); ++outer)
  {
    if (outer->IsConsumed()) continue;

    for (auto inner = outer; inner != boxes_.end(); ++inner)
    {
      if (inner->IsBeyond(*outer)) break;
      if (inner->IsConsumed()) continue;
      if (outer->Engulfs(*inner)) inner->Consume();
    }
  }

  CleanAndSortBoxes_();
}

//---------------------------------------------------------------------------//
// Finally we need to group our text items together in the text boxes for
// joining and analysis.

PageBox Whitespace::Output()
{
  vector<TextBox> result;
  for (auto& box : boxes_)
  {
    vector<TextPointer> text_vector;
    auto start_loop_at = text_box_->begin();
    for (auto text_it  = start_loop_at; text_it != text_box_->end(); ++text_it)
    {
      if (box.Engulfs(**text_it) && !(*text_it)->IsConsumed())
      {
        text_vector.push_back(*text_it);
        start_loop_at = text_it;
      }

      if ((*text_it)->IsBeyond(box)) break;
    }
    result.emplace_back(TextBox(move(text_vector), move(box)));
  }

  return PageBox((Box) *text_box_, result);
}
