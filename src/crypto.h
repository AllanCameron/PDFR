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

/* This header file includes the necessary algorithms to decrypt protected pdfs.
 * This only applies to situations in which a password is not required to
 * open the file. It allows reading of pdfs in which the ability to copy and
 * paste, save or modify the file have been disabled by the owner but it can
 * still be opened and read by anyone without a user password.
 *
 * Most pdfs will open without the need for decryption, but some (such as the
 * ISO 32000 pdf reference document itself) are useless without the ability to
 * decrypt.
 *
 * Decryption is quite well encapsulated here even though it is not a class.
 * The only externally called functions are perm() and decryptStream()
 * which are available in the global workspace. Although md5 and rc4 are also
 * made available globally, this is purely so that they can be used externally
 * from this library as they are useful functions in their own right. The
 * implementation of the decryption is left to internal functions in crypto.cpp
 * and are not available globally.
 */

#include "utilities.h"

// A std::vector<uint8_t> is more succinctly described by the type name "bytes"

typedef std::vector<uint8_t> bytes;

// The default user password is required to construct the filekey and needs to
// be defined ahead of used.

static bytes UPW = { 0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
                     0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
                     0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
                     0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A };

// Return permission flags for file
bytes perm(std::string str);

// Gives md5 hash of a vector of raw bytes
bytes md5(bytes input);

// Gives md5 hash of a std::string (as bytes)
bytes md5(std::string input);

// Gives rc4 hash of a message:key pair, or the message given a key and hash
bytes rc4(bytes msg, bytes key);

// This is the main decryption function. It takes the raw stream, the file key
// (which is calculated in the xref class), the object and generation numbers
// then returns the decrypted stream.
std::string decryptStream(std::string strm, bytes key, int objNum, int objGen);

//---------------------------------------------------------------------------//

#endif
