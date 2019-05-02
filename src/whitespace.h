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
// The whitespace class takes a word grouper as an argument in its constructor
// and from that uses a sequence of helper functions to construct its final
// output, which is a vector of WS_box containing the text boxes for a page.

class Whitespace
{
public:
  // constructor
  Whitespace(textbox);

  //  Output the text element groups directly
  std::vector<textbox> output();

  // Output the final text box co-ordinates
  std::vector<Box> ws_box_out() const;

private:
  //The main output is a collection of pairs of text boxes with their elements
  textbox m_page_text; // a copy of word grouper's output
  std::unordered_map<size_t, std::vector<std::shared_ptr<Vertex>>> polygonMap;
  float max_line_space;                // The average font size on the page
  std::vector<Box> m_boxes;            // used in construction AND output
  std::vector<std::shared_ptr<Vertex>> m_vertices; // Used to make polygons
  static const size_t DIVISIONS = 400; // number of strips used for whitespace

  void getMaxLineSize();
  void pageDimensions();    // Gets page margins
  void cleanAndSortBoxes(); // Helper to remove Boxes flagged for deletion
  void makeStrips();        // Cover the whitespace with tall thin strips
  void mergeStrips();       // merge adjacent strips into boxes
  void removeSmall();       // remove insufficiently tall boxes
  void makeVertices();      // use Boxes to find vertices of polygons
  void tidyVertices();      // identify and remove the unneeded vertices
  void tracePolygons();     // trace around polygons by following vertices
  void makePolygonMap();    // map polygons to size_t keys
  void polygonMax();        // find bounding boxes of polygons
  void removeEngulfed();    // remove boxes within other boxes
};


//---------------------------------------------------------------------------//

#endif
