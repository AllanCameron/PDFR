//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR debugging tools header file                                         //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

// uncomment these lines for debugger and profiler
//#define PDFR_DEBUG
//#define PROFILER_PDFR
#include "external/Profiler.h"
#ifdef PDFR_DEBUG
#ifndef PDFR_DEBUGGER

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
  // print the contents of the vector followed by a comma for all but last
  if(s > 1) for(size_t i = 0; i < (s - 1); ++i) std::cout << x[i] << ", ";
  // the last entry doesn't need a comma after it
  if(s > 0) std::cout << x[s - 1] << std::endl;
}

#endif
#endif
