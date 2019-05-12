//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR utilities header file                                               //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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

//---------------------------------------------------------------------------//
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
std::vector<K> GetKeys(const std::unordered_map<K, V>& t_map)
{
  // vector to store results
  std::vector<K> result;

  // Ensure it is big enough
  result.reserve(t_map.size());

  // Declare map iterator according to its type
  typename std::unordered_map<K, V>::const_iterator it;

  // The following loop iterates through the map, gets the key and stores it
  for(it = t_map.begin(); it != t_map.end(); ++it) result.push_back(it->first);

  return result;
}

//---------------------------------------------------------------------------//
// Simple template to shorten boilerplate of sticking vectors together
// Given two vectors of the same type, add B's contents to the end of A
// This modifies A

template <typename T>
void Concatenate(std::vector<T>& t_start, const std::vector<T>& t_append)
{
  t_start.insert(t_start.end(), t_append.begin(), t_append.end());
}

//---------------------------------------------------------------------------//
// Mimics R's order(); returns the indices which if applied would sort the
// vector from lowest to highest

template <typename T>
std::vector<int> Order(const std::vector<T>& t_data)
{
  std::vector<int> index(t_data.size(), 0); // a new int vector to store results
  std::iota(std::begin(index), std::end(index), 0); // fill with ascending ints

  // Use a lambda function to sort 'index' based on the order of 'data'
  sort(index.begin(), index.end(), [&](const int& low, const int& high)
  {
    return (t_data[low] < t_data[high]);
  });

  return index;
}

//---------------------------------------------------------------------------//
// Sort one vector by another's order. Modifies supplied vector

template <typename Ta, typename Tb>
void SortBy(std::vector<Ta>& t_sortee, const std::vector<Tb>& t_sorter)
{
  // Nothing to do!
  if(t_sortee.empty()) return;

  // Throw error if lengths don't match
  if(t_sortee.size() != t_sorter.size())
  {
    throw std::runtime_error("SortBy requires equal-lengthed vectors");
  }

  // Vector to store results
  std::vector<Ta> result;

  // Use Order(t_sorter) as defined above to sort sortee
  for(auto i : Order(t_sorter)) result.emplace_back(t_sortee[i]);

  std::swap(result, t_sortee);
}

//---------------------------------------------------------------------------//
// Class template for a tree structure, to expand the nodes of tree-like
// reference groups (e.g. pages in large documents or contents in complex ones)
// Each node contains a pointer to its parent (unless it is the root node
// in which case this variable is nullptr). It also contains a vector of
// pointers to child nodes. If the vector of child nodes is empty, then this
// is a leaf node. For simplicity, this is recorded using the boolean is_leaf_.
// Finally, each node contains an arbitrary object of type T.
//
// The reason for using a tree structure is that we can use recursion to
// navigate an otherwise complex and arbitrary branching structure. In
// particular, we can build the tree quickly using recursion and find all
// the leaf nodes using recursion

template <class T>
class TreeNode
{
 public:
  // std::shared_ptr<TreeNode<T>> looks messy, so we'll abbreviate it to Node
  using Node = std::shared_ptr<TreeNode<T>>;

  // The standard constructor takes an object and a parent node
  TreeNode<T>(T t_data, Node t_parent):
    parent_(t_parent), data_(t_data) {};

  // However, to start a new tree, create a node without passing a parent node
  TreeNode<T>(T t_data) :
    parent_(Node(nullptr)), data_(t_data) {};

  // The blank constructor might be needed in copy construction
  TreeNode<T>() {};

  //-------------------------------------------------------------------------//
  // This function adds child nodes to a given tree node

  void AddKids(std::vector<T> t_data)
  {
    // For each object, make a new TreeNode with it using 'this' as parent
    for(auto datum : t_data)
    {
      kids_.push_back(std::make_shared<TreeNode<T>>(datum, Node(this)));
    }
  };

  //-------------------------------------------------------------------------//
  // Simple getter for the object contained in the node

  inline T Get() const { return data_; }

  //-------------------------------------------------------------------------//
  // Follows parent pointers sequentially to get to the root node and returns
  // a pointer to it.

  Node TopNode()
  {
    // If parent node is nullptr then !parent_ will be true.
    if(!parent_) return Node(this);

    // Get the parent node of the current node
    Node next_node_up = parent_;

    // While next_node_up has a valid parent, make its parent next_node_up
    while (next_node_up->parent_) next_node_up = next_node_up->parent_;

    // Since current next_node_up has no parent (nullptr), it must be the root
    return next_node_up;
  }

  //-------------------------------------------------------------------------//
  // simple getter for child nodes

  inline std::vector<Node> GetKids() const { return kids_; }

  //-------------------------------------------------------------------------//
  // Simple checker for child nodes

  inline bool HasKids() const { return !kids_.empty();}

  //-------------------------------------------------------------------------//
  // Returns a vector of all objects contained in ancestor nodes

