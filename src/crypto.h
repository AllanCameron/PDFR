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
 * is accessed via a wrapper function in the xref class. The public interface
 * is a single function to decrypt a stream given the raw stream, the object
 * number and the generation number of the pdf object in which the stream
 * resides.
 */

#include "dictionary.h"
#include<deque>

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
  Crypto(const Dictionary& encryption_dictionary, const Dictionary& trailer);

  // This is the main decryption function which is also the public interface for
  // the class. It takes the raw stream, the object and generation numbers then
  // returns the decrypted stream.
  void DecryptStream(std::string& stream_to_be_decoded,
                      int object_number,
                      int object_generation_number) const;

private:
  // private data members
  const Dictionary& m_encryption_dictionary;
  const Dictionary& m_trailer;
  int               m_revision;
  std::vector<uint8_t> m_filekey;
  static const std::vector<uint8_t> sm_default_user_password;
  static const std::vector<FourBytes> sm_md5_table;
  static const std::vector<std::vector<FourBytes>> sm_mixarray;

  // Chops FourBytes into 4 bytes
  std::vector<uint8_t> ChopLong_(FourBytes unsigned_32_bit_int) const;

  // Return permission flags for file
  std::vector<uint8_t> ReadPermissions_(std::string permissions_string);

  // Helper function for md5
  void Md5Mix_(int iteration_of_md5,
               std::deque<FourBytes>& mix_deque,
               std::vector<FourBytes>& message_fingerprint) const;

  // Gives md5 hash of a vector of raw bytes
  std::vector<uint8_t> Md5_(std::vector<uint8_t> message_as_string) const;

  // Gives md5 hash of a string (as bytes)
  std::vector<uint8_t> Md5_(std::string message_as_bytes) const;

  // Gives rc4 cipher of message:key pair, given key and message
  void Rc4_(std::vector<uint8_t>& message, std::vector<uint8_t> key) const;

  // Gets /O and /U cipher
  std::vector<uint8_t> ReadPassword_(const std::string& o_or_u);

  // Constructs file key
  void ReadFileKey_();

  // Checks file key (revision 2)
  void CheckKeyR2_();

  // Checks file key (revision 3)
  void CheckKeyR3_();

};

//---------------------------------------------------------------------------//

#endif
