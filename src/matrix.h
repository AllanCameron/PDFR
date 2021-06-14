//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Matrix header file                                                  //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_MATRIX

//---------------------------------------------------------------------------//

#define PDFR_MATRIX

#include<vector>
#include<array>
#include<string>
#include<memory>

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

//---------------------------------------------------------------------------//

#endif
