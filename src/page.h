//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR page header file                                                    //
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

#ifndef PDFR_PAGE

//---------------------------------------------------------------------------//

#define PDFR_PAGE

/* This is the eighth in a sequence of daisy-chained headers that build up the
 * tools needed to read and parse the text content of pdfs. It comes after
 * font.h in the sequence and is the last step of constructing the logical
 * structure of pdf files.
 *
 * Each page object represents a page in the pdf document, taking as its
 * construction parameters just a document pointer and a page number.
 *
 * The page object acts as a container and organiser of the data required to
 * build a representation of the page. This includes the page dimensions,
 * the font objects used on the page, any xobjects, and the contents of the
 * page (as a page description program).
 *
 * The document and pagenumber are used to find the appropriate page header
 * dictionary. This gives the page dimensions, contents and resources (such
 * as fonts and xobjects). These items are pulled in from the relevant
 * pdf objects and processed to get the data members.
 *
 * The public interface is more substantial with the page class than with other
 * classes. The reason for this is that some of the data held by the page class
 * may be useful to the end user rather than just being abstractions accessed
 * by other classes. Some of the downstream classes will also needs members of
 * the interface however - the parser class needs to access the fonts,
 * page contents and Xobjects for example.
 */

#include "box.h"

//---------------------------------------------------------------------------//

class page
{
public:
  // constructor
  page(std::shared_ptr<document>, int);
  page(page&& p) noexcept {*this = std::move(p);}
  page& operator=(const page& p){ *this = p; return *this; }
  page& operator=(page&& p) noexcept { *this = std::move(p); return *this;}

  // public methods
  std::vector<std::string> get_font_names();  // Returns PostScript font names
  std::shared_ptr<std::string> get_page_contents(); // Returns page description program as string
  std::shared_ptr<std::string> get_XObject(const std::string&); // Return specified XObject string
  std::shared_ptr<font> get_font(const std::string&);  // Get pointer to font
  Box get_minbox();
  void clear_font_map();

private:
  // private data members
  std::shared_ptr<document> m_doc;        // Pointer to containing document
  int                 m_page_number;     // [Zero-indexed] page number
  Dictionary          m_header,         // The page's header dictionary
                      m_resources,      // Resource sub-dictionary
                      m_fonts;          // Font sub-dictionary
  Box  m_minbox;
  std::string         m_content_string;  // The page PostScript program
  double              m_rotate;         // Intended page rotation in degrees

  // A map of Xobject strings, which are fragments of page description programs
  std::unordered_map<std::string, std::string> m_XObjects;

  // The actual storage container for fonts, mapped to their pdf names
  static std::unordered_map<std::string, std::shared_ptr<font>> sm_fontmap;

  // private methods
  void read_XObjects(); // Write form XObjects to the xobject map
  void read_boxes();           // Store bounding boxes and calculate the smallest
  void read_header();       // Find the correct page header dictionary in document
  void read_resources();    // Obtain the resource dictionary
  void read_fonts();        // Get font dictionary and build fontmap
  void read_contents();     // find content objects to Write contentstring

  // Gets the leaf nodes of a content tree
  void expand_contents(std::vector<int> obs,
                      std::shared_ptr<tree_node<int>> tree);
};

//---------------------------------------------------------------------------//

#endif
