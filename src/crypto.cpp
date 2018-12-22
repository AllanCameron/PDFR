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

#include "pdfr.h"
#include "stringfunctions.h"
#include "streams.h"
#include "crypto.h"

#define unsigned long UL;

std::vector<uint8_t> perm(std::string str)
{
  if(str.length() == 0)
    throw std::runtime_error("Could not determine permission flags");
  int a = stoi(str);
  uint8_t b = a & 0xff;
  uint8_t c = (a >> 8) & 0xff;
  uint8_t d = (a >> 16) & 0xff;
  uint8_t e = (a >> 24) & 0xff;
  std::vector<uint8_t> res = {b, c, d, e};
  return res;
}

/*---------------------------------------------------------------------------*/

std::vector<uint8_t> upw()
{
  uint8_t c[32] = { 0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
                    0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
                    0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
                    0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A };
  std::vector<uint8_t> s;
  for(int i = 0; i < 32; i++)
    s.push_back(c[i]);
  return s;
}

/*---------------------------------------------------------------------------*/

UL md5mix(int n, UL a, UL b, UL c, UL d, UL e, UL f, UL g)
{
  UL mixer;
  switch(n)
  {
    case 1  : mixer = (a + ((b & c) | (~b & d)) + e + f); break;
    case 2  : mixer = (a + ((b & d) | (c & ~d)) + e + f); break;
    case 3  : mixer = (a + (b ^ c ^ d) + e + f); break;
    case 4  : mixer = (a + (c ^ (b | ~d)) + e + f); break;
    default : throw std::runtime_error("MD5 error");
  }
  mixer &= 4294967295;
  return b + (((mixer << g) | (mixer >> (32 - g))) & 4294967295);
}

/*---------------------------------------------------------------------------*/

