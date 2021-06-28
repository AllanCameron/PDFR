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
#include "text_element.h"


/*---------------------------------------------------------------------------*/
/* This is a header-only implementation of a GraphicObject class, which is used
 * to store information about shapes extracted from the page description
 * program.
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
  virtual void NewSubpath() {}
  virtual void SetX(std::vector<float> values) {}
  virtual void SetY(std::vector<float> values) {}
  virtual void CloseSubpath() {}
  virtual void AppendX(std::vector<float> value) {}
  virtual void AppendY(std::vector<float> value) {}
  virtual std::vector<float> GetX() {return {0};}
  virtual std::vector<float> GetY() {return {0};}
  virtual bool IsClosed() { return false;}
  virtual float Bottom()  { return 0;}
  virtual float Top()     { return 0;}
  virtual float Left()    { return 0;}
  virtual float Right()   { return 0;}
  virtual float Width()   { return 0;}
  virtual float Height()  { return 0;}
  virtual std::string GetText() {return "";}
  virtual float GetFontSize() {return 0;}
  virtual std::vector<int> GetSubpaths() {return {0};}

  // Getters

  virtual float GetLineWidth() {return this->linewidth_;}
  virtual std::vector<float> GetColour() {return this->stroke_colour_;}
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
  Path(): path_x_({}), path_y_({}), current_subpath_(0), is_closed_({false}) {}

  void SetX(std::vector<float> values) {this->path_x_ = values;}
  void SetY(std::vector<float> values) {this->path_y_ = values;}

  void NewSubpath() {++current_subpath_;}
  void CloseSubpath() {
    is_closed_.back() = true;
    int pos = std::find(subpaths_.begin(), subpaths_.end(), current_subpath_) -
              subpaths_.begin();
    path_x_.push_back(path_x_[pos]);
    path_y_.push_back(path_y_[pos]);
    subpaths_.push_back(subpaths_.back());
  }

  void AppendX(std::vector<float> value) {
    Concatenate(this->path_x_, {value});
    while(subpaths_.size() < path_x_.size()){
      subpaths_.push_back(current_subpath_);
    }
  }

  void AppendY(std::vector<float> value) {
    Concatenate(this->path_y_, {value});
    while(subpaths_.size() < path_x_.size()){
      subpaths_.push_back(current_subpath_);
    }
  }

  std::vector<float> GetX() {return this->path_x_;}
  std::vector<float> GetY() {return this->path_y_;}
  bool IsClosed() { return this->is_closed_.back();}

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
  std::vector<int> GetSubpaths() {return subpaths_;}

 private:
  std::vector<float> path_x_;
  std::vector<float> path_y_;
  int current_subpath_;
  std::vector<int> subpaths_;
  std::vector<bool> is_closed_;
  };

/*---------------------------------------------------------------------------*/

class Text : public GraphicObject {

 public:
  Text(std::shared_ptr<TextElement> text) : contents_(text) {}
  std::string GetText() {return contents_->Utf();}
  std::vector<float> GetColour() {return this->GetFillColour();}
  std::vector<float> GetX() {return {contents_->GetLeft()};}
  std::vector<float> GetY() {return {contents_->GetBottom()};}
  float GetFontSize() {return contents_->GetSize();}


 private:
  std::shared_ptr<TextElement> contents_;
};

#endif
