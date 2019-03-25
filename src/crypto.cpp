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

bytes crypto::UPW =
{
  0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
  0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
  0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
  0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
};

//---------------------------------------------------------------------------//
// The md5 algorithm uses pseudorandom numbers to chop its message into bytes.
// Having them in a vector stops us from having to call the md5mix function
// with each seperate number 64 times.

static vector<fourbytes> rnums =
{
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

//---------------------------------------------------------------------------//
// More pseudorandom numbers for the md5 hash

std::vector<std::vector<fourbytes>> mixarray =
{
  {7, 12, 17, 22},
  {5,  9, 14, 20},
  {4, 11, 16, 23},
  {6, 10, 15, 21},
};
//---------------------------------------------------------------------------//
// This simple function "chops" a four-byte int to a vector of four bytes.
// The bytes are returned lowest-order first as this is the typical use.

bytes crypto::chopLong(fourbytes longInt) const
{
  // The mask specifies that only the last byte is read when used with &
  fourbytes mask = 0x000000ff;
  bytes result =
  {
    (uint8_t) (longInt & mask),         // read last byte
    (uint8_t) ((longInt >> 8) & mask),  // read penultimate byte
    (uint8_t) ((longInt >> 16) & mask), // read second byte
    (uint8_t) ((longInt >> 24) & mask)  // read first byte
  };
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

void crypto::md5mix(int n, deque<fourbytes>& m, vector<fourbytes>& x) const
{
  fourbytes mixer, e; // temporary container for results
  fourbytes f = rnums[n]; // select the starting pseudorandom number
  fourbytes g = mixarray[n / 16][n % 4]; // select another pseudorandom number
  switch(n / 16 + 1) // mangles bytes in various ways as per md5 algorithm
  {
    case 1  : e = x[(1 * n + 0) % 16];
              mixer = (m[0] + ((m[1] & m[2]) | (~m[1] & m[3])) + e + f); break;
    case 2  : e = x[(5 * n + 1) % 16];
              mixer = (m[0] + ((m[1] & m[3]) | (m[2] & ~m[3])) + e + f); break;
    case 3  : e = x[(3 * n + 5) % 16];
              mixer = (m[0] + (m[1] ^ m[2] ^ m[3]) + e + f);             break;
    case 4  : e = x[(7 * n + 0) % 16];
              mixer = (m[0] + (m[2] ^ (m[1] | ~m[3])) + e + f);          break;
    default: throw runtime_error("md5 error: n > 63");
  }
  // further bit shuffling:
  m[0] = m[1] + (((mixer << g) | (mixer >> (32 - g))) & 0xffffffff);
  // now push all elements to the left (with aliasing)
  m.push_front(m.back());
  m.pop_back();
}

/*---------------------------------------------------------------------------*/
// The main md5 algorithm. This version of if was modified from various
// open source online implementations.

bytes crypto::md5(bytes msg) const
{
  int len = msg.size(); // The length of the message
  std::vector<fourbytes> x(16, 0); // 16 * fourbytes will contain "fingerprint"
  int nblocks = (len + 72) / 64; // The number of 64-byte blocks to be processed
                                 // allowing for an extra 8-byte filler
  // Starting pseudorandom numbers
  deque<fourbytes> mixvars = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

  // This next block fills the 16-element long "fingerprint" of the message (x)
  // with up to 64 bytes from each block of the message, mangles them and
  // turns them into the pseudorandom seed for the next block
  int i, j, k = 0;
  for (i = 0; i < nblocks; ++i) // for each block...
  {
    for (j = 0; j < 16 && k < len - 3; ++j, k += 4) // for each 4 bytes of block
      // create a single low-order first fourbyte of the four seperate bytes
      x[j] = (((((msg[k+3] << 8) + msg[k+2]) << 8) + msg[k+1]) << 8) + msg[k];
    if (i == nblocks - 1) // if this is the last block...
    {
      // create a fourbyte from the remaining bytes (low order first), with a
      // left-sided padding of (binary) 10000000...
      if (k == len - 3)
        x[j++] = 0x80000000 + (((msg[k+2] << 8) + msg[k+1]) << 8) + msg[k];
      else if (k == len - 2) x[j++] = 0x800000 + (msg[k + 1] << 8) + msg[k];
      else if (k == len - 1) x[j++] = 0x8000 + msg[k];
      else x[j++] = 0x80;

      while (j < 16) x[j++] = 0; // right pad x with zeros to total 16 bytes
      x[14] = len << 3; // store a mangled copy of length in the fingerprint
    }
    deque<fourbytes> initvars = mixvars; // store a copy of initial random nums
    for(int n = 0; n < 64; n++) md5mix(n, mixvars, x); // shuffle x 64 times
    for(int m = 0; m < 4;  m++) mixvars[m] += initvars[m]; // add initial nums
  }
  bytes output; // create empty output vector for output
  // split the resultant 4 x fourbytes into a single 16-byte vector
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
  int keyLen = key.size();
  int msgLen = msg.size();
  uint8_t a, b, x, y;
  int i;
  bytes state;
  for (i = 0; i <= 0xff; ++i) state.push_back(i); // fill state with 0 - 0xff
  if (keyLen == 0) return; // no key - can't modify message
  a = b = x = y = 0;
  for (auto& i : state) // for each element in state...
  {
    // mix the state around according to the key
    b = (key[a] + i + b) % 256;
    swap(i, state[b]);
    a = (a + 1) % keyLen;
  }

  for(int k = 0; k < msgLen; k++) // for each character in the message
  {
    // mix the message around according to the state
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
// file key and to check it's right. The crypto object is then kept alive to
// decode any encoded strings in the file

crypto::crypto(dictionary enc, dictionary trail) :
  encdict(enc), trailer(trail), revision(2)
{
  // Unless specified, the revision number used for encryption is 2
  if(encdict.hasInts("/R"))
    revision = encdict.getInts("/R")[0];
  getFilekey();
  if(revision == 2)
    checkKeyR2(); // if rnum 2, check it and we're done
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
