//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR crypto implementation file                                          //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "crypto.h"

using namespace std;

/*---------------------------------------------------------------------------*/
// Constructor for the crypto class. Its two jobs are to obtain the
// file key and to check it's right. The crypto object is then kept alive to
// decode any encoded strings in the file

Crypto::Crypto(Dictionary t_encrypt_dict, Dictionary t_trailer)
  : encryption_dictionary_(t_encrypt_dict),
    trailer_(t_trailer),
    revision_(2)
{
  // Unless specified, the revision number used for encryption is 2
  if (encryption_dictionary_.ContainsInts("/R"))
  {
    revision_ = encryption_dictionary_.GetInts("/R")[0];
  }

  ReadFileKey_();

  // if revision 2, check it and we're done. Otherwise use revision 3
  if (revision_ == 2) CheckKeyR2_();
  else CheckKeyR3_();
}

//---------------------------------------------------------------------------//
// The default user password cipher is required to construct the file key and is
// declared as a static member of the crypto class; it is defined here

const vector<uint8_t> Crypto::default_user_password_ =
{
  0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
  0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
  0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
  0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
};

//---------------------------------------------------------------------------//
// The Md5 algorithm uses pseudorandom numbers to chop its message into bytes.
// Having them in a vector stops us from having to call the Md5Mix_ function
// with each seperate number 64 times. These numbers come from the function
// md5_table[i] = abs(sin(i + 1)) * 2^32, but it is quicker to pre-compute them

const vector<FourBytes> Crypto::md5_table =
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
// More pseudorandom numbers for the Md5 hash

const std::vector<std::vector<FourBytes>> Crypto::mixarray =
{
  {7, 12, 17, 22},
  {5,  9, 14, 20},
  {4, 11, 16, 23},
  {6, 10, 15, 21},
};

//---------------------------------------------------------------------------//
// This simple function "chops" a four-byte int to a vector of four bytes.
// The bytes are returned lowest-order first as this is the typical use.

