//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR crypto implementation file                                          //
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

#include "crypto.h"

using namespace std;

//---------------------------------------------------------------------------//
// The default user password cipher is required to construct the file key and is
// declared as a static member of the crypto class; it is defined here

bytes crypto::UPW = { 0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
                      0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
                      0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
                      0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A };

//---------------------------------------------------------------------------//
// This simple function "chops" a four-byte int to a vector of four bytes.
// The bytes are returned lowest-order first as this is the typical use.

bytes crypto::chopLong(fourbytes longInt) const
{
  bytes result; //container for result

   // The mask specifies that only the last byte is read when used with &
  fourbytes mask = 0x000000ff;
  result.push_back(longInt & mask);         // read last byte
  result.push_back((longInt >> 8) & mask);  // read penultimate byte
  result.push_back((longInt >> 16) & mask); // read second byte
  result.push_back((longInt >> 24) & mask); // read first byte

  return result;
}

//---------------------------------------------------------------------------//
// perm is short for permissions. The permission flags for which actions are
// available to the User are somewhat obfuscated in pdf. The flags are given
// as a string representing a 4-byte integer - this can be stoi'd easily enough
// to the intended integer. That integer then needs to be interpreted as a
// set of 32 bits, each of which acts as a permission flag. The permissions
// are required in order to construct the file key. To make this a compliant
// reader, we should also handle the flags appropriately. For the purposes of
// text extraction however, this is not required, and we just need the
// permissions flag to produce the file key.

bytes crypto::perm(std::string str) // takes a string with the permissions int
{
  // No string == no permissions. Can't decode pdf, so throw an error
  if(str.empty()) throw std::runtime_error("No permission flags");

  int flags = stoi(str); // Convert the string to a 4-byte int

  // This reads off the bytes from lowest order to highest order
  return chopLong(flags);
}

/*---------------------------------------------------------------------------*/
// The md5 algorithm produces a "hash" of 16 bytes from any given sequence
// of bytes. It is not practically possible to reverse the hash or find
// a random set of bytes that when passed through the function will match the
// hash. It is therefore like a "fingerprint" of any given plain data or string.
// You need to have the same data from which the hash was created in order to
// get the same hash. This allows passwords to be matched without the need for
// the actual password to be stored anywhere: a program or service can be
// protected just by insisting that a string is passed which matches the
// stored hash.
//
// The main work of the md5 function is done by shuffling byte positions around
// and performing bitwise operations on them. It seems fairly random and
// arbitrary, but is completely deterministic so that a given set of bytes
// always produced the same output.
//
// This function is called several times with different parameters as part
// of the main md5 algorithm. It can be considered a "shuffler" of bytes

void crypto::md5mix(int n, deque<fourbytes>& m, fourbytes e, fourbytes f,
                    fourbytes g) const
{
  fourbytes mixer; // temporary container for results

  // chops and mangles bytes in various ways as per md5 algorithm
  switch(n)
  {
    case 1  : mixer = (m[0] + ((m[1] & m[2]) | (~m[1] & m[3])) + e + f); break;
    case 2  : mixer = (m[0] + ((m[1] & m[3]) | (m[2] & ~m[3])) + e + f); break;
    case 3  : mixer = (m[0] + (m[1] ^ m[2] ^ m[3]) + e + f);          break;
    case 4  : mixer = (m[0] + (m[2] ^ (m[1] | ~m[3])) + e + f);       break;
    default : throw std::runtime_error("MD5 error"); // this function needs n!
  }
  mixer &= 0xffffffff; // applies four-byte mask

  m[0] = m[1] + (((mixer << g) | (mixer >> (32 - g))) & 0xffffffff);
  m.push_front(m.back());
  m.pop_back();
}

/*---------------------------------------------------------------------------*/
// The main md5 algorithm. This version of if was modified from various
// open source online implementations.

