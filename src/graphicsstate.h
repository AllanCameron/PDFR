//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GraphicsState header file                                           //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_GS

//---------------------------------------------------------------------------//

#define PDFR_GS

#include<utility>
#include<string>
#include<vector>
#include<memory>
#include "page.h"

//---------------------------------------------------------------------------//
// To define the position of elements on a page, the pdf page description
// program uses 3 * 3 matrices. These allow for arbitrary scaling, rotation,
// translation and skewing. Since the last column of a transformation
// matrix is always {0, 0,  1}, the matrices in pdf are defined by just six
// numbers in the page description program.
//
// For example, the entry "11 12 13 14 15 16 Tm" represents the following
// 3x3 transformation matrix:
//
//                      |   11    12    0  |
//                      |                  |
//                      |   13    14    0  |
//                      |                  |
//                      |   15    16    1  |
//
// The matrices all use floating point numbers and are all 3 x 3. Although we
// could just model them with a length 9 array of floats, it makes things a bit
// easier to just define a 3 x 3 float matrix here. That way, we can easily
// add or multiply two matrices using '+' and '-' instead of calling named
// functions. This is despite the fact that the underlying data member is
// a std::array<float, 9> anyway.

class Matrix
{
 public:
  // The default constructor returns a 3 x 3 identity matrix
  Matrix(): data_(std::array<float, 9> {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0}) {}

  // We can create a Matrix directly from a length-9 array of floats
  Matrix(std::array<float, 9> float_array): data_(float_array){}

  // This constructor takes a vector of 6 strings representing floats and
  // turns them into a 3 x 3 matrix as specified by the pdf page descriptor
  Matrix(const std::vector<std::string>& string_vector)
  {
    if (string_vector.size() < 6)
    {
      throw std::runtime_error("Can't create Matrix with fewer than 6 floats");
    }

    data_ = {stof(string_vector[0]), stof(string_vector[1]), 0,
             stof(string_vector[2]), stof(string_vector[3]), 0,
             stof(string_vector[4]), stof(string_vector[5]), 1};
  }

  // Assignment constructor
  Matrix& operator=(const Matrix& other)
  {
    this->data_ = other.data_;
    return *this;
  }

  // Operator overload of '*': returns dot product of two matrices
  Matrix operator*(const Matrix& other)
  {
    std::array<float, 9> new_data {};

    // Use indices to fill by loop
    for (size_t i = 0; i < 9; ++i)
    {
      new_data[i] = (data_[i % 3 + 0] * other.data_[3 * (i / 3) + 0] +
                     data_[i % 3 + 3] * other.data_[3 * (i / 3) + 1] +
                     data_[i % 3 + 6] * other.data_[3 * (i / 3) + 2] );
    }

    return Matrix(new_data);
  }

  // Transforms this matrix into the dot product of *this and t_other
  void operator*=(const Matrix& other)
  {
    std::array<float, 9> new_data {};

    // Use indices to fill by loop
    for (size_t i = 0; i < 9; ++i)
    {
      new_data[i] = (data_[i % 3 + 0] * other.data_[3 * (i / 3) + 0] +
                     data_[i % 3 + 3] * other.data_[3 * (i / 3) + 1] +
                     data_[i % 3 + 6] * other.data_[3 * (i / 3) + 2] );
    }
    // Swap rather than copy the array used as the data member
    std::swap(this->data_, new_data);
  }

  // Overloaded + operator returns the element-by-element addition of Matrices
  Matrix operator+(const Matrix& other)
  {
    std::array<float, 9> new_data {};
    for (size_t element = 0; element < 9; ++element)
    {
      new_data[element] = this->data_[element] + other.data_[element];
    }
    return Matrix(new_data);
  }

  // Transforms *this into *this + t_other using element-by-element addition
  void operator+=(const Matrix& other)
  {
    for (size_t element = 0; element < 9; ++element)
    {
      this->data_[element] += other.data_[element];
    }
  }

  // Gets a reference to an element of the data member
  float& operator[](size_t index)
  {
    return data_[index];
  }

  std::array<float, 2> transformXY(float x, float y)
  {
    std::array<float, 2> result = {data_[0] * x + data_[3] * y + data_[6],
                                   data_[1] * x + data_[4] * y + data_[7]};
    return result;
  }

 private:
  std::array<float, 9> data_;   // The actual data member
};


/*---------------------------------------------------------------------------*/
/* This is a header-only implementation of a Path class, which is used to
 * store information about shapes extracted from the page description program.
 *
 */

class Path
{
public:
  Path() : x_({0}), y_({0}), size_(1),
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

class TextState
{
public:
  float Tc,
        Tw,
        Th,
        Tl;
  std::string Tf;
  float Tfs;
  int Tmode;
  float Trise;

  TextState() : Tc(0), Tw(0), Th(100), Tl(0), Tf(""),
                Tfs(0), Tmode(0), Trise(0) {}
};

//---------------------------------------------------------------------------//

class GraphicsState
{
public:
  Matrix CTM;
  Path clipping_path;
  std::vector<std::string> colour_space;
  std::vector<float> colour,
                     fill;
  TextState text_state;
  float line_width;
  int line_cap;
  int line_join;
  float miter_limit;
  std::string rendering_intent;
  bool stroke_adjustment;
  std::vector<int> dash_array;
  std::vector<std::string> blending_mode;
  std::string soft_mask;
  float alpha_constant;
  bool  alpha_source;

  GraphicsState(std::shared_ptr<Page> p) :
                    CTM(Matrix()), clipping_path(Path()),
                    colour_space({"/DeviceGray"}),
                    colour({0, 0, 0}), fill({0, 0, 0}),
                    text_state(TextState()), line_width(1),
                    line_cap(0), line_join(0), miter_limit(10.0),
                    rendering_intent("/RelativeColorimetric"),
                    stroke_adjustment(false),
                    dash_array({0}),
                    blending_mode({"Normal"}), soft_mask("None"),
                    alpha_constant(1.0), alpha_source(false) {
    std::shared_ptr<Box> b = p->GetMinbox();
    clipping_path.SetX({b->GetLeft(), b->GetLeft(), b->GetRight(),
                        b->GetRight(), b->GetLeft()});
    clipping_path.SetY({b->GetBottom(), b->GetTop(), b->GetTop(),
                        b->GetBottom(), b->GetBottom()});
  }

};

//---------------------------------------------------------------------------//

#endif


