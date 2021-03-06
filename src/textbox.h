//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextBox header file                                                 //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_TEXT_BOX

//---------------------------------------------------------------------------//

#define PDFR_TEXT_BOX

#include "text_element.h"

//---------------------------------------------------------------------------//
// We need to be able to process groups of text_elements together; for this we
// could just use a vector of TextPointer. However, we often need to know the
// bounding box of a group of text_elements. We can therefore define a TextBox
// as a struct with a Box and a vector of text_elements.
//
// This header file contains the definitions of the TextElement, TextPointer and
// TextBox classes. Most of their methods are straightforward and inlined, but
// some of the more involved methods are described in text_element.cpp

//---------------------------------------------------------------------------//
// The TextBox will be the main data repository for our output. It inherits from
// Box and contains a vector of text_elements. To make it easy to work with, it
// contains functions that allow us to use it as if it was just a vector of
// text_elements. This allows for easy iteration.

class TextBox : public Box
{
  using TextPointer = std::shared_ptr<TextElement>;
  using TextBoxIterator = std::vector<TextPointer>::iterator;
  using TextBoxConstIterator = std::vector<TextPointer>::const_iterator;

 public:
  // Standard constructor - takes vector of TextElement pointers and the minbox
  TextBox(std::vector<TextPointer> text, Box box)
   : Box(box), data_(text) {}

  // Constructor from text and vector of floats representing a box
  TextBox(std::vector<TextPointer> text, std::vector<float> float_vector)
   : Box(float_vector), data_(text) {}

  // Constructor from individual elements
  TextBox(std::vector<TextPointer> text, float left, float right,
          float top,  float bottom)
   : Box(left, right, top, bottom), data_(text) {}

  // Assignment constructor
  TextBox(Box box):  Box(box) {}

  // Default constructor
  TextBox() = default;

  // Copy contructor
  TextBox(const TextBox& textbox) = default;

  // Lvalue assignment constructor
  TextBox& operator=(const TextBox& textbox) = default;

  // Rvalue assignment constructor
  TextBox& operator=(TextBox&& textbox) noexcept {
    std::swap(textbox, *this); return *this;}

  std::shared_ptr<TextElement> CastToElement()
  {
    if (data_.size() > 1)
    {
      throw std::runtime_error("Can't cast multiple TextBoxes to TextElement");
    }
    auto& element = data_[0];
    element->SetLeft(this->GetLeft());
    element->SetRight(this->GetRight());
    element->SetTop(this->GetTop());
    element->SetBottom(this->GetBottom());
    return element;
  }

  // Functions to copy the methods of vectors to access main data object
  inline TextBoxIterator begin() {return data_.begin(); }
  inline TextBoxIterator end()   {return data_.end(); }
  inline void erase(TextBoxIterator start, TextBoxIterator finish)
  {
    data_.erase(start, finish);
  }
  inline TextBoxConstIterator cbegin() const {return data_.cbegin(); }
  inline TextBoxConstIterator cend() const {return data_.cend(); }
  inline TextPointer& operator[](int index) { return data_[index]; }
  inline TextPointer front() const {return data_.front(); }
  inline TextPointer back() const { return data_.back(); }
  inline size_t size() const { return data_.size(); }
  inline bool empty() const { return data_.empty(); }
  inline void push_back(TextPointer text_ptr) { data_.push_back(text_ptr);}
  inline void clear() { data_.clear(); }
  inline void resize(int new_size) { data_.resize(new_size); }
  inline void SwapData(std::vector<TextPointer>& other)
  {
    std::swap(data_, other);
  }

  inline void emplace_back(TextPointer text_ptr)
  {
    data_.emplace_back(text_ptr);
  }

  void RemoveDuplicates();

  // Divides a TextBox into two
  TextBox SplitIntoTopAndBottom(float divide_at_this_y_value);
  TextBox SplitIntoLeftAndRight(float divide_at_this_x_value);

 private:
  // The data member
  std::vector<TextPointer> data_;
};

//---------------------------------------------------------------------------//
// This struct inherits from Box, and is created by feeding it a TextBox. It
// converts the vector of text_elements (which is conceptually a vector of
// data frame rows) into columns of the different data types.

class TextTable: public Box
{
 public:
  TextTable(const TextBox&);
  void Join(TextTable&);
  inline std::vector<float>&       GetLefts()      { return this->lefts_;  }
  inline std::vector<float>&       GetRights()     { return this->rights_; }
  inline std::vector<float>&       GetTops()       { return this->tops_;   }
  inline std::vector<float>&       GetBottoms()    { return this->bottoms_;}
  inline std::vector<float>&       GetSizes()      { return this->sizes_;  }
  inline std::vector<std::string>& GetFontNames()  { return this->fonts_;  }
  inline std::vector<std::string>& GetText()       { return this->text_;   }

 private:
  std::vector<std::string> text_, fonts_;
  std::vector<float> lefts_, rights_, bottoms_, tops_, sizes_;
};


//---------------------------------------------------------------------------//
// PageBox class. This is a class containing multiple textboxes as well as a
// 'naked' Box that gives the page dimensions

class PageBox : public Box
{
 public:
  PageBox(const Box& box, std::vector<TextBox> text_boxes)
    : Box(box), data_(text_boxes) {}

  inline TextBox& operator[](size_t i) { return data_[i];}
  inline std::vector<TextBox>::iterator begin() { return data_.begin();}
  inline std::vector<TextBox>::iterator end() { return data_.end();}
  inline bool empty() const { return data_.empty();}
  inline size_t size() const { return data_.size();}
  inline void push_back(TextBox textbox) { data_.push_back(textbox);}
  TextBox CastToTextBox()
  {
    auto result = TextBox((Box) *this);
    for (auto box : data_)
    {
      if(!box.empty()) result.push_back(box.CastToElement());
    }
    return result;
  }

private:
  std::vector<TextBox> data_;
};

#endif
