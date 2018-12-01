#include "pdfr.h"
#include "stringfunctions.h"
#include "streams.h"
#include "crypto.h"



std::vector<uint8_t> perm(std::string str)
  {
    int a = std::stoi(str);
    char b = a & 0xff;
    char c = (a >> 8) & 0xff;
    char d = (a >> 16) & 0xff;
    char e = (a >> 24) & 0xff;
    std::vector<uint8_t> res;
    res.push_back(b);
    res.push_back(c);
    res.push_back(d);
    res.push_back(e);
    return res;
  }


/*---------------------------------------------------------------------------*/


std::vector<uint8_t> upw()
  {
    uint8_t c[32] = {
                      0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
                      0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
                      0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
                      0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
                    };
    std::vector<uint8_t> s;
    for(int i = 0; i < 32; i++) s.push_back(c[i]);
    return s;
  }


/*---------------------------------------------------------------------------*/


unsigned long rotateLeft(unsigned long x, int y)
  {
    x &= 4294967295;
    return ((x << y) | (x >> (32 - y))) & 4294967295;
  }


/*---------------------------------------------------------------------------*/


unsigned long md5first(unsigned long a, unsigned long b, unsigned long c,
                       unsigned long d, unsigned long e, unsigned long f,
                       unsigned long g)
  {
    return b + rotateLeft((a + ((b & c) | (~b & d)) + e + g), f);
  }


/*---------------------------------------------------------------------------*/


unsigned long md5second(unsigned long a, unsigned long b, unsigned long c,
                        unsigned long d, unsigned long e, unsigned long f,
                        unsigned long g)
  {
    return b + rotateLeft((a + ((b & d) | (c & ~d)) + e + g), f);
  }


/*---------------------------------------------------------------------------*/


unsigned long md5third(unsigned long a, unsigned long b, unsigned long c,
                       unsigned long d, unsigned long e, unsigned long f,
                       unsigned long g)
  {
    return b + rotateLeft((a + (b ^ c ^ d) + e + g), f);
  }


/*---------------------------------------------------------------------------*/


unsigned long md5fourth(unsigned long a, unsigned long b, unsigned long c,
                        unsigned long d, unsigned long e, unsigned long f,
                        unsigned long g)
  {
    return b + rotateLeft((a + (c ^ (b | ~d)) + e + g), f);
  }


/*---------------------------------------------------------------------------*/


