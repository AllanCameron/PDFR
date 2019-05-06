//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR crypto header file                                                  //
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

#ifndef PDFR_CRYPTO

//---------------------------------------------------------------------------//

#define PDFR_CRYPTO

/* This header file includes the declaration of a class which containes the
 * algorithms needed to decrypt protected pdfs.
 *
 * This only applies to situations in which a password is not required to
 * open the file. It allows reading of pdfs in which the ability to copy and
 * paste, save or modify the file have been disabled by the owner but it can
 * still be opened and read by anyone without a user password.
 *
 * Most pdfs will open without the need for decryption, but some (such as the
 * ISO 32000 pdf reference document itself) are useless without the ability to
 * decrypt.
 *
 * Decryption is quite well encapsulated here. The implementation of decryption
 * is left to private member functions. The decryption itself is called only
 * when an object stream is extracted at the point of pdf object creation and
 * is accessed via a wrapper function in the xref class.
 */

#include "dictionary.h" // The class needs utilities and has to be able to
                        // negotiate pdf dictionaries.
#include<deque> // needed for md5mix function

//---------------------------------------------------------------------------//
// A std::vector<uint8_t> is more succinctly described by the type name "Bytes"

typedef std::vector<unsigned char> ByteVector;

//---------------------------------------------------------------------------//
// The md5 algorithm makes use of 4-byte numbers (unsigned long or uint32_t).
// To shorten the name and make it explicit what we are talking about I have
// typedef'd uint32_t as FourBytes

typedef uint32_t FourBytes;

//---------------------------------------------------------------------------//
// Class definition for crypto

class Crypto
{
public:
  // Constructors
  Crypto(){};
  Crypto(Dictionary, Dictionary);

  // This is the main decryption function which is also the public interface for
  // the class. It takes the raw stream, the object and generation numbers then
  // returns the decrypted stream.
  void decrypt_stream(std::string&, int obj, int gen) const;

private:
  // private data members
  Dictionary encryption_dictionary_;
  Dictionary trailer_;
  int        revision_;
  ByteVector filekey_;

  static  ByteVector default_user_password_;

  // Chops FourBytes into 4 bytes
  ByteVector chop_long(FourBytes) const;

  // Return permission flags for file
  ByteVector read_permissions(std::string);

  // Helper function for md5
  void md5_mix(int, std::deque<FourBytes>&, std::vector<FourBytes>&) const;

  // Gives md5 hash of a vector of raw bytes
  ByteVector md5(ByteVector input) const;

  // Gives md5 hash of a string (as bytes)
  ByteVector md5(std::string input) const;

  // Gives rc4 cipher of message:key pair, given key and message
  void rc4(ByteVector&, ByteVector) const;

  // Gets /O and /U cipher
  ByteVector get_password(const std::string&);

  // Constructs file key
  void get_file_key();

  // Checks file key (revision 2)
  void check_key_r2();

  // Checks file key (revision 3)
  void check_key_r3();
};

//---------------------------------------------------------------------------//

#endif