vector<uint8_t> Crypto::ChopLong_(FourBytes t_long_int) const
{
  // The mask specifies that only the last byte is read when used with &
  FourBytes mask = 0x000000ff;

  // Create a length-4 vector of bytes filled with low-high bytes from longint
  vector<uint8_t> result(4, 0);
  for (int i = 0; i < 4; ++i) result[i] = (t_long_int >> (8 * i)) & mask;

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

vector<uint8_t> Crypto::ReadPermissions_(std::string t_string)
{
  // No string == no permissions. Can't decode pdf, so throw an error
  if (t_string.empty()) throw runtime_error("No permission flags");

  // Convert the string to a 4-byte int
  int flags = stoi(t_string);

  // This reads off the bytes from lowest order to highest order
  return ChopLong_(flags);
}

/*---------------------------------------------------------------------------*/
// The Md5 algorithm produces a "hash" of 16 bytes from any given sequence
// of bytes. It is not practically possible to reverse the hash or find
// a random set of bytes that when passed through the function will match the
// hash. It is therefore like a "fingerprint" of any given plain data or string.
// You need to have the same data from which the hash was created in order to
// get the same hash. This allows passwords to be matched without the need for
// the actual password to be stored anywhere: a program or service can be
// protected just by insisting that a string is passed which matches the
// stored hash.
//
// The main work of the Md5 function is done by shuffling byte positions around
// and performing bitwise operations on them. It seems fairly random and
// arbitrary, but is completely deterministic so that a given set of bytes
// always produced the same output.
//
// This function is called several times with different parameters as part
// of the main Md5 algorithm. It can be considered a "shuffler" of bytes

void Crypto::Md5Mix_(int t_cycle,
                     deque<FourBytes>& t_deque,
                     vector<FourBytes>& t_fingerprint) const
{
  // Declare and define some pseudorandom numbers
  FourBytes mixer,
            e,
            f = md5_table[t_cycle],
            g = mixarray[t_cycle / 16][t_cycle % 4];

  // Mangle bytes in various ways as per Md5 algorithm
  switch (t_cycle / 16 + 1)
  {
    case 1  : e     = t_fingerprint[(1 * t_cycle + 0) % 16];
              mixer = (t_deque[0] + ((t_deque[1] & t_deque[2]) |
                      (~t_deque[1] & t_deque[3])) + e + f);
              break;

    case 2  : e     = t_fingerprint[(5 * t_cycle + 1) % 16];
              mixer = (t_deque[0] + ((t_deque[1] & t_deque[3]) |
                      (t_deque[2] & ~t_deque[3])) + e + f);
              break;

    case 3  : e     = t_fingerprint[(3 * t_cycle + 5) % 16];
              mixer = (t_deque[0] + (t_deque[1] ^ t_deque[2] ^
                       t_deque[3]) + e + f);
              break;

    case 4  : e     = t_fingerprint[(7 * t_cycle + 0) % 16];
              mixer = (t_deque[0] + (t_deque[2] ^
                      (t_deque[1] | ~t_deque[3])) + e + f);
              break;

    default: throw runtime_error("Md5 error: n > 63");
  }

  // further bit shuffling:
  t_deque[0] = t_deque[1] + (((mixer << g) | (mixer >> (32 - g))) & 0xffffffff);

  // now push all elements to the left (with aliasing)
  t_deque.push_front(t_deque.back());
  t_deque.pop_back();
}

/*---------------------------------------------------------------------------*/
// The main Md5 algorithm. This version of it was modified from various
// open source online implementations.

vector<uint8_t> Crypto::Md5_(vector<uint8_t> t_message) const
{
  // The length of the message
  int message_length = t_message.size();

  // 16 * FourBytes will contain "fingerprint"
  std::vector<FourBytes> fingerprint(16, 0);

  // The number of 64-byte blocks to be processed plus extra 8-byte filler
  int number_of_blocks = (message_length + 72) / 64;
                                 //
  // Starting pseudorandom numbers
  deque<FourBytes> mixvars = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

  // This next block fills the 16-element long "fingerprint" of the message
  // with up to 64 bytes from each block of the message, mangles them and
  // turns them into the pseudorandom seed for the next block
  int block, fourbyte_it, message_it = 0;
  for (block = 0; block < number_of_blocks; ++block)
  {
    // For each 4 bytes of block create a fourbyte from 4 bytes, low-order first
    for (fourbyte_it = 0;
         fourbyte_it < 16 && message_it < message_length - 3;
         fourbyte_it++, message_it += 4)
    {
      fingerprint[fourbyte_it] = (t_message[message_it + 3] << 24) |
                                 (t_message[message_it + 2] << 16) |
                                 (t_message[message_it + 1] <<  8) |
                                 (t_message[message_it + 0] <<  0) ;
    }

    // If this is the last block...
    if (block == number_of_blocks - 1)
    {
      // create a fourbyte from the remaining bytes (low order first), with a
      // left-sided padding of (binary) 10000000...
      if (message_it == message_length - 3)
      {
        fingerprint[fourbyte_it++] = 0x80000000                        |
                                     (t_message[message_it + 2] << 16) |
                                     (t_message[message_it + 1] <<  8) |
                                     (t_message[message_it + 0] <<  0) ;
      }
      else if (message_it == message_length - 2)
      {
        fingerprint[fourbyte_it++] = 0x800000                          |
                                     (t_message[message_it + 1] << 8)  |
                                     (t_message[message_it + 0] << 0)  ;
      }

      else if (message_it == message_length - 1)
      {
        fingerprint[fourbyte_it++] = 0x8000 + t_message[message_it];
      }
      else fingerprint[fourbyte_it++] = 0x80;

      // Right pad fingerprint with zeros to total 16 bytes
      while (fourbyte_it < 16) fingerprint[fourbyte_it++] = 0;

      // Store a mangled copy of length in the fingerprint
      fingerprint[14] = message_length << 3;
    }

    // Store a copy of initial random numbers
    deque<FourBytes> initial_deque = mixvars;

    // Shuffle fingerprint 64 times
    for (int i = 0; i < 64; i++) Md5Mix_(i, mixvars, fingerprint);

    // Add initial random numbers
    for (int i = 0; i < 4;  i++) mixvars[i] += initial_deque[i];
  }

  vector<uint8_t> output; // create empty output vector for output

  // Split the resultant 4 x FourBytes into a single 16-byte vector
  for (auto fourbyte : mixvars) Concatenate(output, ChopLong_(fourbyte));

  return output;
}

//----------------------------------------------------------------------------//
// This allows the Md5 function to be run on a std::string without prior
// conversion. It simply converts a string to bytes than puts it
// into the "bytes" version of the function

vector<uint8_t> Crypto::Md5_(std::string t_input) const
{
  return Md5_(vector<uint8_t>(t_input.begin(), t_input.end()));
}

//-------------------------------------------------------------------------//
// RC4 is a stream cipher used in encryption. It takes a string (or, as in this
// function, a vector of bytes) called a key, as well as the message to be
// scrambled. It uses the key as a seed from which to generate a seemingly
// random string of bytes. However, this stream of bytes can then be converted
// directly back into the original message using exactly the same key.
// The algorithm is now in the public domain

void Crypto::Rc4_(vector<uint8_t>& t_message, vector<uint8_t> t_key) const
{
  int key_length = t_key.size(), message_length = t_message.size();
  uint8_t a = 0, b = 0, x = 0, y = 0;

  // Create state and fill with 0 - 0xff
  vector<uint8_t> state;
  for (int i = 0; i <= 0xff; ++i) state.push_back(i);

  // No key - can't modify message
  if (key_length == 0) return;

  // for each element in state, mix the state around according to the key
  for (auto& element : state)
  {
    b = (t_key[a] + element + b) % 256;
    swap(element, state[b]);
    a = (a + 1) % key_length;
  }

  // For each character in the message, mix as per Rc4 algorithm
  for (int k = 0; k < message_length; k++)
  {
    uint8_t x1, y1;
    x1 = x = (x + 1) % 256;
    y1 = y = (state[x] + y) % 256;
    iter_swap(state.begin() + x1, state.begin() + y1);
    t_message[k] = t_message[k] ^ state[(state[x1] + state[y1]) % 256];
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
// through an Md5 hash. The first n bytes of the result, where n is the file
// key length plus 5, is then used as the key with which to decrypt the
// stream using the Rc4 algorithm.

void Crypto::DecryptStream(string& t_stream,
                           int t_object_number,
                           int t_object_gen) const
{
  // t_stream as bytes
  vector<uint8_t> stream_as_bytes(t_stream.begin(), t_stream.end());

  // Start building the object key with the file key
  vector<uint8_t> object_key = filekey_;

  // Append the bytes of the object number
  Concatenate(object_key, ChopLong_(t_object_number));

  // We only want the three lowest order bytes; pop the last
  object_key.pop_back();

  // Append lowest order byte of gen number
  object_key.push_back( t_object_gen & 0xff);

  // Then append the second lowest byte of gen number
  object_key.push_back((t_object_gen >> 8) & 0xff);

  // Store the object key's size
  uint8_t object_key_size = object_key.size();

  // Now Md5 hash the object key
  object_key = Md5_(object_key);

   // Trim the result to match the object key's size
  object_key.resize(object_key_size);

  // Now we use this key to decrypt the stream using Rc4
  Rc4_(stream_as_bytes, object_key);

  // finally we convert the resultant bytes back to a string
  t_stream = string(stream_as_bytes.begin(), stream_as_bytes.end());
}

/*---------------------------------------------------------------------------*/
// Gets the bytes comprising the hashed owner password from the encryption
// dictionary

vector<uint8_t> Crypto::ReadPassword_(const string& t_key)
{
   // Get raw bytes of owner password hash
  string password(encryption_dictionary_.GetString(t_key));
  string temporary_password;
  temporary_password.reserve(32);

  // This loop removes backslash escapes.
  for (auto iter = password.begin() + 1; iter != password.end(); ++iter)
  {
    if (*iter == '\\')
    {
      if (*(iter + 1) == '\\') temporary_password.push_back('\\');
    }
    else temporary_password.push_back(*iter);

    if (temporary_password.size() == 32)
    {
      swap(temporary_password, password);
      break;
    }
  }

  // The owner password should have 32 or more characters
  if (password.size() < 32) throw runtime_error("Corrupted password hash");

  // Return first 32 bytes (skip the first char which is the opening bracket)
  return vector<uint8_t>(password.begin(), password.end());
}

/*---------------------------------------------------------------------------*/
// The decryption key is needed to decrypt all streams except the xrefstream
// Its creation is described in ISO 3200 and implented here

void Crypto::ReadFileKey_()
{
  // Get the generic user password
  filekey_ = default_user_password_;

  // Stick the owner password on
  Concatenate(filekey_, ReadPassword_("/O"));

  // Stick permissions flags on
  auto permission_string = encryption_dictionary_.GetString("/P");
  Concatenate(filekey_, ReadPermissions_(permission_string));

  // Get first 16 bytes of file ID and stick them on too
  vector<uint8_t> id_bytes = ConvertHexToBytes(trailer_.GetString("/ID"));
  id_bytes.resize(16);
  Concatenate(filekey_, id_bytes);

  // now Md5 hash the result
  filekey_ = Md5_(filekey_);

  // Set the default filekey size
  size_t filekey_length = 5;

  // if the filekey length is not 5, it will be specified as a number of bits
  // so divide by 8 to get the number of bytes.
  if (encryption_dictionary_.ContainsInts("/Length"))
  {
    filekey_length = encryption_dictionary_.GetInts("/Length")[0] / 8;
  }

  // Resize the key as needed.
  filekey_.resize(filekey_length);
}

/*---------------------------------------------------------------------------*/
// This algorithm checks that the file key is correct by ensuring that an Rc4
// cipher of the default user password matches the user password hash in
// the encoding dictionary

void Crypto::CheckKeyR2_()
{
  // Get the pdf's hashed user password and the default user password
  vector<uint8_t> user_password_bytes = ReadPassword_("/U"),
                  test_answer = default_user_password_;

  // Rc4 the default user password using the supplied filekey
  Rc4_(test_answer, filekey_);

  // This should be the same as the pdf's hashed user password
  if (test_answer.size() == 32 && test_answer == user_password_bytes) return;

  // Otherwise, this key will not decrypt the pdf - throw an error
  throw runtime_error("Incorrect cryptkey");
}

/*---------------------------------------------------------------------------*/
// This is a more involved checking algorithm for higher levels of encryption.
// I couldn't get it to work for ages until I realised that the user and owner
// passwords sometimes contain backslash-escaped characters

void Crypto::CheckKeyR3_()
{
  // We start by Md5 hashing the filekey 50 times
  size_t key_length = filekey_.size();
  for (int i = 0; i < 50; ++i)
  {
    filekey_ = Md5_(filekey_);
    filekey_.resize(key_length);
  }

  // Next get the default user password
  vector<uint8_t> user_password = default_user_password_;

  // We now append the bytes from the ID entry of the trailer dictionary
  Concatenate(user_password, ConvertHexToBytes(trailer_.GetString("/ID")));

  // We only want the first 16 bytes from the ID so truncate to 48 bytes (32+16)
  user_password.resize(48);

  // As per ISO 32000 we now Md5 the result and Rc4 using the filekey
  user_password = Md5_(user_password);
  Rc4_(user_password, filekey_);

  // From ISO 32000: Take the result of the Rc4 and do the following 19 times:
  for (uint8_t iteration = 1; iteration < 20; ++iteration)
  {
    // Create a new key by doing an XOR of each byte of the filekey with
    // the iteration number (1 to 19) of the loop.
    vector<uint8_t> temp_key;
    for (auto byte : filekey_) temp_key.push_back(byte ^ iteration);

    // Create an Rc4 hash of the ongoing hash which is fed to next iteration
    Rc4_(user_password, temp_key);
  }

  // Now get the pdf's user password to test against our calculated password
  vector<uint8_t> test_against = ReadPassword_("/U");

  // Check the first 16 bytes of the two vectors to confirm the match
  for (int i = 0; i < 16; ++i)
  {
    // If any of the bytes do not match, we have the incorrect filekey
    if (test_against[i] != user_password[i])
    {
      throw runtime_error("cryptkey doesn't match");
    }
  }
}
