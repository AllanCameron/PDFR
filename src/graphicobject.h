//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GraphicObject header file                                           //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_GO

//---------------------------------------------------------------------------//

#define PDFR_GO

#include<utility>
#include<string>
#include<vector>
#include<memory>


/*---------------------------------------------------------------------------*/
/* This is a header-only implementation of a Path class, which is used to
 * store information about shapes extracted from the page description program.
 *
 */

class GraphicObject
{
public:
  GraphicObject() : x_({0}), y_({0}), size_(1),
  colour_({0, 0, 0}), is_closed_(false), is_visible_(false),
  is_filled_(false), fill_colour_({0.5, 0.5, 0.5}) {};

  void SetX(std::vector<float> values) {this->x_ = values;}
  void SetY(std::vector<float> values) {this->y_ = values;}
  void AppendX(float value) { Concatenate(this->x_, {value});}
  void AppendY(float value) { Concatenate(this->y_, {value});}
  void SetSize(float size) {this->size_ = size;}
  void SetColour(std::vector<float> colour) {this->colour_ = colour;}
  void SetFillColour(std::vector<float> colour) {this->fill_colour_ = colour;}
  void SetVisibility(bool visible) {this->is_visible_ = visible;}
  void SetClosed(bool is_closed) {this->is_closed_ = is_closed;}
  void SetFilled(bool is_filled) {this->is_filled_ = is_filled;}

  std::vector<float> GetX() {return this->x_;}
  std::vector<float> GetY() {return this->y_;}
  float GetSize() {return this->size_;}
  std::vector<float> GetColour() {return this->colour_;}
  bool IsClosed() {return this->is_closed_;}
  bool IsVisible() {return this->is_visible_;}
  bool IsFilled() {return this->is_filled_;}
  std::vector<float> GetFillColour() {return this->fill_colour_;}

  float Bottom() {return *std::min_element(this->y_.begin(), this->y_.end());}
  float Top()    {return *std::max_element(this->y_.begin(), this->y_.end());}
  float Left()   {return *std::min_element(this->x_.begin(), this->x_.end());}
  float Right()  {return *std::max_element(this->x_.begin(), this->x_.end());}

  float Width()  {return this->Right() - this->Left();}
  float Height() {return this->Top() - this->Bottom();}

private:
  std::vector<float> x_;
  std::vector<float> y_;
  float size_;
  std::vector<float> colour_;
  bool is_closed_;
  bool is_visible_;
  bool is_filled_;
  std::vector<float> fill_colour_;

};

/*---------------------------------------------------------------------------*/

#endif