  std::vector<T> GetAncestors() const
  {
    // Start the vector with the object contained in this node
    std::vector<T> result {data_};

    // Otherwise, we get the parent node (or nullptr if this is the root)
    Node next_parent = parent_;

    // Now for the parent node, we append its object to our result, and repeat
    // the process until we hit a nullptr, at which point we're done
    while (next_parent)
    {
      result.push_back(next_parent->objnum);
      next_parent = next_parent->parent_;
    }

    return result;
  }

  //-------------------------------------------------------------------------//
  // Similar to getting the objects from all ancestors, we might want to get
  // objects from all descendant nodes. We can do this with recursion.

  std::vector<T> GetDescendants() const
  {
    // Vector to store results
    std::vector<T> result;

    // For each child node, put its object in our vector
    for(auto& kid : kids_)
    {
      result.push_back(kid->data_);

      // If the child node has its own child nodes, we just want to call the
      // function recursively and append each new result to our vector
      if(kid->HasKids())
      {
        std::vector<T> temporary = kid->GetDescendants();
        result.reserve(temporary.size() + result.size());
        result.insert(result.end(), temporary.begin(), temporary.end());
      }
    }
    return(result);
  }

  //-------------------------------------------------------------------------//
  // An important use of the tree structure is to get all the objects contained
  // in its leaf nodes. Again, we use recursion to do this

  std::vector<T> GetLeafs() const
  {
    // Vector to store results
    std::vector<T> result;

    // For each child node, if it is a leaf, append to our vector
    for(auto& kid : kids_)
    {
      if(!kid->HasKids())
      {
        result.push_back(kid->data_);
      }
      // If its not a leaf, call this function recursively until we get leaves.
      // Append the result of the recursively called function to our vector.
      else
      {
        std::vector<T> temporary = kid->GetLeafs();
        result.reserve(temporary.size() + result.size());
        result.insert(result.end(), temporary.begin(), temporary.end());
      }
    }
    return result;
  }
  //-------------------------------------------------------------------------//

 private:
  Node parent_;             // Shared pointer to parent TreeNode<T>
  std::vector<Node> kids_;  // Contains vector of shared pointers to child nodes
  T data_;                  // The object contained in this tree node itself
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

std::string CarveOut(const std::string& string_to_be_carved,
                     const std::string& left_delimiter,
                     const std::string& right_delimiter);

//---------------------------------------------------------------------------//
// finds all closest pairs of strings a, b and returns the substring between.
// This is used to carve out variable substrings between fixed substrings -
// a surprisingly common task in parsing text.

std::vector<std::string> MultiCarve(const std::string& string_to_be_carved,
                                    const std::string& left_delimiter,
                                    const std::string& right_delimiter);

//---------------------------------------------------------------------------//
// Decent approximation of whether a string contains binary data or not
// Uses <algorithm> from std

bool IsAscii(const std::string& string_to_be_tested);

//---------------------------------------------------------------------------//
//Takes a string of bytes represented in ASCII and converts to actual bytes
// eg "48656c6c6f20576f726c6421" -> "Hello World!"

std::vector<uint8_t> ConvertHexToBytes(const std::string& hex_encoded_string);

//---------------------------------------------------------------------------//
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
// eg 161 -> "00A1"

std::string ConvertIntToHex(int int_to_be_converted);

//---------------------------------------------------------------------------//
// Classify characters for use in lexers. This allows the use of switch
// statements that depend on whether a letter is a character, digit or
// whitespace but is indifferent to which specific instance of each it finds.
// For cases where the lexer needs to find a specific symbol, this function
// returns the original character if it is not a digit, a letter or whitespace

char GetSymbolType(const char input_char);

//---------------------------------------------------------------------------//
// Returns the data represented by an Ascii encoded hex string as a vector
// of two-byte numbers

std::vector<RawChar> ConvertHexToRawChar(std::string& hex_encoded_string);

//---------------------------------------------------------------------------//
// Converts normal string to a vector of 2-byte width numbers (RawChar)
// This requires sequential conversion from char to uint8_t to uint16_t
// (RawChar is just a synonym for uint16_t)

std::vector<RawChar> ConvertStringToRawChar(const std::string& input_string);

//---------------------------------------------------------------------------//
// This is a simple lexer to find any object references in the given string,
// in the form "xx x R". It is far quicker than finding matches with Regex,
// even though the code is more unwieldy. It is essentially a finite state
// machine that reads character by character and stores any matches found

std::vector<int> ParseReferences(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// Another lexer. This one finds any integers in a string.
// If there are decimal points, it ignores the fractional part.
// It will not accurately represent hex, octal or scientific notation (eg 10e5)

std::vector<int> ParseInts(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// This lexer retrieves floats from a string. It searches through the entire
// given string character by character and returns all instances where the
// result can be interpreted as a decimally represented number. It will also
// consume and convert integers but not hex, octal or scientific notation

std::vector<float> ParseFloats(const std::string& string_to_be_parsed);

//---------------------------------------------------------------------------//
// Loads a file's contents into a single std::string using <fstream>

std::string GetFile(const std::string& path_to_file);

#endif