std::vector<uint8_t> md5(std::vector<uint8_t> input)
{
  int len = input.size();
  std::vector<UL> x {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int nblocks = (len + 72) / 64;
  UL a = 1732584193;
  UL b = 4023233417;
  UL c = 2562383102;
  UL d = 271733878;

  int i, j, k;
  k = 0;
  for (i = 0; i < nblocks; ++i)
  {
    for (j = 0; j < 16 && k < len - 3; ++j, k += 4)
      x.at(j) = (((((input.at(k + 3) << 8) +
                input.at(k + 2)) << 8) +
                input.at(k + 1)) << 8) +
                input.at(k);
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

    UL astore = a;
    UL bstore = b;
    UL cstore = c;
    UL dstore = d;

    a = md5mix(1, a, b, c, d, x.at(0), 3614090360, 7);
    d = md5mix(1, d, a, b, c, x.at(1), 3905402710, 12);
    c = md5mix(1, c, d, a, b, x.at(2), 606105819, 17);
    b = md5mix(1, b, c, d, a, x.at(3), 3250441966, 22);
    a = md5mix(1, a, b, c, d, x.at(4), 4118548399, 7);
    d = md5mix(1, d, a, b, c, x.at(5), 1200080426, 12);
    c = md5mix(1, c, d, a, b, x.at(6), 2821735955, 17);
    b = md5mix(1, b, c, d, a, x.at(7), 4249261313, 22);
    a = md5mix(1, a, b, c, d, x.at(8), 1770035416, 7);
    d = md5mix(1, d, a, b, c, x.at(9), 2336552879, 12);
    c = md5mix(1, c, d, a, b, x.at(10), 4294925233, 17);
    b = md5mix(1, b, c, d, a, x.at(11), 2304563134, 22);
    a = md5mix(1, a, b, c, d, x.at(12), 1804603682, 7);
    d = md5mix(1, d, a, b, c, x.at(13), 4254626195, 12);
    c = md5mix(1, c, d, a, b, x.at(14), 2792965006, 17);
    b = md5mix(1, b, c, d, a, x.at(15), 1236535329, 22);
    a = md5mix(2, a, b, c, d, x.at(1), 4129170786, 5);
    d = md5mix(2, d, a, b, c, x.at(6), 3225465664, 9);
    c = md5mix(2, c, d, a, b, x.at(11), 643717713, 14);
    b = md5mix(2, b, c, d, a, x.at(0), 3921069994, 20);
    a = md5mix(2, a, b, c, d, x.at(5), 3593408605, 5);
    d = md5mix(2, d, a, b, c, x.at(10), 38016083, 9);
    c = md5mix(2, c, d, a, b, x.at(15), 3634488961, 14);
    b = md5mix(2, b, c, d, a, x.at(4), 3889429448, 20);
    a = md5mix(2, a, b, c, d, x.at(9), 568446438, 5);
    d = md5mix(2, d, a, b, c, x.at(14), 3275163606, 9);
    c = md5mix(2, c, d, a, b, x.at(3), 4107603335, 14);
    b = md5mix(2, b, c, d, a, x.at(8), 1163531501, 20);
    a = md5mix(2, a, b, c, d, x.at(13), 2850285829, 5);
    d = md5mix(2, d, a, b, c, x.at(2), 4243563512, 9);
    c = md5mix(2, c, d, a, b, x.at(7), 1735328473, 14);
    b = md5mix(2, b, c, d, a, x.at(12), 2368359562, 20);
    a = md5mix(3, a, b, c, d, x.at(5), 4294588738, 4);
    d = md5mix(3, d, a, b, c, x.at(8), 2272392833, 11);
    c = md5mix(3, c, d, a, b, x.at(11), 1839030562, 16);
    b = md5mix(3, b, c, d, a, x.at(14), 4259657740, 23);
    a = md5mix(3, a, b, c, d, x.at(1), 2763975236, 4);
    d = md5mix(3, d, a, b, c, x.at(4), 1272893353, 11);
    c = md5mix(3, c, d, a, b, x.at(7), 4139469664, 16);
    b = md5mix(3, b, c, d, a, x.at(10), 3200236656, 23);
    a = md5mix(3, a, b, c, d, x.at(13), 681279174, 4);
    d = md5mix(3, d, a, b, c, x.at(0), 3936430074, 11);
    c = md5mix(3, c, d, a, b, x.at(3), 3572445317, 16);
    b = md5mix(3, b, c, d, a, x.at(6), 76029189, 23);
    a = md5mix(3, a, b, c, d, x.at(9), 3654602809, 4);
    d = md5mix(3, d, a, b, c, x.at(12), 3873151461, 11);
    c = md5mix(3, c, d, a, b, x.at(15), 530742520, 16);
    b = md5mix(3, b, c, d, a, x.at(2), 3299628645, 23);
    a = md5mix(4, a, b, c, d, x.at(0), 4096336452, 6);
    d = md5mix(4, d, a, b, c, x.at(7), 1126891415, 10);
    c = md5mix(4, c, d, a, b, x.at(14), 2878612391, 15);
    b = md5mix(4, b, c, d, a, x.at(5), 4237533241, 21);
    a = md5mix(4, a, b, c, d, x.at(12), 1700485571, 6);
    d = md5mix(4, d, a, b, c, x.at(3), 2399980690, 10);
    c = md5mix(4, c, d, a, b, x.at(10), 4293915773, 15);
    b = md5mix(4, b, c, d, a, x.at(1), 2240044497, 21);
    a = md5mix(4, a, b, c, d, x.at(8), 1873313359, 6);
    d = md5mix(4, d, a, b, c, x.at(15), 4264355552, 10);
    c = md5mix(4, c, d, a, b, x.at(6), 2734768916, 15);
    b = md5mix(4, b, c, d, a, x.at(13), 1309151649, 21);
    a = md5mix(4, a, b, c, d, x.at(4), 4149444226, 6);
    d = md5mix(4, d, a, b, c, x.at(11), 3174756917, 10);
    c = md5mix(4, c, d, a, b, x.at(2), 718787259, 15);
    b = md5mix(4, b, c, d, a, x.at(9), 3951481745, 21);

    a += astore;
    b += bstore;
    c += cstore;
    d += dstore;
  }

  std::vector<uint8_t> output;

  output.push_back((uint8_t)(a & 255));
  output.push_back((uint8_t)((a >>= 8) & 255));
  output.push_back((uint8_t)((a >>= 8) & 255));
  output.push_back((uint8_t)((a >>= 8) & 255));
  output.push_back((uint8_t)(b & 255));
  output.push_back((uint8_t)((b >>= 8) & 255));
  output.push_back((uint8_t)((b >>= 8) & 255));
  output.push_back((uint8_t)((b >>= 8) & 255));
  output.push_back((uint8_t)(c & 255));
  output.push_back((uint8_t)((c >>= 8) & 255));
  output.push_back((uint8_t)((c >>= 8) & 255));
  output.push_back((uint8_t)((c >>= 8) & 255));
  output.push_back((uint8_t)(d & 255));
  output.push_back((uint8_t)((d >>= 8) & 255));
  output.push_back((uint8_t)((d >>= 8) & 255));
  output.push_back((uint8_t)((d >>= 8) & 255));

  return output;
}

//----------------------------------------------------------------------------//

std::vector<uint8_t> md5(std::string input)
{
    std::vector<uint8_t> res;
    for(size_t i = 0; i < input.length(); i++)
    {
      char a = input[i];
      int b = a;
      uint8_t c = b;
      res.push_back(c);
    }
    return md5(res);
}

//-------------------------------------------------------------------------//

std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key)
  {
    int keyLen = key.size();
    int msgLen = msg.size();
    uint8_t a, b, t;
    int i;
    std::vector<uint8_t> state;
    for (i = 0; i < 256; ++i) state.push_back(i);
    if (keyLen == 0) return state;
    a = b = 0;
    for (i = 0; i < 256; ++i)
      {
        b = (key[a] + state[i] + b) % 256;
        t = state[i];
        state[i] = state[b];
        state[b] = t;
        a = (a + 1) % keyLen;
      }

    uint8_t x = 0;
    uint8_t y = 0;
    std::vector<uint8_t> res = msg;
    for(int k = 0; k < msgLen; k++)
      {
        uint8_t x1, y1, tx, ty;
        x1 = x = (x + 1) % 256;
        y1 = y = (state[x] + y) % 256;
        tx = state[x1];
        ty = state[y1];
        state[x1] = ty;
        state[y1] = tx;
        res[k] = res[k] ^ state[(tx + ty) % 256];
      }
    return res;
  }

/*---------------------------------------------------------------------------*/

std::string decryptStream(std::string streamstr, std::vector<uint8_t> key,
                             int objNum, int objGen)
  {
    std::vector<uint8_t> streambytes = stringtobytes(streamstr);
    std::vector<uint8_t> objkey = key;
    objkey.push_back(objNum & 0xff);
    objkey.push_back( (objNum >> 8) & 0xff);
    objkey.push_back( (objNum >> 16) & 0xff);
    objkey.push_back( objGen & 0xff);
    objkey.push_back((objGen >> 8) & 0xff);
    uint8_t objkeysize = objkey.size();
    objkey = md5(objkey);
    while(objkey.size() > objkeysize) objkey.pop_back();
    std::vector<uint8_t> bytevec = rc4(streambytes, objkey);
    std::string restring =  bytestostring(bytevec);
    return restring;
  }

/*---------------------------------------------------------------------------*/