std::vector<uint8_t> md5(std::vector<uint8_t> input)
  {
    int len = input.size();
    std::vector<unsigned long> x {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int nblocks = (len + 72) / 64;
    unsigned long a = 1732584193;
    unsigned long b = 4023233417;
    unsigned long c = 2562383102;
    unsigned long d = 271733878;

    int i, j, k;
    k = 0;
    for (i = 0; i < nblocks; ++i)
      {
        for (j = 0; j < 16 && k < len - 3; ++j, k += 4)
          {
            x.at(j) = (((((input.at(k + 3) << 8) +
                      input.at(k + 2)) << 8) +
                      input.at(k + 1)) << 8) +
                      input.at(k);
          }
        if (i == nblocks - 1)
          {
            if (k == len - 3)
              {
                x.at(j) = 0x80000000 +
                          (((input.at(k + 2) << 8) +
                          input.at(k + 1)) << 8) +
                          input.at(k);
              }
            else if (k == len - 2)
              {
                x.at(j) = 0x800000 + (input.at(k + 1) << 8) + input.at(k);
              }
            else if (k == len - 1)
              {
                x.at(j) = 0x8000 + input.at(k);
              }
            else
              {
                x.at(j) = 0x80;
              }
            j++;
            while (j < 16)
              {
                x.at(j++) = 0;
              }
            x.at(14) = len << 3;
          }

        unsigned long astore = a;
        unsigned long bstore = b;
        unsigned long cstore = c;
        unsigned long dstore = d;

        a = md5first(a, b, c, d, x.at(0),   7, 3614090360);
        d = md5first(d, a, b, c, x.at(1),  12, 3905402710);
        c = md5first(c, d, a, b, x.at(2),  17, 606105819);
        b = md5first(b, c, d, a, x.at(3),  22, 3250441966);
        a = md5first(a, b, c, d, x.at(4),   7, 4118548399);
        d = md5first(d, a, b, c, x.at(5),  12, 1200080426);
        c = md5first(c, d, a, b, x.at(6),  17, 2821735955);
        b = md5first(b, c, d, a, x.at(7),  22, 4249261313);
        a = md5first(a, b, c, d, x.at(8),   7, 1770035416);
        d = md5first(d, a, b, c, x.at(9),  12, 2336552879);
        c = md5first(c, d, a, b, x.at(10), 17, 4294925233);
        b = md5first(b, c, d, a, x.at(11), 22, 2304563134);
        a = md5first(a, b, c, d, x.at(12),  7, 1804603682);
        d = md5first(d, a, b, c, x.at(13), 12, 4254626195);
        c = md5first(c, d, a, b, x.at(14), 17, 2792965006);
        b = md5first(b, c, d, a, x.at(15), 22, 1236535329);
        a = md5second(a, b, c, d, x.at(1),   5, 4129170786);
        d = md5second(d, a, b, c, x.at(6),   9, 3225465664);
        c = md5second(c, d, a, b, x.at(11), 14, 643717713);
        b = md5second(b, c, d, a, x.at(0),  20, 3921069994);
        a = md5second(a, b, c, d, x.at(5),   5, 3593408605);
        d = md5second(d, a, b, c, x.at(10),  9, 38016083);
        c = md5second(c, d, a, b, x.at(15), 14, 3634488961);
        b = md5second(b, c, d, a, x.at(4),  20, 3889429448);
        a = md5second(a, b, c, d, x.at(9),   5, 568446438);
        d = md5second(d, a, b, c, x.at(14),  9, 3275163606);
        c = md5second(c, d, a, b, x.at(3),  14, 4107603335);
        b = md5second(b, c, d, a, x.at(8),  20, 1163531501);
        a = md5second(a, b, c, d, x.at(13),  5, 2850285829);
        d = md5second(d, a, b, c, x.at(2),   9, 4243563512);
        c = md5second(c, d, a, b, x.at(7),  14, 1735328473);
        b = md5second(b, c, d, a, x.at(12), 20, 2368359562);
        a = md5third(a, b, c, d, x.at(5),   4, 4294588738);
        d = md5third(d, a, b, c, x.at(8),  11, 2272392833);
        c = md5third(c, d, a, b, x.at(11), 16, 1839030562);
        b = md5third(b, c, d, a, x.at(14), 23, 4259657740);
        a = md5third(a, b, c, d, x.at(1),   4, 2763975236);
        d = md5third(d, a, b, c, x.at(4),  11, 1272893353);
        c = md5third(c, d, a, b, x.at(7),  16, 4139469664);
        b = md5third(b, c, d, a, x.at(10), 23, 3200236656);
        a = md5third(a, b, c, d, x.at(13),  4, 681279174);
        d = md5third(d, a, b, c, x.at(0),  11, 3936430074);
        c = md5third(c, d, a, b, x.at(3),  16, 3572445317);
        b = md5third(b, c, d, a, x.at(6),  23, 76029189);
        a = md5third(a, b, c, d, x.at(9),   4, 3654602809);
        d = md5third(d, a, b, c, x.at(12), 11, 3873151461);
        c = md5third(c, d, a, b, x.at(15), 16, 530742520);
        b = md5third(b, c, d, a, x.at(2),  23, 3299628645);
        a = md5fourth(a, b, c, d, x.at(0),   6, 4096336452);
        d = md5fourth(d, a, b, c, x.at(7),  10, 1126891415);
        c = md5fourth(c, d, a, b, x.at(14), 15, 2878612391);
        b = md5fourth(b, c, d, a, x.at(5),  21, 4237533241);
        a = md5fourth(a, b, c, d, x.at(12),  6, 1700485571);
        d = md5fourth(d, a, b, c, x.at(3),  10, 2399980690);
        c = md5fourth(c, d, a, b, x.at(10), 15, 4293915773);
        b = md5fourth(b, c, d, a, x.at(1),  21, 2240044497);
        a = md5fourth(a, b, c, d, x.at(8),   6, 1873313359);
        d = md5fourth(d, a, b, c, x.at(15), 10, 4264355552);
        c = md5fourth(c, d, a, b, x.at(6),  15, 2734768916);
        b = md5fourth(b, c, d, a, x.at(13), 21, 1309151649);
        a = md5fourth(a, b, c, d, x.at(4),   6, 4149444226);
        d = md5fourth(d, a, b, c, x.at(11), 10, 3174756917);
        c = md5fourth(c, d, a, b, x.at(2),  15, 718787259);
        b = md5fourth(b, c, d, a, x.at(9),  21, 3951481745);

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
      for(unsigned i = 0; i < input.length(); i++)
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
    while(objkey.size() > objkeysize) {objkey.pop_back();}
    std::vector<uint8_t> bytevec = rc4(streambytes, objkey);
    std::string restring =  bytestostring(bytevec);
    return restring;
  }


/*---------------------------------------------------------------------------*/