bytes crypto::md5(bytes input) const
{
  PROFC_NODE("md5");
  int len = input.size();
  std::vector<fourbytes> x {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 16 * fourbytes
  int nblocks = (len + 72) / 64;
  deque<fourbytes> mixvars = {1732584193, 4023233417, 2562383102, 271733878};

  int i, j, k = 0;
  for (i = 0; i < nblocks; ++i)
  {
    for (j = 0; j < 16 && k < len - 3; ++j, k += 4)
      x[j] = (((((input[k+3] << 8) + input[k+2]) << 8) + input[k+1]) << 8)
      + input[k];
    if (i == nblocks - 1)
    {
      if (k == len - 3)
        x.at(j) = 0x80000000 +
                  (((input.at(k + 2) << 8) +
                  input.at(k + 1)) << 8) +
                  input.at(k);
      else if (k == len - 2)
        x.at(j) = 0x800000 + (input.at(k + 1) << 8) + input.at(k);
      else if (k == len - 1)
        x.at(j) = 0x8000 + input.at(k);
      else
        x.at(j) = 0x80;
      j++;
      while (j < 16)
        x.at(j++) = 0;
      x.at(14) = len << 3;
    }
    deque<fourbytes> initvars = mixvars;
    vector<fourbytes> rnums = {
      3614090360,  3905402710,   606105819,  3250441966,  4118548399,
      1200080426,  2821735955,  4249261313,  1770035416,  2336552879,
      4294925233,  2304563134,  1804603682,  4254626195,  2792965006,
      1236535329};
    // now shuffle the deck using the md5mix function
    for(int n = 0; n < 16; n++)
      md5mix(1, mixvars, x[n], rnums[n], (5 * n) % 20 + 7);

    md5mix(2, mixvars, x.at(1),  4129170786,  5);
    md5mix(2, mixvars, x.at(6),  3225465664,  9);
    md5mix(2, mixvars, x.at(11), 643717713,  14);
    md5mix(2, mixvars, x.at(0),  3921069994, 20);
    md5mix(2, mixvars, x.at(5),  3593408605,  5);
    md5mix(2, mixvars, x.at(10), 38016083,    9);
    md5mix(2, mixvars, x.at(15), 3634488961, 14);
    md5mix(2, mixvars, x.at(4),  3889429448, 20);
    md5mix(2, mixvars, x.at(9),  568446438,   5);
    md5mix(2, mixvars, x.at(14), 3275163606,  9);
    md5mix(2, mixvars, x.at(3),  4107603335, 14);
    md5mix(2, mixvars, x.at(8),  1163531501, 20);
    md5mix(2, mixvars, x.at(13), 2850285829,  5);
    md5mix(2, mixvars, x.at(2),  4243563512,  9);
    md5mix(2, mixvars, x.at(7),  1735328473, 14);
    md5mix(2, mixvars, x.at(12), 2368359562, 20);
    md5mix(3, mixvars, x.at(5),  4294588738,  4);
    md5mix(3, mixvars, x.at(8),  2272392833, 11);
    md5mix(3, mixvars, x.at(11), 1839030562, 16);
    md5mix(3, mixvars, x.at(14), 4259657740, 23);
    md5mix(3, mixvars, x.at(1),  2763975236,  4);
    md5mix(3, mixvars, x.at(4),  1272893353, 11);
    md5mix(3, mixvars, x.at(7),  4139469664, 16);
    md5mix(3, mixvars, x.at(10), 3200236656, 23);
    md5mix(3, mixvars, x.at(13), 681279174,   4);
    md5mix(3, mixvars, x.at(0),  3936430074, 11);
    md5mix(3, mixvars, x.at(3),  3572445317, 16);
    md5mix(3, mixvars, x.at(6),  76029189,   23);
    md5mix(3, mixvars, x.at(9),  3654602809,  4);
    md5mix(3, mixvars, x.at(12), 3873151461, 11);
    md5mix(3, mixvars, x.at(15), 530742520,  16);
    md5mix(3, mixvars, x.at(2),  3299628645, 23);
    md5mix(4, mixvars, x.at(0),  4096336452,  6);
    md5mix(4, mixvars, x.at(7),  1126891415, 10);
    md5mix(4, mixvars, x.at(14), 2878612391, 15);
    md5mix(4, mixvars, x.at(5),  4237533241, 21);
    md5mix(4, mixvars, x.at(12), 1700485571,  6);
    md5mix(4, mixvars, x.at(3),  2399980690, 10);
    md5mix(4, mixvars, x.at(10), 4293915773, 15);
    md5mix(4, mixvars, x.at(1),  2240044497, 21);
    md5mix(4, mixvars, x.at(8),  1873313359,  6);
    md5mix(4, mixvars, x.at(15), 4264355552, 10);
    md5mix(4, mixvars, x.at(6),  2734768916, 15);
    md5mix(4, mixvars, x.at(13), 1309151649, 21);
    md5mix(4, mixvars, x.at(4),  4149444226,  6);
    md5mix(4, mixvars, x.at(11), 3174756917, 10);
    md5mix(4, mixvars, x.at(2),  718787259,  15);
    md5mix(4, mixvars, x.at(9),  3951481745, 21);

    for(int m = 0; m < 4; m++) mixvars[m] += initvars[m];
  }
  bytes output;                // create empty output vector for output
  for(auto m : mixvars) concat(output, chopLong(m));
  return output;
}

//----------------------------------------------------------------------------//
// This allows the md5 function to be run on a std::string without prior
// conversion. It simply converts a string to bytes than puts it
// into the "bytes" version of the function

bytes crypto::md5(std::string input) const
{
    bytes res(input.begin(), input.end()); // simple conversion to bytes
    return md5(res); // run md5 on bytes
}

//-------------------------------------------------------------------------//
// RC4 is a stream cipher used in encryption. It takes a string (or, as in this
// function, a vector of bytes) called a key, as well as the message to be
// scrambled. It uses the key as a seed from which to generate a seemingly
// random string of bytes. However, this stream of bytes can then be converted
// directly back into the original message using exactly the same key.
// The algorithm is now in the public domain

void crypto::rc4(bytes& msg, bytes key) const
{
  PROFC_NODE("rc4");
  int keyLen = key.size();
  int msgLen = msg.size();
  uint8_t a, b, t, x, y;
  int i;
  bytes state;
  for (i = 0; i <= 0xff; ++i) state.push_back(i); // fill state with 0 - 0xff
  if (keyLen == 0)
    return;
  a = b = x = y = 0;
  for (auto& i : state)
    {
      b = (key[a] + i + b) % 256;
      t = i;
      i = state[b];
      state[b] = t;
      a = (a + 1) % keyLen;
    }

  for(int k = 0; k < msgLen; k++)
    {
      uint8_t x1, y1;
      x1 = x = (x + 1) % 256;
      y1 = y = (state[x] + y) % 256;
      iter_swap(state.begin() + x1, state.begin() + y1);
      msg[k] = msg[k] ^ state[(state[x1] + state[y1]) % 256];
    }
}

/*---------------------------------------------------------------------------*/
// In order to decrypt an encrypted pdf stream, we need a few pieces of
// information. Firstly, the file key is required - this is constructed by
// the get_cryptkey() function. We also need to know the object number and
// generation number of the object where the stream is found.
//
// The decryption algorithm takes these pieces of information and adds their
// low order bytes to the end of the file key before running the result
// through an md5 hash. The first n bytes of the result, where n is the file
// key length plus 5, is then used as the key with which to decrypt the
// stream using the rc4 algorithm.

void crypto::decryptStream(std::string& streamstr, int objNum, int objGen)
const  {
  bytes streambytes(streamstr.begin(), streamstr.end()); // stream as bytes
  bytes objkey = filekey; // Start building the object key with the file key
  concat(objkey, chopLong(objNum)); // append the bytes of the object number
  objkey.pop_back(); // we only wanted the three lowest order bytes; pop last
  objkey.push_back( objGen & 0xff); // append lowest order byte of gen number
  objkey.push_back((objGen >> 8) & 0xff); // then the second lowest byte
  uint8_t objkeysize = objkey.size();
  objkey = md5(objkey); // md5 hash the resultant key
  while(objkey.size() > objkeysize) objkey.pop_back(); // then trim to fit

  // Now we use this key to decrypt the stream using rc4
  rc4(streambytes, objkey);

  // finally we convert the resultant bytes to a string
  streamstr = std::string(streambytes.begin(), streambytes.end());
}

/*---------------------------------------------------------------------------*/
// Gets the bytes comprising the hashed owner password from the encryption
// dictionary

bytes crypto::getPassword(const string& key)
{
  string ostarts = encdict.get(key);  // get raw bytes of owner password hash

  // The owner password has should have >= 32 characters
  if(ostarts.size() < 33) throw runtime_error("Corrupted password hash");

  // Return first 32 bytes (skip the first char which is the opening bracket)
  bytes obytes(ostarts.begin() + 1, ostarts.begin() + 33);
  return obytes;
}

/*---------------------------------------------------------------------------*/
// The decryption key is needed to decrypt all streams except the xrefstream
// Its creation is described in ISO 3200 and implented here

void crypto::getFilekey()
{
  bytes Fstring = UPW; // The generic or null user password
  concat(Fstring, getPassword("/O")); // stick the owner password on
  concat(Fstring, perm(encdict.get("/P"))); // Stick permissions flags on

  // get first 16 bytes of file ID and stick them on too
  bytes idbytes = bytesFromArray(trailer.get("/ID"));
  idbytes.resize(16);
  concat(Fstring, idbytes);

  // now md5 hash the result
  filekey = md5(Fstring);

  size_t cryptlen = 5; // the default filekey size

  // if the filekey length is not 5, it will be specified as a number of bits
  // so divide by 8 to get the number of bytes and resize the key as needed.
  if(encdict.hasInts("/Length"))
    cryptlen = encdict.getInts("/Length").at(0) / 8;
  filekey.resize(cryptlen);
}

/*---------------------------------------------------------------------------*/
// This algorithm checks that the file key is correct by ensuring that an rc4
// cipher of the default user password matches the user password hash in
// the encoding dictionary

void crypto::checkKeyR2()
{
  bytes ubytes = getPassword("/U"); // user password cipher
  bytes checkans = UPW;
  rc4(checkans, filekey); // rc4 of default user password
  if(checkans.size() == 32 && checkans == ubytes) return;
  throw runtime_error("Incorrect cryptkey");
}

/*---------------------------------------------------------------------------*/
// This is a more involved checking algorithm which I can't get to work on
// a test file, even though I know the key is right (it deciphers the streams)
// I have checked the ISO 32000 spec, Poppler and pdf.js but it just doesn't
// seem to work. Maybe one for Stackoverflow...

void crypto::checkKeyR3()
{/*
  bytes ubytes = getPassword("/U");
  bytes buf = UPW;
  concat(buf, bytesFromArray(trailer.get("/ID")));
  buf.resize(48);
  bytes checkans = rc4(md5(buf), filekey);
  for (int i = 19; i >= 0; i--)
  {
    bytes tmpkey;
    for (auto j : filekey)
      tmpkey.push_back(j ^ ((uint8_t) i));
    checkans = rc4(checkans, tmpkey);
  }
  int m = 0;
  for(int l = 0; l < 16; l++)
  {
    if(checkans[l] != ubytes[l]) break;
    m++;
  }
  if(m != 16) std::cout << "cryptkey doesn't match";
*/}

/*---------------------------------------------------------------------------*/
// Creator function for the crypto class. Its two jobs are to obtain the
// file key and to check it's right.

crypto::crypto(dictionary enc, dictionary trail) :
  encdict(enc), trailer(trail), revision(2)
{
  // Unless specified, the revision number used for encryption is 2
  if(encdict.hasInts("/R")) revision = encdict.getInts("/R")[0];

  //  gets the filekey
  getFilekey();

  if(revision == 2) checkKeyR2(); // if rnum 2, check it and we're done
  else
  {//  Otherwise we're going to md5 and trim the filekey 50 times
    size_t cryptlen = filekey.size();
    for(int i = 0; i < 50; i++)
    {
      filekey = md5(filekey);
      filekey.resize(cryptlen);
    }
    checkKeyR3(); // check the filekey and we're done
  }
}
