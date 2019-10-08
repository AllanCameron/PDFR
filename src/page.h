//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Page header file                                                    //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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
 * Each Page object represents a page in the pdf document, taking as its
 * construction parameters just a document pointer and a page number.
 *
 * The Page object acts as a container and organiser of the data required to
 * build a representation of the page. This includes the page dimensions,
 * the Font objects used on the page, any xobjects, and the contents of the
 * page (as a page description program).
 *
 * The document and pagenumber are used to find the appropriate page header
 * dictionary. This gives the page dimensions, contents and resources (such
 * as fonts and xobjects). These items are pulled in from the relevant
 * pdf objects and processed to get the data members.
 *
 * The public interface is more substantial with the Page class than with other
 * classes. The reason for this is that some of the data held by the Page class
 * may be useful to the end user rather than just being abstractions accessed
 * by other classes. Some of the downstream classes will also needs members of
 * the interface however - the parser class needs to access the fonts,
 * page contents and Xobjects for example.
 */

#include "font.h"

class Box;
template <class T> class TreeNode;

//---------------------------------------------------------------------------//

class Page
{
 public:
  // Constructor
  Page(std::shared_ptr<Document> document_ptr, int page_number);

  // Move constructor
  Page(Page&& t_other_page) noexcept {*this = std::move(t_other_page);}

  // lvalue assignment operator
  Page& operator=(const Page& t_other_page)
  {
    *this = t_other_page;
    return *this;
  }

  // rvalue assignment operator
  Page& operator=(Page&& t_other_page) noexcept
  {
    *this = std::move(t_other_page);
    return *this;
  }

  // Returns PostScript font names
  std::vector<std::string> GetFontNames();

  // Returns page description program
  std::shared_ptr<std::string> GetPageContents();

  // Returns a pointer to the contents of an XObject used by the page
  std::shared_ptr<std::string> GetXObject(const std::string& p_x_object_name);

  // Returns a pointer to the Font object from a given font name
  std::shared_ptr<Font> GetFont(const std::string& p_font_name);

  // Returns a Box object describing the page's bounding box.
  inline std::shared_ptr<Box> GetMinbox() const { return minbox_;}

  // Since the font map is a static object, it should be cleared at the end
  // of processing any particular document. Important!
  inline void ClearFontMap() { fontmap_.clear(); };

 private:
  // private data members
  std::shared_ptr<Document>   document_;        // Pointer to main document
  int                         page_number_;     // [Zero-indexed] page number
  std::shared_ptr<Dictionary> header_,          // The page's header dictionary
                              resources_,       // Resource sub-dictionary
                              fonts_;           // Font sub-dictionary
  std::shared_ptr<Box>        minbox_;          // Page bounding Box
  std::string                 content_string_;  // The page PostScript program
  double                      rotate_;          // Page rotation in degrees

  // A map of Xobject strings, which are fragments of page description programs
  std::unordered_map<std::string, std::string> xobjects_;

  // The actual storage container for fonts, mapped to their pdf names
  static std::unordered_map<std::string, std::shared_ptr<Font>> fontmap_;

  // private methods
  void ReadXObjects_(); // Write form XObjects to the xobject map
  void ReadBoxes_();        // Store bounding boxes and calculate the smallest
  void ReadHeader_();       // Find the correct header dictionary in document
  void ReadResources_();    // Obtain the resource dictionary
  void ReadFonts_();        // Get font dictionary and build fontmap
  void ReadContents_();     // find content objects to Write contentstring

  // Gets the leaf nodes of a content tree
  std::vector<int> ExpandContents_(std::vector<int>);
};

//---------------------------------------------------------------------------//

#endif
