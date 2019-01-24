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
 * the interface however - the GraphicsState class needs to access the fonts,
 * page contents and Xobjects for example.
 */

#include "font.h"

//---------------------------------------------------------------------------//

class page
{

public:

  // constructor function
  page(document* a_pointer_to_the_document, int this_is_the_page_number);

  // public methods
  std::vector<std::string> getFontNames();  // Returns PostScript font names
  std::string pageContents(); // Returns page description program as string
  std::vector<float> getminbox(); // Get co-ordinates of smallest bounding box
  std::string getXobject(const std::string&); // Return specified XObject string
  font* getFont(const std::string&);  // Returns a pointer to specified string

private:

  // private data members
  document*           d;              // Pointer to containing document
  int                 pagenumber;     // [Zero-indexed] page number
  dictionary          header,         // The page's header dictionary
                      resources,      // Resource sub-dictionary
                      fonts;          // Font sub-dictionary
  std::vector<float>  bleedbox,       //----//
                      cropbox,              //
                      mediabox,             //--> Various page bounding boxes
                      trimbox,              //
                      artbox,         //----//
                      minbox;         // The smallest bounding box around text
  std::string         contentstring;  // The page description program as string
  double              rotate;         // Intended page rotation in degrees

  // A map of Xobject strings, which are fragments of page description programs
  std::unordered_map<std::string, std::string> XObjects;

  // The actual storage container for fonts, mapped to their pdf names
  std::unordered_map<std::string, font> fontmap;

  // private methods
  void parseXObjStream(); // Write form XObjects to the xobject map
  void boxes();           // Store bounding boxes and calculate the smallest
  void getHeader();       // Find the correct page header dictionary in document
  void getResources();    // Obtain the resource dictionary
  void getFonts();        // Get font dictionary and build fontmap
  void getContents();     // find content objects to Write contentstring

  // Gets the leaf nodes of a content tree
  std::vector<int> expandContents(std::vector<int> objnums);

};

//---------------------------------------------------------------------------//

#endif
