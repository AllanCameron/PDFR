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
/* This is a header-only implementation of a GraphicObject class, which is used to
 * store information about shapes extracted from the page description program.
 *
 */

class GraphicObject
{
public:
  GraphicObject() : linewidth_(1),
  stroke_colour_({0, 0, 0}), is_stroked_(false),
  is_filled_(false), fill_colour_({0.5, 0.5, 0.5}) {};

  // Setters
  void SetLineWidth(float size) {this->linewidth_ = size;}
  void SetColour(std::vector<float> colour) {this->stroke_colour_ = colour;}
  void SetFillColour(std::vector<float> colour) {this->fill_colour_ = colour;}
  void SetStroke(bool visible) {this->is_stroked_ = visible;}
  void SetFilled(bool is_filled) {this->is_filled_ = is_filled;}

  // virtual functions allow type-specific behaviour in derived classes

  virtual void SetX(std::vector<float> values) {}
  virtual void SetY(std::vector<float> values) {}
  virtual void SetClosed(bool is_closed) {}
  virtual void AppendX(float value) {}
  virtual void AppendY(float value) {}
  virtual std::vector<float> GetX() {return {0};}
  virtual std::vector<float> GetY() {return {0};}
  virtual bool IsClosed() { return false;}
  virtual float Bottom()  { return 0;}
  virtual float Top()     { return 0;}
  virtual float Left()    { return 0;}
  virtual float Right()   { return 0;}
  virtual float Width()   { return 0;}
  virtual float Height()  { return 0;}

  // Getters

  float GetSize() {return this->linewidth_;}
  std::vector<float> GetColour() {return this->stroke_colour_;}
  bool IsStroked() {return this->is_stroked_;}
  bool IsFilled() {return this->is_filled_;}
  std::vector<float> GetFillColour() {return this->fill_colour_;}

private:
  float linewidth_;
  std::vector<float> stroke_colour_;
  bool is_stroked_;
  bool is_filled_;
  std::vector<float> fill_colour_;

};

/*---------------------------------------------------------------------------*/

class Path : public GraphicObject {
public:
  Path(): path_x_({0}), path_y_({0}), is_closed_(false) {}

  void SetX(std::vector<float> values) {this->path_x_ = values;}
  void SetY(std::vector<float> values) {this->path_y_ = values;}
  void SetClosed(bool is_closed) {this->is_closed_ = is_closed;}
  void AppendX(float value) { Concatenate(this->path_x_, {value});}
  void AppendY(float value) { Concatenate(this->path_y_, {value});}
  std::vector<float> GetX() {return this->path_x_;}
  std::vector<float> GetY() {return this->path_y_;}
  bool IsClosed() { return this->is_closed_;}
  float Bottom()  { return *std::min_element(this->path_y_.begin(),
                                             this->path_y_.end());}
  float Top()     { return *std::max_element(this->path_y_.begin(),
                                             this->path_y_.end());}
  float Left()    { return *std::min_element(this->path_x_.begin(),
                                             this->path_x_.end());}
  float Right()   { return *std::max_element(this->path_x_.begin(),
                                             this->path_x_.end());}
  float Width()   { return this->Right() - this->Left();}
  float Height()  { return this->Top() - this->Bottom();}

private:
  std::vector<float> path_x_;
  std::vector<float> path_y_;
  bool is_closed_;
  };

/*---------------------------------------------------------------------------*/

#endif
