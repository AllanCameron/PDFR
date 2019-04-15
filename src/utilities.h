//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities header file                                               //
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

#ifndef PDFR_UTILTIES

//---------------------------------------------------------------------------//

#define PDFR_UTILTIES

/* This file is the first point in a daisy-chain of header files that
 * constitute the program. Since every other header file
 * in the program ultimately #includes utilities.h, it acts as a single point
 * from which to propagate global definitions and #includes.
 *
 * Since everything here is in the global workspace, it is best to be selective
 * about adding new functions - if possible, new functions should be added
 * to "downstream" classes.
 *
 * There are also a small number of templates defined here which are used to
 * reduce boilerplate code in the rest of the program.
 */

#include<string>
#include<vector>
#include<unordered_map>
#include<numeric>
#include<algorithm>
#include<iostream>
#include "debugtools.h"

/* The characters in pdf strings are most portably interpreted as uint16_t.
 * They need to be translated to Unicode for rendition to the intended
 * characters. Since Unicode is best handled as uint16_t too, it is easy
 * to get confused about pre-translation to Unicode and post-translation.
 * There are thus two typedefs for uint16_t to allow easy tracking of which
 * is which.
 */

typedef uint16_t RawChar;
typedef uint16_t Unicode;

//---------------------------------------------------------------------------//
// Template returning a vector of a std::map's keys. This can be used to
// enumerate the keys for iterating through the map or as a method of printing
// the map to the console

template< typename K, typename V > // K, V stand for key, value
std::vector<K> getKeys(const std::unordered_map<K, V>& Map)
{
  // vector to store results
  std::vector<K> result;

  // Ensure it is big enough
  result.reserve(Map.size());

  // Declare map iterator according to its type
  typename std::unordered_map<K, V>::const_iterator i;

  // The following loop iterates through the map, gets the key and stores it
  for(i = Map.begin(); i != Map.end(); ++i)
  {
    result.push_back(i->first);
  }

  return result;
}

//---------------------------------------------------------------------------//
// Simple template to shorten boilerplate of sticking vectors together
// Given two vectors of the same type, add B's contents to the end of A
// This modifies A

