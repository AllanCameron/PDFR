# Inflating a deflate stream in a pdf

This document describes the steps needed to write a program to decompress a deflate stream.

We start with a vector of bytes called `data` containing the deflate stream, as you might find it lifted directly from a pdf file. This contains a message that in its original state is 121 bytes long. The compressed version has 81 bytes, so it is compressed to about 2/3 of its original size. This document will discuss in full detail how the message is reconstructed.

```
std::vector<uint8_t> data {
    0x78, 0x9c, 0x65, 0x8b, 0x3d, 0x0e, 0x80, 0x20, 0x0c, 0x46, 0xaf, 0xf2,
    0x6d, 0x2c, 0xc4, 0x7b, 0x70, 0x8c, 0x06, 0x1b, 0x21, 0x42, 0x4b, 0x6c,
    0x1d, 0xbc, 0xbd, 0x46, 0x13, 0x17, 0xd6, 0xf7, 0x93, 0x42, 0x87, 0xa8,
    0x83, 0x30, 0x0a, 0x93, 0x91, 0x38, 0x46, 0x3b, 0xf3, 0xce, 0x47, 0x44,
    0x7a, 0xdc, 0xcc, 0x83, 0xc1, 0x54, 0x22, 0x48, 0xd6, 0xb7, 0x50, 0x69,
    0xd7, 0xe7, 0xaa, 0x6c, 0x7f, 0x6d, 0xf0, 0xda, 0xe0, 0x85, 0xa7, 0x1f,
    0x59, 0x3b, 0xdb, 0x72, 0x03, 0x08, 0xac, 0x2b, 0x13};
```

Since the algorithm described below is implemented in C++, we will describe it using zero-indexed notation. This means the first byte will be denoted `data[0]`, the second byte `data[1]` etc.

## A note on bit ordering
Although as we are going through the vector bytes are read left to right, the bits _within_ the bytes are read in chunks from right to left, which can be confusing. This means if I talk about the first 4 bits of a byte, I mean the 4 low order bytes. For example, the "first"" 4 bits of the first byte would be obtained by `data[0] & 0x0f` in C++ or `bitwAnd(data[1], 0x0f)` in R. The bits themselves within a "chunk" are *not* reversed though, so if the first byte was made of the bits `00001010` then the first 4 bits should be interpreted as `1010` or 10, not as `0101` or 5.

Sometimes we need to read a chunk of bits from two adjacent bytes. What happens then? Well, we just take the high order bits from the first byte and stick the low-order bits from the next byte on to the end. For example, if my two bytes are `0xf0 0xca` or `11110000 11001010` in binary, then reading the first 4 bits of this sequence should give me `0000` or 0. If I want to read the next _8_ bits, I have to take the `1111` high order bits from the first byte and stick on the `1010` low order bits from the second byte to give me `11111010` or `0xfa`.

## Our data as bits
It's going to be much easier to see what's happening if we show the individual bits we are working through rather than just talking about bytes. Here is our compressed stream as bits:

```
data[0] ---> 01111000 10011100 01100101 10001011 00111101 00001110 10000000
data[7] ---> 00100000 00001100 01000110 10101111 11110010 01101101 00101100
data[14]---> 11000100 01111011 01110000 10001100 00000110 00011011 00100001
data[21]---> 01000010 01001011 01101100 00011101 10111100 10111101 01000110
data[28]---> 00010011 00010111 11010110 11110111 10010011 01000010 10000111
data[35]---> 10101000 10000011 00110000 00001010 10010011 10010001 00111000
data[42]---> 01000110 00111011 11110011 11001110 01000111 01000100 01111010
data[49]---> 11011100 11001100 10000011 11000001 01010100 00100010 01001000
data[56]---> 11010110 10110111 01010000 01101001 11010111 11100111 10101010
data[63]---> 01101100 01111111 01101101 11110000 11011010 11100000 10000101
data[70]---> 10100111 00011111 01011001 00111011 11011011 01110010 00000011
data[77]---> 00001000 10101100 00101011 00010011 <--- data[80]
        
```
This should make it clear which byte we are talking about at any point in the following discussion.

## Reading bytes and reading bits
It is very easy to read a whole byte. Getting the value of the first byte would be done in C++ by getting data[0] or in R by data[1]. This will return an integer value between 0 and 255.

Reading bits in chunks that aren't whole bytes is trickier and requires some familiarity with the bit manipulation functions `&`, `|`, `<<` and `>>` in C++ or `bitwAnd()`, `bitwOr()`, `bitwShiftL()` and `bitwShiftR()` in R. Reading particular bits of a byte usually means first creating a "mask" that, when applied using the bitwise AND function, will only read the bits of interest. For example, suppose I have the byte `my_byte = 235`, which is `11101011` in binary. If I want to read only the 5 low order bits then I want to mask out the 3 high order bits by creating the mask `00011111`. This is 31. If I want to read 7 low-order bits then I create the mask `01111111`, which is 127. In general, if I want to read _x_ low order bits, my mask should be `mask = (1 << x) - 1`. Now I just need to do `answer = mask & my_byte` to get the _x_ low order bits of `my_byte`. In this case, if I wanted the five low order bits, then I would find that `31 & 235` is 11, so my five low order bits would be `01011`. Sure enough, these are the five lowest order bits in `my_byte`.

Of course, we don't always want the low order bits. Suppose I have already "consumed" or read the five low-order bits of `my_byte` and want to know the "next" three bits (remember we are reading chunks of bits within a byte right-to-left even though we are reading the bytes left-to-right)


## The first byte

The first byte declares the compression method flags, so is known as CMF. The first 4 bits declare the compression method (CM). The almost universal compression method is CM = 8, so the first byte should look like `xxxx1000`. 

The second four bits give the compression info. For CM = 8, you take these 4 bits and add 8. This gives you the log (to the base 2) of the LZ77 window size that was used to create the deflate stream. For example, if xxxx = 0111 = 7, the window size used was 2^(7 + 8) = 32,768.

In practice, since 32K is the maximum window size that can be used in CM = 8, this means the 4 high order bits are almost always `0111`, so the first byte is almost always `01111000` or `78`

## The second byte

The first 5 bits of the second byte are known as FCHECK and are used as a checksum for the first two bytes. If you make a 16-bit integer using `256 * data[0] + data[1]`, then the resultant number should be divisible by 31 (i.e. `((data[0] << 8) | data[1]) % 31 == 0`. If it isn't, then something has gone wrong.

The 6th bit is a single-bit flag indicating whether a preset dictionary is in use (i.e. whether the flag is set is tested by `(data[1] & 0x10) >> 5 == 1`).

The remaining 2 highest order bits (`(data[1] & 0xc0) >> 6`) in the second byte denote the level of compression used, from 0 (fastest) to 3 (best compression). The default is `10` i.e. 2.

## Third and fourth bytes

We now enter a *block*. Each block is self-contained. It starts with a 3 bit header. The lowest order bit of this header flags whether this is the final block in the stream, i.e. `data[2] & 0x01 == 1` answers "is this the final block?"

The next two bits (`data[2] & 0x06`) tell us the type of compression. A value of 0 means no compression is used.
