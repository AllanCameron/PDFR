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

## Reading bytes and reading bits
It is very easy to read a whole byte. Getting the value of the first byte would be done in C++ by getting `data[0]` or in R by `data[1]`. This will return an integer value between 0 and 255.

Unfortunately, the whole crux of the deflate algorithm is that we want symbols to be compressed into sequences of bits that are less than one byte. This means that we have to read chunks of bits from _within_ the bytes. Reading bits in chunks that aren't whole bytes is tricky and requires some familiarity with the bitwise operations "AND", "OR", "Left Shift" and "Right Shift", denoted `&`, `|`, `<<` and `>>` in C++, and accessed by the `bitwAnd()`, `bitwOr()`, `bitwShiftL()` and `bitwShiftR()` functions in R.

Reading particular bits of a byte usually means first creating a "mask" that, when applied using the bitwise AND function, will only read the bits of interest. For example, suppose I have the byte `my_byte = 235`, which is `11101011` in binary. If I want to read only the 5 leftmost ("low order"") bits then I want to mask out the 3 rightmost ("high order") bits by creating the mask `00011111`. This is 31. If I want to read 7 low-order bits then I create the mask `01111111`, which is 127. In general, if I want to read _x_ low order bits, my mask should be `mask = (1 << x) - 1`. Now I just need to do `mask & my_byte` to get the _x_ low order bits of `my_byte`. In this example, if I just wanted the five low order bits, then I would find that `31 & 235` is 11, so my five low order bits would be `01011`. Sure enough, these are the five lowest order bits in `my_byte`.

If I just want the high order bits, then I can mask out the low-order bits in a similar way. If I only want the three high-order bits, then I can create a mask to remove the five low-order bits by doing `0xff - ((1 << 5) - 1)`. However, I need to remember to bit-shift my answer to the right by 5 places. In our example, `((1 << 5) - 1) = 31` so the mask is `0xff - ((1 << 5) - 1) = 224` which is `11100000` in binary. Applying this to `my_byte` with `my_byte & 224` gives 224, since the three high order bits are `111`. Now, since I wanted the _value_ of the three high order bits, I want to know what `111` in binary is. To do this, I shift `111` to the right by five places using `224 >> 5` to get `00000111`, or 7.

## A note on bit ordering
Although we read individual bytes from left to right, the bits _within_ the bytes are read in chunks from right to left, which can be confusing. This means if I talk about the first 4 bits of a byte, I mean the 4 low order bytes. For example, the "first" 4 bits of the first byte would be obtained by `data[0] & 0x0f` in C++ or `bitwAnd(data[1], 0x0f)` in R. The bits themselves within a "chunk" are *not* reversed though, so if the first byte was made of the bits `00001010` then the first 4 bits should be interpreted as `1010` or 10, not as `0101` or 5.

Sometimes we need to read a chunk of bits from two adjacent bytes. What happens then? Well, we just take the high order bits from the first byte and stick the low-order bits from the next byte on to right side. For example, if my two bytes are `0xf0 0xca` or `11110000 11001010` in binary, then reading the first 4 bits of this sequence should give me `0000` or 0. If I want to read the next _8_ bits, I have to take the `1111` high order bits from the first byte and stick on the `1010` low order bits from the second byte to give me `10101111` or `0xaf`.


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


## The first byte

The first byte of a deflate-compressed stream declares the compression method flags, so is known as CMF. The first 4 bits declare the compression method (CM). The almost universal compression method is CM = 8, so the first byte should look like `xxxx1000`. 

The second four bits give the compression info. For CM = 8, you take these 4 bits and add 8. This gives you the log (to the base 2) of the LZ77 window size that was used to create the deflate stream. For example, if xxxx = 0111 = 7, the window size used was 2^(7 + 8) = 32,768.

In practice, since 32K is the maximum window size that can be used in CM = 8, this means the 4 high order bits are almost always `0111`, so the first byte is almost always `01111000` or `78`. This is the case in our data.

## The second byte

The first 5 bits of the second byte are known as FCHECK and are used as a checksum for the first two bytes. If you make a 16-bit integer using `256 * data[0] + data[1]`, then the resultant number should be divisible by 31 (i.e. `((data[0] << 8) | data[1]) % 31 == 0`. If it isn't, then something has gone wrong.

The 6th bit is a single-bit flag indicating whether a preset dictionary is in use (i.e. whether the flag is set is tested by `(data[1] & 0x10) >> 5 == 1`). As far as I can tell, this seems not to be implemented in the varieties of deflate used in pdf streams.

The remaining 2 highest order bits (`(data[1] & 0xc0) >> 6`) in the second byte denote the level of compression used, from 0 (fastest) to 3 (best compression). The default is `10` i.e. 2.

In our data, our second byte is `10011100`, meaning we are at default compression (`10`), no FDICT is set (`0`) and we have a valid header, since `256 * 0x78 + 0x9c` is 30876, which is 996 * 31.

## Third byte

We now enter a *block*. Each block is self-contained. It starts with a 3 bit header. The lowest order bit of this header flags whether this is the final block in the stream, i.e. `data[2] & 0x01 == 1` answers "is this the final block?"

The next two bits (`data[2] & 0x06`) tell us the type of compression. A value of 0 means no compression is used. A value of 1 is compression with a default compression dictionary, and a value of 2 is compression with a dynamic dictionary. There seems little point in having a "no compression" mode, and I'm not sure how often mode 0 is implemented in real life. The default dictionary (mode 1) is often used for very short messages with no repetition. If the compression mode is 1, you will need to have the default compression dictionary to look up the codes in the following stream. It is available online or in streams.cpp in this package. If the compression mode is 1, then after the three-byte block header, the codes will come straight away, ready for reading and you should skip to the "Reading the compressed data" section.

In our stream, the third byte begins with `1`, indicating that this is indeed the last block, and the next two bits are `10`, indicating that this block has its own dictionary encoded in the next few bits.

## Building a dynamic dictionary
The compressed data itself is going to consist of variable-length chunks of bits which represent symbols. Each symbol is either going to represent an actual byte in our message, or is going to be a pointer to a chunk of actual bytes that have already been decoded.

The variable-length chunks of bits - "codes", in the compressed data, have to be read by being looked up in a dictionary. This dictionary itself is tailor-made to make the codes in the compressed data as short as possible.

Before we can read the stream therefore, we need to know what the codes are and what they represent. Hence we have to build a dictionary.

### Getting the dictionary size
After our three header bits in the third byte have declared dynamic compression, we know that our next 5 bits will tell us how many "literal codes" there are from which we need to build our dictionary. What does this mean? Well, the literal codes are the actual bit-chunk codes that we are going to be reading from the compressed data. You would think they should range from 0 to 255 to represent the actual bytes of our message. However, they actually range from 0 to 285. Why? Well, we need a stop code (256), and we also need a method of indicating that we are entering a pointer to an earlier decoded sequence. The numbers 257 - 285 tell us that we are entering a pointer; what the actual numbers are isn't important right now. In our data, byte 3 is `01100101`, and we have already read the 3 low order bits, so now we read `01100`, or 12. Since there are a minimum of 257 literal codes for any message, we add our 12 to this number to get the number of literal codes we need in our dictionary: 269.

The next 5 bits in our sequence will indicate the number of _distance codes_ we will have in our dictionary. The distance codes need to be read only when we go into pointer mode. Since there can be anywhere from 1 to 32 distance codes, we need to add 1 to the 0 - 31 indicated by these five bits. In our case, this means we will have `01011` (11) + 1 = 12 distance codes.

### Populating the dictionary
OK, so now we know how big our dictionary is going to be. But how do we populate it? Unfortunately for us, the dictionary itself is compressed. You see, it turns out that you can completely specify the code dictionary just by specifying how many bits are needed to represent each particular literal value.

Since the number of bits needed for any one character will never exceed 15 (by design), you _could_ specify the code dictionary with 4-bits per literal value, giving each literal a number between 0 and 15 as the length of bits required to represent it. However, this would give you an awful lot of wasted bytes full of zeros representing the number of bits required to specify those literal values that never occur in the message.

Here's what happens instead. We use _another_ code to describe the bit-lengths of the final dictionary code table: the numbers 0 - 15 will represent an actual number of bits, and the numbers 16, 17, 18 will represent repeat sequences (mostly of zeros). I will start building my dictionary bit-length table at literal value 0, and if I come across the numbers 0-15, I insert them as the number of bits that are going to represent that literal in my final dictionary. I then move on to the next literal. If I come across a 16, 17, or 18, I will insert a number of repeats based on a set of rules for those 3 numbers. I keep going like this until my literal and distance codes all have an associated bit-length to describe them. 

Once I finish this, I will have a one to one mapping between number of bits and the literal code that it represented. From this, I can completely recreate the actual code dictionary.

So, what we need to do is find out how the length codes 0 - 18 are represented in the compressed dictionary. That is what we need to do first.

### Getting the code length codes

After the 5 bits that describe the length of the literal codes and the 5 bits that describe the distance codes come 4 bits telling us how many "code length codes" are about to be described. In our example, the four bits are `1100`, made from the lowest order bit of `data[4]` and the 3 highest order bits of `data[3]`. This is equal to 12, but since we have 4 bits trying to describe a number of code lengths that may be as high as 19 (since there may be a code length for each of the numbers 0 through 18), we add 4 to this. In our case, this means we can expect 16 code lengths to follow.

Each of these 16 code lengths are made up of three bits and are read as numbers. Let's do that now. I will label our 16 groups of 3 bits as 0 - f, and demonstrate the bits beloning to each group below each byte. The reading order of the bits are shown in the order |->, where "|" indicates the first bit of each group

```
data[4]:    00111101 00001110 10000000 00100000 00001100 01000110 10101111
group :     2111000- 44433322 77666555 a9998887 cccbbbaa ffeeeddd -------f
bits |->:   >|->|->  |->|->|- ->|->|-> >|->|->| |->|->|- ->|->|->        |
             1  0    4  3  2    6  5    9  8  7 c  b  a    e  d          f
```

This now gives us our code lengths, as shown in the following table:

| group | binary | decimal |
|-------|--------|---------|
|   0   |  110   |    6    |
|   1   |  011   |    3    |
|   2   |  100   |    4    |
|   3   |  011   |    3    |
|   4   |  000   |    0    |
|   5   |  000   |    0    |
|   6   |  000   |    0    |
|   7   |  010   |    2    |
|   8   |  000   |    0    |
|   9   |  010   |    2    |
|   a   |  000   |    0    |
|   b   |  011   |    3    |
|   c   |  000   |    0    |
|   d   |  110   |    6    |
|   e   |  000   |    0    |
|   f   |  101   |    5    |

Note the group names 0-f don't actually signify anything here other than being used for illustration purposes. What do the decimal numbers represent? They are the lengths of the binary codes that will be used to describe the numbers 0 to 18. How? We need to fill a length-19 array with the decimal numbers in the following order, given to us by the Deflate specification:
`{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};`
This order is chosen because these are the most-to-least commonly used symbols in the literal-length encoding system. For our data then, we would have:

```
 order {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}
 value {6,  3,  4,  3, 0, 0, 0, 2, 0,  2, 0,  3, 0,  6, 0,  5, 0,  0, 0 }
```

Note that we only had 16 values, so we append three zeros on the end to make it up to 19 (there are no codes of length 14, 1 or 15 in our literal table). Now rearranging the values according to the given order, we have:

```
code_lengths = {3, 0, 5, 6, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 3, 4};
```

This is sufficient to recreate the actual codes we are about to read to recreate the literal table. We do this by building a _Huffman tree_. There are several good descriptions online of how to recreate a Huffman tree from a given array of lengths, but in our case, this given set of lengths gives us the following Huffman tree:

| code | bits   |
|------|--------|
|   0  | 100    |
|   2  | 11110  |
|   3  | 111110 |
|   4  | 101    |
|   5  | 00     |
|   6  | 01     |
|  16  | 111111 |
|  17  | 110    |
|  18  | 1110   |

This now allows us to read the data which follows to get our literal code table. Let's go back to our data, starting where we left off at byte 10:

```
data[10] --> 10101111 11110010 01101101 00101100 11000100 01111011 01110000
```
We have already read the first bit of byte 10. We want to read the remaining bits until we have a match. Now, here's the kicker: the Huffman codes are packed into the bytes backwards. That means that when we take a chunk of bits we need to read it backwards. If we read the three low order bits of `11100110` then we need to interpret this as `011` for the purposes of looking up the Huffman code. I know, I know, this is confusing, but that's just how it is.

Remember, none of our codes are less than 2 bits long, so we start reading backwards from the 7th bit in byte 10, using two bits, and ask whether we have a match. `11` isn't in our table, so how about three bits? `111` isn't in our table either. How about `1110`? Yes! That represents code 18. 

Codes 16, 17 and 18 are different from the rest. They don't represent actual code lengths, but repeat sequences of actual lengths. Code 18 means we read the next seven bits and add 11 to get the number of zeros we want to add to our literal array. The next seven bits are _not_ bit reversed though (!), so we read them as "normal": `101` from the most significant 3 bits of byte 10, with the next 4 bits from the lower end of byte 11 `0010` being placed to the right to give `0010101` or 21. Adding 11 gives us 32, which means we want to start our literal length array with 32 zeros. 

Now we can read another code. `11`, `111`, `1111` and `11111` don't match, but `111110` does: it's code 3, so we place a 3 in our literal length array at literal_length[32]. This means that of all the ascii characters in our final message, the lowest will be 32, or 0x20, which is the space character. It will be represented by 3 bits when we come to decoding the actual compressed data.

Next comes `110` which is code 17. This means we have more zero repeats. The rules for code 17 are that we take the next three bits and add 3 to get the number of repeats. Here the next three bits are `011`, or 3, so we want to add 6 zeros to our array. 

Next up is `00`, which translates to length 5. We place this at position 39 and read the next code.

We go on like this until our literal and distance array is full. The full listing looks like this:

---

1) Read code 18 using 4 bits, consume next 7 bits to get repeat length; Write 32 zeros from position [0] to position [31]
2) Read code 3 using 6 bits; Write 3 at position [32]
3) Read code 17 using 3 bits, consume next 3 bits to get repeat length; Write 6 zeros from [33] to [38]
4) Read code 5 using 2 bits; Write 5 at position [39]
5) Read code 17 using 3 bits, consume next 3 bits to get repeat length; Write 4 zeros from [40] to [43]
6) Read code 5 using 2 bits; Write 5 at position [44]
7) Read code 0 using 3 bits; Write 0 at position [45]
8) Read code 6 using 2 bits; Write 6 at position [46]
9) Read code 18 using 4 bits, consume next 7 bits to get repeat length; Write 26 zeros from [47] to [72]
10) Read code 5 using 2 bits; Write 5 at position [73]
11) Read code 18 using 4 bits, consume next 7 bits to get repeat length; Write 23 zeros from [74] to [96]
12) Read code 4 using 3 bits; Write 4 at position [97]
13) Read code 0 using 3 bits; Write 0 at position [98]
14) Read code 5 using 2 bits; Write 5 at position [99]
15) Read code 6 using 2 bits; Write 6 at position [100]
16) Read code 4 using 3 bits; Write 4 at position [101]
17) Read code 0 using 3 bits; Write 0 at position [102]
18) Read code 6 using 2 bits; Write 6 at position [103]
19) Read code 5 using 2 bits; Write 5 at position [104]
20) Read code 5 using 2 bits; Write 5 at position [105]
21) Read code 0 using 3 bits; Write 0 at position [106]
22) Read code 6 using 2 bits; Write 6 at position [107]
23) Read code 5 using 2 bits; Write 5 at position [108]
24) Read code 5 using 2 bits; Write 5 at position [109]
25) Read code 4 using 3 bits; Write 4 at position [110]
26) Read code 4 using 3 bits; Write 4 at position [111]
27) Read code 5 using 2 bits; Write 5 at position [112]
28) Read code 0 using 3 bits; Write 0 at position [113]
29) Read code 6 using 2 bits; Write 6 at position [114]
30) Read code 4 using 3 bits; Write 4 at position [115]
31) Read code 4 using 3 bits; Write 4 at position [116]
32) Read code 6 using 2 bits; Write 6 at position [117]
33) Read code 17 using 3 bits, consume next 3 bits to get repeat length; Writing 3 zeros from [118] to [120]
34) Read code 6 using 2 bits; Write 6 at position [121]
35) Read code 18 using 4 bits, consume next 7 bits to get repeat length; Write 134 zeros from [122] to [255]
36) Read code 6 using 2 bits; Write 6 at position [256]
37) Read code 6 using 2 bits; Write 6 at position [257]
38) Read code 0 using 3 bits; Write 0 at position [258]
39) Read code 6 using 2 bits; Write 6 at position [259]
40) Read code 6 using 2 bits; Write 6 at position [260]
41) Read code 0 using 3 bits; Write 3 at position [261]
42) Read code 0 using 3 bits; Write 0 at position [262]
43) Read code 6 using 2 bits; Write 6 at position [263]
44) Read code 17 using 3 bits, consume next 3 bits to get repeat length; Write 4 zeros from [264] to [267]
45) Read code 5 using 2 bits; Write 5 at position [268]
46) Read code 17 using 3 bits, consume next 3 bits to get repeat length; Write 8 zeros from [269] to [276]
47) Read code 2 using 5 bits; Write 2 at position [277]
48) Read code 16 using 6 bits, consume next 2 bits to get repeat length; Write 3 twos from [278] to [280]