template <typename T>
void concat(std::vector<T>& A, const std::vector<T>& B)
{
  A.insert(A.end(), B.begin(), B.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); returns the indices which if applied would sort the
// vector from lowest to highest

template <typename T>
std::vector<int> order(const std::vector<T>& data)
{
  std::vector<int> index(data.size(), 0); // a new int vector to store results
  std::iota(std::begin(index), std::end(index), 0); // fill with ascending ints
  // Use a lambda function to sort 'index' based on the order of 'data'
  sort(index.begin(), index.end(), [&](const T& a, const T& b)
  {
    return (data[a] < data[b]);
  });
  return index;
}

//---------------------------------------------------------------------------//
// Sort one vector by another's order. Modifies supplied vector

template <typename Ta, typename Tb>
void sortby(std::vector<Ta>& vec, const std::vector<Tb>& data)
{
  if(vec.size() == 0) return; // Nothing to do!
  if(vec.size() != data.size()) // throw error if vector lengths don't match
    throw std::runtime_error("sortby requires equal-lengthed vectors");
  std::vector<Ta> res; // vector to store results
  // Use order(data) as defined above to sort vec
  for(auto i : order(data)) res.emplace_back(vec[i]);
  std::swap(res, vec); // replace vec by the stored results
}

//---------------------------------------------------------------------------//
// Class template for a tree structure, to expand the nodes of tree-like
// reference groups (e.g. pages in large documents or contents in complex ones)
// Each node contains a pointer to its parent (unless it is the root node
// in which case this variable is nullptr). It also contains a vector of
// pointers to child nodes. If the vector of child nodes is empty, then this
// is a leaf node. For simplicity, this is recorded using the boolean isLeaf.
// Finally, each node contains an arbitrary object of type T.
//
// The reason for using a tree structure is that we can use recursion to
// navigate an otherwise complex and arbitrary branching structure. In
// particular, we can build the tree quickly using recursion and find all
// the leaf nodes using recursion

template <class T>
class tree_node
{
public:
  // std::shared_ptr<tree_node<T>> looks messy, so we'll abbreviate it to Node
  using Node = std::shared_ptr<tree_node<T>>;

  // The standard constructor takes an object and a parent node
  tree_node<T>(T o, Node par):parent(par), isLeaf(true), obj(o) {};

  // However, to start a new tree, create a node without passing a parent node
  tree_node<T>(T o) : parent(Node(nullptr)),  isLeaf(true), obj(o){};

  // The blank constructor might be needed in copy construction
  tree_node<T>(){};

  //-------------------------------------------------------------------------//
  // This function adds child nodes to a given tree node

  void add_kids(std::vector<T> obs)
  {
    // If this node is going to have children, it will no longer be a leaf
    isLeaf = false;

    // For each object, make a new tree_node with it using 'this' as parent
    for(auto i : obs)
    {
      kids.push_back(std::make_shared<tree_node<T>>(i, Node(this)));
    }
  };

  //-------------------------------------------------------------------------//
  // Simple getter for the object contained in the node

  T get() const {return obj;}

  //-------------------------------------------------------------------------//
  // Follows parent pointers sequentially to get to the root node and returns
  // a pointer to it.

  Node top_node()
  {
    if(!parent) // If parent node is nullptr then !parent will resolve to true.
    {
      return Node(this);
    }

    // Get the parent node of the current node
    Node next_node_up = parent;

    // While next_node_up has a valid parent, make its parent next_node_up
    while(next_node_up->parent)
    {
      next_node_up = next_node_up->parent;
    }

    // Since current next_node_up has no parent (nullptr), it must be the root
    return next_node_up;
  }

  //-------------------------------------------------------------------------//
  // simple getter for child nodes

  std::vector<Node> getkids() const {return kids;};

  //-------------------------------------------------------------------------//
  // Returns a vector of all objects contained in ancestor nodes

  std::vector<T> getAncestors() const
  {
    // Start the vector with the object contained in this node
    std::vector<T> res {obj};

    // Otherwise, we get the parent node (or nullptr if this is the root)
    Node next_parent = parent;

    // Now for the parent node, we append its object to our result, and repeat
    // the process until we hit a nullptr, at which point we're done
    while(next_parent)
    {
      res.push_back(next_parent->objnum);
      next_parent = next_parent->parent;
    }

    return res;
  }

  //-------------------------------------------------------------------------//
  // Similar to getting the objects from all ancestors, we might want to get
  // objects from all descendant nodes. We can do this with recursion.

  std::vector<T> getDescendants() const
  {
    // Vector to store results
    std::vector<T> res;

    // For each child node, put its object in our vector
    for(auto& i : kids)
    {
      res.push_back(i->obj);
      // If the child node has its own child nodes, we just want to call the
      // function recursively and append each new result to our vector
      if(!i->kids.empty())
      {
        std::vector<T> tmp = i->getDescendants();
        res.reserve(tmp.size() + res.size());
        res.insert(res.end(), tmp.begin(), tmp.end());
      }
    }
    return(res);
  }

  //-------------------------------------------------------------------------//
  // An important use of the tree structure is to get all the objects contained
  // in its leaf nodes. Again, we use recursion to do this

  std::vector<T> getLeafs() const
  {
    // Vector to store results
    std::vector<T> res;

    // For each child node, if it is a leaf, append to our vector
    for(auto& i : kids)
    {
      if(i->isLeaf)
      {
        res.push_back(i->obj);
      }
      // If its not a leaf, call this function recursively until we get leaves.
      // Append the result of the recursively called function to our vector.
      else
      {
        std::vector<T> tmp = i->getLeafs();
        res.reserve(tmp.size() + res.size());
        res.insert(res.end(), tmp.begin(), tmp.end());
      }
    }
    return res;
  }
  //-------------------------------------------------------------------------//

private:
  Node parent;            // Shared pointer to parent tree_node<T>
  bool isLeaf;            // This will be false if the node has children
  std::vector<Node> kids; // Contains vector of shared pointers to child nodes
  T obj;                  // The object contained in this tree node itself
};


//---------------------------------------------------------------------------//
//                                                                           //
//                      global function declarations                         //
//                                                                           //
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Return the first substring of s that lies between two strings.
// This can be used e.g. to find the byte position that always sits between
// "startxref" and "%%EOF"

std::string carveout(const std::string&,
                     const std::string&,
                     const std::string&);

//---------------------------------------------------------------------------//
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string> multicarve(const std::string&,
                                    const std::string&,
                                    const std::string&);

//---------------------------------------------------------------------------//
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const std::string&);

//---------------------------------------------------------------------------//
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

std::vector<unsigned char> bytesFromArray(const std::string&);

//---------------------------------------------------------------------------//
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

std::string intToHexstring(int);

//---------------------------------------------------------------------------//
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char symbol_type(const char);

//---------------------------------------------------------------------------//
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

std::vector<RawChar> HexstringToRawChar(std::string&);

//---------------------------------------------------------------------------//
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

std::vector<RawChar> StringToRawChar(const std::string&);

//---------------------------------------------------------------------------//
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

std::vector<int> getObjRefs(const std::string&);

//---------------------------------------------------------------------------//
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> getints(const std::string&);

//---------------------------------------------------------------------------//
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// consume and convert integers but not hex, octal or scientific notation

std::vector<float> getnums(const std::string&);

//---------------------------------------------------------------------------//
// Loads a file's contents into a single std::string using <fstream>

std::string get_file(const std::string&);

//---------------------------------------------------------------------------//
// Converts a sequence of Unicode code points (given as a vector of 16 bit
// unsigned integers) as a utf-8 encoded string

std::string utf(const std::vector<uint16_t>& u);

#endif
