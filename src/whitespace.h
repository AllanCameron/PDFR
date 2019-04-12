//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR whitespace header file                                              //
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

#ifndef PDFR_WSPACE

//---------------------------------------------------------------------------//

#define PDFR_WSPACE

/* This class's job is to take the output of the parser and to carry out the
 * first stage of page segmentation. It does this by dividing the page into
 * a large number of tall vertical strips. Any strips that encounter an
 * obstruction (i.e. one or more glyphs) as they go from the top to the bottom
 * of the page are divided so they do not overlap the glyphs. Thus, if there
 * are n rows of text that the strip would otherwise cross, the strip is
 * divided into n + segments.
 *
 * Once the strips are all calculated, they will cover all the significant empty
 * spaces (henceforth whitespace) in a document, leaving islands of text content
 * uncovered. These islands are physically, and usually logically related.
 *
 * However, there is some work to be done to identify the islands in question.
 * Firstly, we need to ensure that contiguous whitespace is joined together as
 * far as possible. This is done by looking to the right of each strip segment.
 * If the strip immediately to the right has the same top and bottom value, then
 * it is joined to the strip to the left by reducing its left value to the
 * same as the test strip, then flagging the test strip for deletion.
 *
 * The procedure may also leave small holes in the text islands due to
 * whitespace between words and lines. We remove these based on size criteria.
 *
 * Once we have are final set of whitespace boxes, we look at each vertex in
 * each whitespace box to determine which quadrants contain whitespace. None
 * should contain zero or four quadrants, and they should all lie on either a
 * page margin or the margin of a text island.
 *
 * The point of doing this is that we can use this information to draw a line
 * clockwise around the edge of each island by identifying the configuration
 * of whitespace around each vertex. Once we have drawn the polygons defining
 * each island, we can then assign glyphs to be inside one of these polygons.
 * This gives us a group of page segments along with the glyphs they contain.
 * We can then use this information to group letters and words together,
 * establish a reading order and attempt classification of text elements based
 * on size, shape, position and order on the page.
 */

#include "word_grouper.h"

//---------------------------------------------------------------------------//
// This enum allows Vertices to be labelled according to which clockwise
// direction around a polygon their incoming and outgoing edges point. The
// strange numbering is simply to allow a matching to the indices of the crop
// box passed from page.h, so that these labels can be used in place of indices

enum Direction {North = 3, South = 1, East = 2, West = 0, None = 4};

//---------------------------------------------------------------------------//
// A lot of the work of Whitespace.cpp is done by identifying and manipulating
// rectangles of whitespace. These are simply a struct of 4 co-ordinates and
// an additional deletion flag that allows a vector of WSbox to be iterated and
// allowing deletions without removing iterator validity.

struct WSbox
{
  float left, right, top, bottom;
  bool deletionFlag;

  inline bool operator==(const WSbox& other) const
  {
    return left == other.left && right  == other.right &&
           top  == other.top  && bottom == other.bottom;
  }

  inline void remove() {deletionFlag = true;}

  inline bool is_NW_of(float x, float y) const
  {
    return right >= x && left < x && top > y && bottom <= y;
  }

  inline bool is_NE_of(float x, float y) const
  {
    return right > x && left <= x && top > y && bottom <= y;
  }

  inline bool is_SE_of(float x, float y) const
  {
    return right > x && left <= x && top >= y && bottom < y;
  }

  inline bool is_SW_of(float x, float y) const
  {
    return right >= x && left < x && top >= y && bottom < y;
  }
};

//---------------------------------------------------------------------------//
// Each vertex starts life at the corner of a whitespace box, then most are
// pruned until only those forming the vertices of text boxes remain. Along the
// way a Vertex has to know in which direction whitespace lies, which directions
// the edges of the text box enter and leave it, which other Vertex this one
// points to, and which group of vertices it belongs to. It also needs a
// deletion flad - hence the relatively large number of member variables. Some
// of these have been compressed into a flag byte to save space.

struct Vertex
{
  float x, y;
  uint8_t flags; // delete-void-void-void-NW-NE-SE-SW
  Direction In, Out;
  size_t points_to, group;

};

//---------------------------------------------------------------------------//
// The whitespace class takes a word grouper as an argument in its constructor
// and from that uses a sequence of helper functions to construct its final
// output, which is a vector of WS_box containing the text boxes for a page.

class Whitespace
{
public:
  // constructor
  Whitespace(textrows);
  //  Output the text element groups directly
  std::vector<std::pair<WSbox, std::vector<text_ptr>>> output() const;
  // Output the final text box co-ordinates
  std::vector<WSbox> ws_box_out() const;

private:
  //The main output is a collection of pairs of text boxes with their elements
  std::vector<std::pair<WSbox, std::vector<text_ptr>>> groups;
  std::vector<text_ptr> WGO; // a copy of wgo's output
  std::vector<float> minbox;
  std::unordered_map<size_t, std::vector<Vertex>> polygonMap;// main polygon map
  WSbox m_page;
  float midfontsize;  // The average font size on the page
  std::vector<WSbox> ws_boxes; // used in whitespace construction AND output
  std::vector<Vertex> vertices; // The vertices used to make polygons
  static const size_t DIVISIONS = 200; // number of strips used for whitespace
  // we use this to map directions to vertices
  static std::unordered_map<uint8_t, std::pair<Direction, Direction>> arrows;

  void pageDimensions();    // Gets page margins
  void clearDeletedBoxes(); // Helper to remove WSboxes flagged for deletion
  void makeStrips();        // Cover the whitespace with tall thin strips
  void mergeStrips();       // merge adjacent strips into boxes
  void removeSmall();       // remove insufficiently tall boxes
  void makeVertices();      // use WSboxes to find vertices of polygons
  void tidyVertices();      // identify and remove the unneeded vertices
  void tracePolygons();     // trace around polygons by following vertices
  void makePolygonMap();    // map polygons to size_t keys
  void polygonMax();        // find bounding boxes of polygons
  void removeEngulfed();    // remove boxes within other boxes
  void groupText();         // Adds each text element to correct box

  bool eq(float a, float b);// compare floats for close equality
};


//---------------------------------------------------------------------------//

#endif
