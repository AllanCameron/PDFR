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

bytes crypto::default_user_password =
{
  0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
  0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
  0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
  0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
};

//---------------------------------------------------------------------------//
// The md5 algorithm uses pseudorandom numbers to chop its message into bytes.
// Having them in a vector stops us from having to call the md5mix function
// with each seperate number 64 times. These numbers come from the function
// md5_table[i] = abs(sin(i + 1)) * 2^32, but it is quicker to pre-compute them

static vector<four_bytes> md5_table =
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

std::vector<std::vector<four_bytes>> mixarray =
{
  {7, 12, 17, 22},
  {5,  9, 14, 20},
  {4, 11, 16, 23},
  {6, 10, 15, 21},
};

//---------------------------------------------------------------------------//
// This simple function "chops" a four-byte int to a vector of four bytes.
// The bytes are returned lowest-order first as this is the typical use.

bytes crypto::chopLong(four_bytes longInt) const
{
  // The mask specifies that only the last byte is read when used with &
  four_bytes mask = 0x000000ff;

  // Create a length-4 vector of bytes filled with low-high bytes from longint
  bytes result(4, 0);
  for(int i = 0; i < 4; ++i)
  {
    result[i] = (longInt >> (8 * i)) & mask;
  }

  return result;
}

//---------------------------------------------------------------------------//
// The permission flags for which actions are available to the User are somewhat
// obfuscated in pdf. The flags are given as a string representing a 4-byte
// integer - this can be stoi'd easily enough to the intended integer. That
// integer then needs to be interpreted as a set of 32 bits, each of which acts
// as a permission flag. The permissions are required in order to construct the
// file key. To make this a compliant reader, we should also handle the flags
// appropriately. For the purposes of text extraction however, this is not
// required, and we just need the permissions flag to produce the file key.

