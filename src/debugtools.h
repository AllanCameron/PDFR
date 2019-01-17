//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR debugging tools header file                                         //
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

#ifndef PDFR_DEBUG

//---------------------------------------------------------------------------//

#define PDFR_DEBUG

/* Debugtools is a small header-only collection of templates and inline
 * functions used to help debug and profile PDFR. The project can be compiled
 * without it, and any file can use the functions by #including debugtools.h
 * in their header. It is not meant for inclusion in a built production version
 * of the package
 */

// Includes come from standard library only
#include<string>
#include<vector>
#include<chrono> // for profiling
#include<iostream> // for outputting to console

/*---------------------------------------------------------------------------*/
// This simple template allows the contents of a vector to be printed to
// the console, provided that type T has a << method defined.

template<typename T>
void printvec(std::vector<T> x)
{
  size_t s = x.size();
  // In the special case of an empty vector, make this explicit
  if(s == 0) std::cout << "[empty vector]" << std::endl;
  if(s > 1) for(size_t i = 0; i < (s - 1); i++) std::cout << x[i] << ", ";
  if(s > 0) std::cout << x[s - 1] << std::endl;
}

/*---------------------------------------------------------------------------*/

inline std::chrono::high_resolution_clock::time_point startClock()
{
  return std::chrono::high_resolution_clock::now();
}

/*---------------------------------------------------------------------------*/

inline void timeSince(std::string message,
               std::chrono::high_resolution_clock::time_point& start)
{
  std::chrono::high_resolution_clock::time_point end =
    std::chrono::high_resolution_clock::now();
  auto diff = end - start;
  std::cout << message << ": "
            << std::chrono::duration <double, std::milli> (diff).count()
            << " ms" << std::endl;
  start = std::chrono::high_resolution_clock::now();
}

/*---------------------------------------------------------------------------*/

#endif
