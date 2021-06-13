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
#include "graphicobject.h"

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

class TextState
{
  public:
    float                 tc,     // Character spacing
                          tw,     // Word spacing
                          th,     // Horizontal scaling
                          tl,     // Text leading
                          tfs,    // Font size
                          trise;  // Text rise
    std::string           tf;     // Font name
    int                   tmode;  // Text printing mode
    std::shared_ptr<Font> current_font;

    TextState() : tc(0), tw(0), th(100), tl(0),
                  tfs(0), trise(0), tf(""), tmode(0) {}
};

//---------------------------------------------------------------------------//

class GraphicsState
{
  public:
    Matrix                   CTM;
    GraphicObject            clipping_path;
    std::vector<std::string> colour_space_stroke,
                             colour_space_fill;
    std::vector<float>       colour,
                             fill;
    TextState                text_state;
    Matrix                   tm_state,
                             td_state;
    float                    line_width;
    int                      line_cap,
                             line_join;
    float                    miter_limit;
    std::string              rendering_intent;
    bool                     stroke_adjustment;
    std::vector<int>         dash_array;
    std::vector<std::string> blending_mode;
    std::string              soft_mask;
    float                    alpha_constant;
    bool                     alpha_source;

    GraphicsState(std::shared_ptr<Page> p) :
                      CTM(Matrix()), clipping_path(GraphicObject()),
                      colour_space_stroke({"/DeviceGray"}),
                      colour_space_fill({"/DeviceGray"}),
                      colour({0, 0, 0}), fill({0, 0, 0}),
                      text_state(TextState()), tm_state(Matrix()),
                      td_state(Matrix()), line_width(1),
                      line_cap(0), line_join(0), miter_limit(10.0),
                      rendering_intent("/RelativeColorimetric"),
                      stroke_adjustment(false),
                      dash_array({0}),
                      blending_mode({"Normal"}), soft_mask("None"),
                      alpha_constant(1.0), alpha_source(false)
    {
      std::shared_ptr<Box> b = p->GetMinbox();
      clipping_path.SetX({b->GetLeft(),   b->GetLeft(), b->GetRight(),
                          b->GetRight(),  b->GetLeft()});
      clipping_path.SetY({b->GetBottom(), b->GetTop(), b->GetTop(),
                          b->GetBottom(), b->GetBottom()});
    }

};

//---------------------------------------------------------------------------//

#endif


