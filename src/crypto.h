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

//---------------------------------------------------------------------------//
// A std::vector<uint8_t> is more succinctly described by the type name "bytes"

typedef std::vector<uint8_t> bytes;

//---------------------------------------------------------------------------//
// The md5 algorithm makes use of 4-byte numbers (unsigned long or uint32_t).
// To shorten the name and make it explicit what we are talking about I have
// typedef'd uint32_t as fourbytes

typedef uint32_t fourbytes;

//---------------------------------------------------------------------------//
// Class definition for crypto

class crypto
{
private:

// private data members
  dictionary encdict;
  dictionary trailer;
  int revision;
  bytes filekey;
  static bytes UPW;

// private member functions
  bytes chopLong(fourbytes);    // Chops fourbytes into vector of 4 bytes lo-hi
  bytes perm(std::string);      // Return permission flags for file
  fourbytes md5mix( int,        //---------//
                    fourbytes,             //
                    fourbytes,             //
                    fourbytes,             //--> "byte mangler" for md5
                    fourbytes,             //
                    fourbytes,             //
                    fourbytes,             //
                    fourbytes); //---------//
  bytes md5(bytes input);       // Gives md5 hash of a vector of raw bytes
  bytes md5(std::string input); // Gives md5 hash of a std::string (as bytes)
  bytes rc4(bytes, bytes);      // Gives rc4 cipher of message:key pair, or the
                                // original message, given a key and the cipher
  bytes getPassword(const std::string&);   // gets /O and /U cipher
  void getFilekey();                       // constructs file key
  void checkKeyR2();                       // checks file key
  void checkKeyR3();                       // checks file key

public:

// Creator functions

  crypto(){};
  crypto(dictionary, dictionary);

// This is the main decryption function which is also the public interface for
// the class. It takes the raw stream, the object and generation numbers then
// returns the decrypted stream.

  std::string decryptStream(std::string, int obj, int gen);
};


//---------------------------------------------------------------------------//

#endif