---

Now we have all of our lengths, we can generate our actual literal and distance tables by reconstructing their Huffman trees. Here are the final literal and distance codes from which we will construct our message:

---

### Literal Codes

literal code| bits
---|----
32 | 000
39 | 10000
44 | 10001
46 | 110100
73 | 10010
97 | 0010
99 | 10011
100 | 110101
101 | 0011
103 | 110110
104 | 10100
105 | 10101
107 | 110111
108 | 10110
109 | 10111
110 | 0100
111 | 0101
112 | 11000
114 | 111000
115 | 0110
116 | 0111
117 | 111001
121 | 111010
256 | 111011
257 | 111100
259 | 111101
260 | 111110
263 | 111111
268 | 11001


---

### Distance codes

|Distance Code | Bits  |
|--------------|-------|
|    8         |   00  |
|    9         |   01  |
|    10        |   10  |
|    11        |   11  |
    
    
---

We have read this table by consuming 179 bits, or 22 bytes + 3 bits. That takes us from the second bit of byte 10 to the 4th bit of byte 32. We now have 48 and 1/2 bytes left to squeeze in our compressed message:

```
Starting from here
                 |
                 v
data[32]  --> 10010011 01000010 10000111 10101000 10000011 00110000 00001010
data[39]  --> 10010011 10010001 00111000 01000110 00111011 11110011 11001110
data[46]  --> 01000111 01000100 01111010 11011100 11001100 10000011 11000001 
data[53]  --> 01010100 00100010 01001000 11010110 10110111 01010000 01101001 
data[60]  --> 11010111 11100111 10101010 01101100 01111111 01101101 11110000 
data[67]  --> 11011010 11100000 10000101 10100111 00011111 01011001 00111011
data[74]  --> 11011011 01110010 00000011 00001000 10101100 00101011 00010011
```

Now we are in a position to read the message. Let's try. The sequence starts `10010`, which corresponds to 73. We write this to our output. Next comes `10000` which is 39, `10111` which is 109, and `000` which is a 32.