bytes crypto::permissions(std::string str)
{
  // No string == no permissions. Can't decode pdf, so throw an error
  if(str.empty()) throw runtime_error("No permission flags");

  // Convert the string to a 4-byte int
  int flags = stoi(str);

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

void crypto::md5mix(int n, deque<four_bytes>& m, vector<four_bytes>& x) const
{
  // Declare and define some pseudorandom numbers
  four_bytes mixer, e, f = md5_table[n], g = mixarray[n / 16][n % 4];

  // Mangle bytes in various ways as per md5 algorithm
  switch(n / 16 + 1)
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
// The main md5 algorithm. This version of it was modified from various
// open source online implementations.

bytes crypto::md5(bytes msg) const
{
  // The length of the message
  int len = msg.size();

  // 16 * four_bytes will contain "fingerprint"
  std::vector<four_bytes> x(16, 0);

  // The number of 64-byte blocks to be processed plus extra 8-byte filler
  int nblocks = (len + 72) / 64;
                                 //
  // Starting pseudorandom numbers
  deque<four_bytes> mixvars = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

  // This next block fills the 16-element long "fingerprint" of the message (x)
  // with up to 64 bytes from each block of the message, mangles them and
  // turns them into the pseudorandom seed for the next block
  int i, j, k = 0;
  for (i = 0; i < nblocks; ++i)
  {
    // For each 4 bytes of block create a fourbyte from 4 bytes, low-order first
    for (j = 0; j < 16 && k < len - 3; ++j, k += 4)
    {
      x[j] = (((((msg[k+3] << 8) + msg[k+2]) << 8) + msg[k+1]) << 8) + msg[k];
    }

    // If this is the last block...
    if (i == nblocks - 1)
    {
      // create a fourbyte from the remaining bytes (low order first), with a
      // left-sided padding of (binary) 10000000...
      if (k == len - 3)
      {
        x[j++] = 0x80000000 + (((msg[k+2] << 8) + msg[k+1]) << 8) + msg[k];
      }
      else if (k == len - 2) x[j++] = 0x800000 + (msg[k + 1] << 8) + msg[k];
      else if (k == len - 1) x[j++] = 0x8000 + msg[k];
      else x[j++] = 0x80;

      // Right pad x with zeros to total 16 bytes
      while (j < 16) x[j++] = 0;

      // Store a mangled copy of length in the fingerprint
      x[14] = len << 3;
    }

    // Store a copy of initial random numbers
    deque<four_bytes> initvars = mixvars;

    // Shuffle x 64 times
    for(int n = 0; n < 64; n++) md5mix(n, mixvars, x);

    // Add initial random numbers
    for(int m = 0; m < 4;  m++) mixvars[m] += initvars[m];
  }

  bytes output; // create empty output vector for output

  // Split the resultant 4 x four_bytes into a single 16-byte vector
  for(auto m : mixvars) concat(output, chopLong(m));

  return output;
}

//----------------------------------------------------------------------------//
// This allows the md5 function to be run on a std::string without prior
// conversion. It simply converts a string to bytes than puts it
// into the "bytes" version of the function

bytes crypto::md5(std::string input) const
{
  bytes res(input.begin(), input.end());

  return md5(res);
}

//-------------------------------------------------------------------------//
// RC4 is a stream cipher used in encryption. It takes a string (or, as in this
// function, a vector of bytes) called a key, as well as the message to be
// scrambled. It uses the key as a seed from which to generate a seemingly
// random string of bytes. However, this stream of bytes can then be converted
// directly back into the original message using exactly the same key.
// The algorithm is now in the public domain

void crypto::rc4(bytes& message, bytes key) const
{
  int key_length = key.size(), message_length = message.size();
  uint8_t a = 0, b = 0, x = 0, y = 0;

  // Create state and fill with 0 - 0xff
  bytes state;
  for (int i = 0; i <= 0xff; ++i) state.push_back(i);

  // No key - can't modify message
  if (key_length == 0) return;

  // for each element in state, mix the state around according to the key
  for (auto& element : state)
  {
    b = (key[a] + element + b) % 256;
    swap(element, state[b]);
    a = (a + 1) % key_length;
  }

  // For each character in the message, mix as per rc4 algorithm
  for(int k = 0; k < message_length; k++)
  {
    uint8_t x1, y1;
    x1 = x = (x + 1) % 256;
    y1 = y = (state[x] + y) % 256;
    iter_swap(state.begin() + x1, state.begin() + y1);
    message[k] = message[k] ^ state[(state[x1] + state[y1]) % 256];
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

void crypto::decryptStream(string& stream, int object_num, int object_gen) const
{
  // Stream as bytes
  bytes stream_as_bytes(stream.begin(), stream.end());

  // Start building the object key with the file key
  bytes object_key = filekey;

  // Append the bytes of the object number
  concat(object_key, chopLong(object_num));

  // We only want the three lowest order bytes; pop the last
  object_key.pop_back();

  // Append lowest order byte of gen number
  object_key.push_back( object_gen & 0xff);

  // Then append the second lowest byte of gen number
  object_key.push_back((object_gen >> 8) & 0xff);

  // Store the object key's size
  uint8_t object_key_size = object_key.size();

  // Now md5 hash the object key
  object_key = md5(object_key);

   // Trim the result to match the object key's size
  object_key.resize(object_key_size);

  // Now we use this key to decrypt the stream using rc4
  rc4(stream_as_bytes, object_key);

  // finally we convert the resultant bytes back to a string
  stream = string(stream_as_bytes.begin(), stream_as_bytes.end());
}

/*---------------------------------------------------------------------------*/
// Gets the bytes comprising the hashed owner password from the encryption
// dictionary

bytes crypto::getPassword(const string& key)
{
   // Get raw bytes of owner password hash
  string password(encdict.get_string(key));
  string temp;
  temp.reserve(32);

  // This loop removes backslash escapes.
  for(auto i = password.begin() + 1; i != password.end(); ++i)
  {
    if(*i == '\\')
    {
      if(*(i + 1) == '\\') temp.push_back('\\');
    }
    else temp.push_back(*i);
    if(temp.size() == 32)
    {
      swap(temp, password);
      break;
    }
  }

  // The owner password should have 32 or more characters
  if(password.size() < 32) throw runtime_error("Corrupted password hash");

  // Return first 32 bytes (skip the first char which is the opening bracket)
  return bytes(password.begin(), password.end());
}

/*---------------------------------------------------------------------------*/
// The decryption key is needed to decrypt all streams except the xrefstream
// Its creation is described in ISO 3200 and implented here

void crypto::getFilekey()
{
  // Get the generic user password
  filekey = default_user_password;

  // Stick the owner password on
  concat(filekey, getPassword("/O"));

  // Stick permissions flags on
  concat(filekey, permissions(encdict.get_string("/P")));

  // Get first 16 bytes of file ID and stick them on too
  bytes idbytes = bytesFromArray(trailer.get_string("/ID"));
  idbytes.resize(16);
  concat(filekey, idbytes);

  // now md5 hash the result
  filekey = md5(filekey);

  // Set the default filekey size
  size_t cryptlen = 5;

  // if the filekey length is not 5, it will be specified as a number of bits
  // so divide by 8 to get the number of bytes.
  if(encdict.contains_ints("/Length"))
  {
    cryptlen = encdict.get_ints("/Length")[0] / 8;
  }

  // Resize the key as needed.
  filekey.resize(cryptlen);
}

/*---------------------------------------------------------------------------*/
// This algorithm checks that the file key is correct by ensuring that an rc4
// cipher of the default user password matches the user password hash in
// the encoding dictionary

void crypto::checkKeyR2()
{
  // Get the pdf's hashed user password
  bytes ubytes = getPassword("/U");

  // Get the default (unhashed) user password
  bytes checkans = default_user_password;

  // rc4 the default user password using the supplied filekey
  rc4(checkans, filekey);

  // This should be the same as the pdf's hashed user password
  if(checkans.size() == 32 && checkans == ubytes) return;

  // Otherwise, this key will not decrypt the pdf - throw an error
  throw runtime_error("Incorrect cryptkey");
}

/*---------------------------------------------------------------------------*/
// This is a more involved checking algorithm for higher levels of encryption.
// I couldn't get it to work for ages until I realised that the user and owner
// passwords sometimes contain backslash-escaped characters

void crypto::checkKeyR3()
{
  // We start with the default user password
  bytes user_password = default_user_password;

  // We now append the bytes from the ID entry of the trailer dictionary
  concat(user_password, bytesFromArray(trailer.get_string("/ID")));

  // We only want the first 16 bytes from the ID so truncate to 48 bytes (32+16)
  user_password.resize(48);

  // As per ISO 32000 we now md5 the result and rc4 using the filekey
  user_password = md5(user_password);
  rc4(user_password, filekey);

  // From ISO 32000: Take the result of the rc4 and do the following 19 times:
  for (uint8_t iteration = 1; iteration < 20; ++iteration)
  {
    // Create a new key by doing an XOR of each byte of the filekey with
    // the iteration number (1 to 19) of the loop.
    bytes temp_key;
    for (auto byte : filekey)
    {
      temp_key.push_back(byte ^ iteration);
    }

    // Create an rc4 hash of the ongoing hash which is fed to next iteration
    rc4(user_password, temp_key);
  }

  // Now get the pdf's user password to test against our calculated password
  bytes test_against = getPassword("/U");

  // Check the first 16 bytes of the two vectors to confirm the match
  for(int i = 0; i < 16; ++i)
  {
    // If any of the bytes do not match, we have the incorrect filekey
    if(test_against[i] != user_password[i])
    {
      throw runtime_error("cryptkey doesn't match");
    }
  }
}

/*---------------------------------------------------------------------------*/
// Constructor for the crypto class. Its two jobs are to obtain the
// file key and to check it's right. The crypto object is then kept alive to
// decode any encoded strings in the file

crypto::crypto(dictionary enc, dictionary trail) :
  encdict(enc), trailer(trail), revision(2)
{
  // Unless specified, the revision number used for encryption is 2
  if(encdict.contains_ints("/R")) revision = encdict.get_ints("/R")[0];

  getFilekey();

  if(revision == 2) checkKeyR2(); // if rnum 2, check it and we're done

  //  Otherwise we're going to md5 and trim the filekey 50 times
  else
  {
    size_t key_length = filekey.size();
    for(int i = 0; i < 50; ++i)
    {
      filekey = md5(filekey);
      filekey.resize(key_length);
    }

    // Check the filekey and we're done
    checkKeyR3();
  }
}
