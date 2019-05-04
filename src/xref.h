//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR xref header file                                                    //
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

#ifndef PDFR_XREF

//---------------------------------------------------------------------------//

#define PDFR_XREF

/* This is the third main header in the daisy-chain of #includes that builds up
 * the tools needed to read and parse pdf, after utilities.h and dictionary.h.
 * It also includes a couple of other headers which are needed to decrypt and
 * decode encrypted and compressed streams (streams.h and crypto.h)
 *
 * The cross reference table (xref) is a data structure (or more accurately a
 * group of data structures) that forms part of the pdf file format and allows
 * for the rapid random access of the pdf objects from which a document is
 * comprised. At its simplest, this is a table containing the object number,
 * the generation number of the object, and the number of bytes from the start
 * of the file where that object is located.
 *
 * However, it is not always quite that simple. Firstly, documents can and do
 * have more than one xref that lists different objects. Secondly, the xref
 * can itself be a compressed stream which must be found and translated before
 * being read. This means the xref class must have access to decryption and
 * decoding algorithms.
 *
 * Fortunately, the location of the start of an xref table (as number of bytes
 * offset from the start of the file) is given right at the end of a file, just
 * before the %%EOF on the last line. It is thus simple to get to the start of
 * an xref from this number. For a normal uncompressed xref, this takes us to
 * the top of a table which is just read and parsed. At the end of the table is
 * a special dictionary which does not belong to any object. This is the
 * trailer dictionary. If there are other xrefs in the file, this tells us
 * where the next one is, and we can continue to hop around and read the xrefs
 * until none are left and we have a complete "roadmap" of where the objects
 * are in the file.
 *
 * If, however, the xref is located in a stream, things get more complicated.
 * The stream belongs to an object, and the dictionary at the beginning of that
 * object doubles as the trailer dictionary. As well as being compressed, the
 * stream containing the xref is usually encoded as a string of bytes which
 * then need to be interpreted using the algorithm normally used for
 * decompressing PNG files. This makes handling xref streams complex enough to
 * warrant their own class. However, since this class only has to perform a part
 * of xref implementation, it has no public interface and is therefore not
 * defined in this header file, but rather within xref.cpp
*/
#include<utility>
#include "streams.h"
#include "crypto.h"

/*---------------------------------------------------------------------------*/
// The main xref data member is an unordered map with the key being the object
// number and the value being a struct of named ints as defined here

struct xrefrow
{
  int startbyte,  // Its byte offset
      stopbyte,   // The offset of the corresponding endobj marker
      in_object;  // If this is a stream object, in which other object is it
};                // located? Has value of 0 if the object is not in a stream

/*---------------------------------------------------------------------------*/
// The main xref class definition. Since this is the main "skeleton" of the pdf
// which is used by other classes to negotiate and parse the pdf, and because it
// can be complex to construct, it is a fairly large and complex class.
//
// Where possible I have tried to delegate some of its work to other classes
// or subclasses, but even still it is a little unwieldy.

class xref
{
public:
  // constructors
  xref(){};
  xref(std::shared_ptr<const std::string>);

  // public methods
  bool is_encrypted() const;           // Eeturns encryption state
  Dictionary get_trailer() const;
  size_t get_object_start_byte(int) const;   // Byyte offset of a given object
  size_t get_object_end_byte(int) const;  // Byte offset of end of given object
  size_t get_holding_number_of(int) const;
  std::vector<int> get_all_object_numbers() const;
  std::array<size_t, 2> get_stream_location(int) const;
  void decrypt(std::string&, int, int) const; // Decrypt a stream
  std::shared_ptr<const std::string> file() const;// pointer to main file string

private:
  std::shared_ptr<const std::string> m_file_string;
  std::unordered_map<int, xrefrow> m_xref_table; // This is the main data member
  std::vector<int> m_xref_locations;         // vector of offsets of xref starts
  Dictionary m_trailer_dictionary;           // Canonical trailer dictionary
  bool m_encrypted;                     // Flag to indicate if encryption used
  std::shared_ptr<Crypto> m_encryption; // crypto object for decrypting files

// private methods
  xref& operator=(const xref&);
  int get_stream_length(const Dictionary& dict) const;
  void locate_xrefs();                     // Finds xref locations
  void read_xref_strings();               // Gets strings from xref locations
  void read_xref_from_stream(int);        // Uses xrefstream class to get xref
  void read_xref_from_string(std::string&);   // parses xref directly
  void create_crypto();                   // Allows decryption of encrypted docs
};

//---------------------------------------------------------------------------//

#endif
