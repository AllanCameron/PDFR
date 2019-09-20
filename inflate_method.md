# Inflating a deflate stream in a pdf

This document describes the steps needed to decompress a deflate stream. Deflate streams are an extremely common way of compressing data, and are an integral part of the gzip and zip file formats. There are some excellent, fast, portable, open-source and well-tested deflate libraries available; it is almost certainly better to use one of these rather than reinventing the wheel.

However, there may be some situations in which a developer wants to write a simple, dependency-free deflate decompressor. Having found myself in this position and encountering many difficulties along the way (probably because I'm _not_ a developer...), I thought it would be helpful to write down the steps explicitly to help anyone else in a similar position.

I will try to describe this method without reference to any particular programming language. However, there are a couple of conventions that I need to choose so that I can consistently represent the steps I am taking.

## Conventions and basics
Firstly, I need a way of representing the data we are putting into the program and getting out of it. Although a decompressed deflate stream might represent a string of text, it can actually represent any sequence of binary information. Whether the output is a text string or some other type of information is something that the algorithm doesn't need to know. It takes in data as _numbers_ and outputs data as _numbers_. Specifically, it takes in only a sequence of integers from 0 to 255 and outputs only a sequence of integers from 0 to 255. During the decoding step, it only ever uses positive integers, and never needs to remember any numbers higher than 32768 (which is 2^15).

For example, if I feed the following sequence in to a deflate decompressor:
`120 156 243  72 205 201 201  87  40 207  47 202  73   1   0  24 171   4  61`
then the output would be: `72 101 108 108 111  32 119 111 114 108 100`. If I know that this message was to be intended as a string of characters, then I would look up the Ascii value of each of these numbers to find the message is "Hello world". However, the decompressor itself doesn't need to know this sequence of numbers was meant to be an Ascii string.

Since the numbers to be fed into the decompressor are individual, addressable, positive integers between 0 and 255, we talk about each number as being a **byte**. A byte is the smallest chunk of data that a computer can read, send or process at any one time, and is made of eight **bits** or binary digits.

When I am talking about **bits** in this document, I will show all the bits in a byte as a sequence of 8 ones and zeroes like this: `00110101`. If you aren't used to reading binary, this number represents 53. Reading it in the normal direction from left to right, you are reading it from "most significant bit" to "least significant bit".

We have to be careful when we are talking about the digital representation of numbers in the context of a deflate stream. If I talk about the number 53 without further qualification, then its binary representation could be `00110101`, but it could also be `110101` or `0000000000110101`. This could make a huge difference to how the data stream is interpreted. For that reason, if I am specifically talking about those 8-bit numbers that are directly addressable by a computer program (i.e. bytes), then I will follow the convention of using hexadecimal numbers from `0x00` to `0xff`. If I am talking about numbers that are not whole bytes, I will use either binary notation or natural numbers to describe them.

I also need to choose conventions to describe the data we are putting into and getting out of the algorithm. I use the term "stream" loosely to mean a sequence of bytes, either being read from or written to. Since I am using a concrete example here, I will assume our input stream consists of an array (equivalently a vector) of bytes called `data`. Any single element in the array is identified using square brackets: `data[13]` means the element at position 13. 

Many languages such as C, C++, Javascript and Java use zero-indexing. This means the first byte in an array will be denoted `data[0]`, the second byte `data[1]` etc. This is in contrast to R, FORTRAN, Matlab and a few others that start counting at one, so that the first element is `data[1]`, the second is `data[2]` etc. I will arbitrarily choose the former.

## Stating the problem
We start with a vector of bytes called `data` containing the deflate stream. This contains a compressed message that in its original uncompressed state is 121 bytes long. The compressed version has 81 bytes, so it is compressed to about 2/3 of its original size.


```
data = [0x78, 0x9c, 0x65, 0x8b, 0x3d, 0x0e, 0x80, 0x20, 0x0c, 0x46, 0xaf, 0xf2,
        0x6d, 0x2c, 0xc4, 0x7b, 0x70, 0x8c, 0x06, 0x1b, 0x21, 0x42, 0x4b, 0x6c,
        0x1d, 0xbc, 0xbd, 0x46, 0x13, 0x17, 0xd6, 0xf7, 0x93, 0x42, 0x87, 0xa8,
        0x83, 0x30, 0x0a, 0x93, 0x91, 0x38, 0x46, 0x3b, 0xf3, 0xce, 0x47, 0x44,
        0x7a, 0xdc, 0xcc, 0x83, 0xc1, 0x54, 0x22, 0x48, 0xd6, 0xb7, 0x50, 0x69,
        0xd7, 0xe7, 0xaa, 0x6c, 0x7f, 0x6d, 0xf0, 0xda, 0xe0, 0x85, 0xa7, 0x1f,
        0x59, 0x3b, 0xdb, 0x72, 0x03, 0x08, 0xac, 0x2b, 0x13];
```

Now, how do we take these bytes and convert them back into the original message?

First, we will have a quick refresher on some basic programming tricks that will help us accomplish this task.


## Reading bytes and reading bits
It is very easy to read a whole byte from our array. Getting the value of the first byte is as simple as `data[0]`. As long as we interpret this byte as an unsigned integer, this will return a value between 0 and 255.

Unfortunately, the whole crux of the deflate algorithm is that we want symbols to be compressed into sequences of bits that are _less_ than one byte. This means that we need to be able to read chunks of bits from _within_ the bytes. Reading bits in chunks that aren't whole bytes requires a bit more care than reading whole bytes, and requires some familiarity with the bitwise operations "AND", "OR", "Left Shift" and "Right Shift", usually denoted `&`, `|`, `<<` and `>>` in various languages such as C, C++, Java, Javascript, etc.

Reading particular bits of a byte usually means first creating a "mask" that, when applied using the bitwise AND function, will only read the bits of interest. For example, suppose I have the byte `my_byte = 0xd5`, which is `11010101` in binary. If I want to read only the 5 rightmost ("low order"") bits then I want to mask out the 3 leftmost ("high order") bits by creating the mask `00011111`. This is 31. If I want to read 7 low-order bits then I create the mask `01111111`, which is 127. In general, if I want to read _x_ low order bits, my mask should be `mask = (1 << x) - 1`. Once I have the mask, I just need to do `mask & my_byte` to get the _x_ low order bits of `my_byte`. In this example, if I just wanted the five low order bits, then I would find that `31 & 213` is 21, so my five low order bits would be `10101`. Sure enough, these are the five lowest order bits in `my_byte`.

If I just want the high order bits, then I can mask out the low-order bits in a similar way. If I only want the three high-order bits, then I can create a mask to remove the five low-order bits by doing `0xff - ((1 << 5) - 1)`. However, I need to remember to bit-shift my answer to the right by 5 places. In our example, `((1 << 5) - 1) = 31` so the mask is `0xff - ((1 << 5) - 1) = 224` which is `11100000` in binary. Applying this to `my_byte` with `my_byte & 224` gives 192, since the three high order bits are `110`. Now, since I wanted the _value_ of the three high order bits, I want to know what `110` in binary is. To do this, I shift `110` to the right by five places using `192 >> 5` to get `00000110`, or 6.

## A note on bit ordering
Although we read individual bytes from left to right, the bits _within_ the bytes are read in chunks from right to left, which can be confusing. This means if I talk about the first 4 bits of a byte, I mean the 4 low order bytes, or the 4 bits on the right as I look at the binary number. For example, the "first" 4 bits of the first byte would be obtained by `data[0] & 0x0f` in C++ or `bitwAnd(data[1], 0x0f)` in R. The bits themselves within a "chunk" are *not* reversed though, so if the first byte was made of the bits `00001010` then the first 4 bits should be interpreted as `1010` or 10, not as `0101` or 5.

Sometimes we need to read a chunk of bits from two adjacent bytes. What happens then? Well, we just take the high order (leftmost) bits from the first byte and stick the low-order bits from the next byte on to their left side. For example, if my two bytes are `0xf0 0xca` or `11110000 11001010` in binary, then reading the first 4 bits of this sequence should give me `0000` or 0. If I want to read the next _8_ bits, I have to take the `1111` high-order bits from the first byte and stick on the `1010` low order bits from the second byte to give me `10101111` or `0xaf`.

Despite what is written in various online resources, it is possible to stick entirely to this bit-ordering convention when reading the deflate stream.

## Our data as bits
We now have enough information to start reading our stream. It's going to be much easier to see what's happening if we show the individual bits we are working through rather than just talking about bytes. Here is our compressed stream as bits:

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
This should make it clear which byte (and which bits in it) we are talking about at any point in the following discussion.

## The two header bytes
### The first byte
The first byte of a deflate-compressed stream declares the compression method flags, so is known as CMF. The first 4 bits declare the compression method (CM). The almost universal compression method is CM = 8, so the first byte should look like `xxxx1000`.

The second four bits give the compression info. For CM = 8, you take these 4 bits and add 8. This gives you the log (to the base 2) of the LZ77 window size that was used to create the deflate stream. For inflation purposes, you don't need to know what this means, but to illustrate, if xxxx = 0111 = 7, the window size used was 2^(7 + 8) = 32,768.

In practice, since 32K is the maximum window size that can be used in CM = 8, this means the 4 high order bits are almost always `0111`, so the first byte is almost always `01111000` or `78`. This is the case in our data.

### The second byte
The first 5 bits of the second byte are known as FCHECK and are used as a checksum for the first two bytes. If you make a 16-bit integer using `256 * data[0] + data[1]`, then the resultant number should be divisible by 31 (i.e. `((data[0] << 8) | data[1]) % 31 == 0`. If it isn't, then something has gone wrong.

The 6th bit is a single-bit flag indicating whether a preset dictionary is in use (i.e. whether the flag is set is tested by `(data[1] & 0x10) >> 5 == 1`). As far as I can tell, this seems not to be implemented in the varieties of deflate used in pdf streams.

The remaining 2 highest order bits (`(data[1] & 0xc0) >> 6`) in the second byte denote the level of compression used, from 0 (fastest) to 3 (best compression). The default is `10` i.e. 2.

In our data, our second byte is `10011100`, meaning we are at default compression (`10`), no FDICT is set (`0`) and we have a valid header, since `(256 * 0x78) + 0x9c` is 30876, which is 996 * 31.

## The deflate block
After the two header bytes, we enter the first *deflate block*. Many streams will only have a single deflate block, but some will have multiple blocks. Each block is self-contained; it contains enough information to decode its own contents. A block starts with a 3 bit header. The lowest order bit of this header flags whether this is the final block in the stream, i.e. `data[2] & 0x01 == 1` answers "is this the final block?". If there is only a single block present, this flag will be set to `1`.

The next two bits (`data[2] & 0x06`) tell us the _type_ of compression. A value of 0 means no compression is used. A value of 1 is compression with a default compression dictionary, and a value of 2 is compression with a dynamic dictionary. A value of three is an error.

### Compression mode 0
There seems little point in having a "no compression" mode, and I'm not sure how often mode 0 is implemented in real life. However, it is straightforward to implement. First, we ignore the last 5 bits of the first byte in the block, then read the next 2 bytes as a 16-bit unsigned number. This will give us the length of the actual uncompressed data in the block. After this, we confirm the length bytes by having another two bytes that are simply the "one's complement" of the two length bytes. For example, if my uncompressed data was 11 bytes long, then my two length bytes would be `0x00 0x0b` and they would be followed by the one's complement of these two bytes (i.e. the same bytes but with all of their zero-bits as ones and their one-bits as zeros), which in this case would be `0xff 0xf4`. After this, we would read 11 bytes of data as they come to get our output for the block.

### Compression mode 1
The default dictionary (mode 1) is often used for very short messages with no repetition. If the compression mode is 1, you will need to have the default compression dictionary to look up the codes in the following stream. It is available online or in streams.cpp in this package. If the compression mode is 1, then the codes will come straight after the three-bit header, ready for reading and you should skip to the "Reading the compressed data" section below.

### Compression mode 2
Most streams will use their own code dictionary to optimally compress the contents of the message, and this is what compression mode 2 means. Constructing the custom dictionary is perhaps the most complex part of our task, so we will devote considerable space to it here.

In our stream, the third byte begins with `1`, indicating that this is indeed the last block, and the next two bits are `10`, indicating that this block has its own dictionary encoded in the next few bits.

## Building a dynamic dictionary
The compressed data itself is going to consist of variable-length chunks of bits which represent symbols. Each symbol is either going to represent an actual byte in our message, or is going to be a pointer to a chunk of actual bytes that have already been decoded and written to the output stream.

The variable-length chunks of bits - "codes", in the compressed data, have to be read by being looked up in a dictionary. In dynamic compression mode (mode 2), this dictionary itself is tailor-made to make the codes in the compressed data as short as possible.

Before we can read the stream therefore, we need to know what the codes are and what they represent. Hence we have to build a dictionary.

### Getting the dictionary size
After our three header bits in the third byte have declared dynamic compression, we know that our next 5 bits will tell us how many "literal codes" there are from which we need to build our dictionary. What does this mean? Well, the literal codes are the actual bit-sequences that we are going to be reading from the compressed data. You would think they should range from 0 to 255 to represent the actual bytes of our message. However, they actually range from 0 to 285. Why? Well, we need a stop code (256), and we also need a method of indicating that we are entering a pointer to an earlier decoded sequence. The numbers 257 - 285 tell us that we are entering a pointer to an earlier part of the output stream; what the actual numbers mean isn't important right now, but we will need to know that later on. 

In our data, byte 3 is `01100101`, and we have already read the 3 low order bits, so now we read `01100`, or 12. Since there are a minimum of 257 literal codes for any message, we always add our 12 to this number to get the number of literal codes we need in our dictionary: 269.

The next 5 bits in our sequence will indicate the number of _distance codes_ we will have in our dictionary. The distance codes need to be read only when we go into pointer mode, that is, we will only need these if we come across a number above 256 in our actual decoded data. Since there can be anywhere from 1 to 32 distance codes, we need to add 1 to the 0 - 31 indicated by these five bits. In our case, this means we will have `01011` (11) + 1 = 12 distance codes.

### Populating the dictionary
OK, so now we know how big our dictionary is going to be. It's going to have 269 literal codes and 12 distance codes. But how do we populate it? Unfortunately for us, the dictionary itself is compressed. You see, it turns out that you can completely specify the exact bit sequences representing a dictionary just by specifying the lengths of those sequences. How? By building a Huffman tree.

### Huffman trees
Huffman trees are a deceptively simple concept. They allow a group of distinct elements to be labelled in the most compact way possible. The problem a Huffman tree addresses is this: suppose I wanted to represent a known, fixed set of symbols in as few bits as possible.

Take the phrase "Hello_world!". That would normally be stored as a sequence of 12 bytes. However, what if I wanted to encode it as a minimal stream of bits, allowing each letter to be represented as a chunk of bits? Well, I could take advantage of the fact that I only have 9 different symbols to encode (since "l" appears 3 times and "o" appears twice). I need 9 different numbers to describe them, so I could do this in 4 bits (0000 - 1000). I could therefore store them as a stream of fixed 4-bit sequences; one symbol for each letter, and as long as I have a dictionary to interpret those 4-bit numbers, I can reconstitute the message.

A Huffman tree allows us to do even better than that by using fewer bits for the most frequently used symbols and more bits for the less frequently used symbols, but using fewer bits overall. We would construct it like this: first, take all the unique symbols we have in our message: `"H" "e" "l" "o" "_" "w" "r" "d" "!"`. Now, sort them by their frequency in the message. If their are ties, sort the ties by ascii value:
```
 symbol   l     o     !     H     _     d     e     r     w  
 count    3     2     1     1     1     1     1     1     1
 ascii   108   111   33    72    95    100   101   114   119
```
Now we start at the right and join the two values together. This creates a new node with summed values of the two joined nodes.

```
 symbol   l     o     !     H     _     d     e     r or w  
 count    3     2     1     1     1     1     1       2
```
Repeat this for the next lowest frequencies.

```
 symbol   l     o     !     H     _     d or e    r or w  
 count    3     2     1     1     1       2         2
 
 symbol   l     o     !     H or _     d or e    r or w  
 count    3     2     1       2          2         2
 
 symbol   l     o or !     H or _     d or e    r or w  
 count    3       3          2          2         2   
 
```
We are now looking to create a new node from adjacent members that will have the lowest combined count. If it's a tie, start on the right. 

```
 symbol   l     o or !     H or _   (d or e) or (r or w)  
 count    3       3          2               4
```


Now we can combine our remaining 4 nodes as 2 pairs:

```
 symbol   l  or (o or ! )    (H or _)  or ((d or e) or (r or w))
 count       6                         6

```
Now we can combine this to a single root node:

```
 symbol   (l  or (o or ! )) or ((H or _)  or ((d or e) or (r or w)))
 count                      12
 
```

The nested "or" clauses represent a tree structure, where each "or" represents a branching point:

```
    (l  or (o or ! )) or ((H or _)  or ((d or e) or (r or w)))
                      |
       -----------------------------------------------
       |                                             |
    l or (o or !)                          (H or _) or ((d or e) or (r or w))
       |                                             |
---------------------                      ---------------------
|                   |                      |                   |
l                 o or !                 H or _       (d or e) or (r or w)
                    |                      |                   |
               ------------              -----            ------------
               |          |              |   |            |          |
               o          !              H   _          d or e     r or w
                                                          |          |
                                                        ------     -----
                                                        |     |    |   |
                                                        d     e    r   w
                     
```

Now, what we can do is associate each branch with a binary digit, either zero or one, to represent left or right. Each symbol can be reached by following a unique sequence of branches, and therefore by a unique set of zeros and ones:

```
                                start
                                  |
       -----------------------------------------------
       |                                             |
       0                                             1
       |                                             |
---------------------                      ---------------------
|                   |                      |                   |
0                   1                      0                   1
|                   |                      |                   |
l              ------------              -----            ------------
               |          |              |   |            |          |
               0          1              0   1            0          1
               |          |              |   |            |          |
               o          !              H   -          -------    -----
                                                        |     |    |   |
                                                        0     1    0   1
                                                        |     |    |   |
                                                        d     e    r   w
```
This allows us to specify the unique digits for each letter, sorted by frequency then by ascii value:

| letter | binary code |
|--------|-------------|
|   l    |    00       |
|   o    |    010      |
|   !    |    011      |
|   H    |    100      |
|   _    |    101      |
|   d    |    1100     |
|   e    |    1101     |
|   r    |    1110     |
|   w    |    1111     |

Our message could therefore be transmitted as `100 1101 00 00 010 101 1111 010 1110 00 1100`. This is only 34 bits. Remember, using the standard ascii encoding I would need 12 bytes, or 96 bits, and using a minimal fixed length code I would need 4 bits per symbol, or 48 bits for my 12 symbols. However, I would also need the recipient to know what the codes meant, and this means I need a way of representing the Huffman tree itself.

Usefully, a recipient can recreate a Huffman tree if they only know the _length_ of the bits used for each code. They start their codes at zero, with the minimum number of bits required (e.g. if the smallest length is three bits, that would give a code `000`). For any other codes of the same length, they just add one, so if they have two other codes of length 3 to recover, they would label them `001` and `010`.

To make sure there is no ambiguity when it comes to actually reading the codes from a stream of bits, the recipient needs to make sure that none of these three-bit sequences appear at the start of the longer codes. To do this, they add one to the highest three-bit code and bit-shift it to the left by one place. Therefore, since the highest 3-bit code was `010`, they add one to make it `011` but then bit shift it to `0110`. Now if they have other codes of length 4, they can increment by one safely without having any clashes: `0111, 1000`. Again, if they want to continue to five bits they would do the same. Taking off from `1000` (which is 8 in binary) they would go to `(8 + 1) << 1`, which in binary is`10010`.

For example, I know my code lengths for the "Hello_world!" dictionary above are `[2, 3, 3, 3, 3, 4, 4, 4, 4]`. There is only one code length of two, so it must be `00`. Stepping up to three bits, we start at `(0 + 1) << 1` or `010`. I have another three codes of length three, so these must be `011`, `100` and `101`. Since `101` in binary is 5, I start my 4-bit sequence at `(5 + 1) << 1` which is `1100` in binary. The last three numbers will therefore be `1101`, `1110` and `1111`. This is exactly what we found with our Huffman codes.

A further difficulty is knowing which symbols these codes actually represent. I could represent this Huffman tree by having an array 128 bytes long, with one entry for each ascii character. I could then just put the number of bits that each symbol in "Hello_world!" is encoded with. All the symbols that don't appear in my dictionary would be "encoded" with zero bits, and ignored when building the Huffman tree. That is in fact the method used in Deflate, but of course it has an obvious drawback: you will have a big array of mostly zeros to include in your description of the Huffman lengths. We'll see how that problem is dealt with soon.

Anyway, that's what a Huffman tree is and how to reconstruct it using only bit lengths. Back to reconstructing our code dictionary...


## Building the code dictionary
Now we know we only need the bit lengths for our literal codes to reconstruct the actual codes. Surely all we need now is to read off the lengths of the entries in our table? Not so fast!

Since the number of bits needed for any one character will never exceed 15 (there are only 285 literal codes to represent), you _could_ specify the code dictionary with 4-bits per literal value, giving each literal a number between 0 and 15 as the length of bits required to represent it. However, this would give you an awful lot of wasted bytes full of zeros representing the number of bits required to specify those literal values that never occur in the message.

Here's what happens instead. We use _another_ compression method (run length encoding) to describe the bit-lengths of the final dictionary code table: the numbers 0 - 15 will represent an actual number of bits, and the numbers 16, 17, 18 will represent repeat sequences (mostly of zeros). I start building my dictionary's bit-length table at literal value 0, and if I come across the numbers 0-15, I insert them as the number of bits that are going to represent that literal in my final dictionary. I then move on to the next literal. If I come across a 16, 17, or 18, I will insert a number of repeats based on a set of rules for those 3 numbers. I keep going like this until my literal and distance codes all have an associated bit-length to describe them.

The rules for 16, 17 and 18 are:
- If it's a 16, read the next two bits from the stream to give a number between 0 and 4. Add 3 to this number. That's the number of repeats you are about to insert into the code length table. For example, if I come across a 16 and the next two bits are `01` then I will have 4 repeats to add. What do I repeat though? I repeat the last number that was written before I came across the 16. So if my codes read 0, 6, 2, 16 and the next two bits are `01`, then my entries will be 0, 6, 2, 2, 2, 2, 2.
- If it's a 17, read the next three bits from the stream to get a number between 0 and 7. Add 3 to this number to get the total number of repeats. The repeats are zeros.
- If it's an 18, read the next seven bits from the stream to get a number 0-127. Add 11 to this to get the number of repeats and insert that many zeros into the code length table.

Once I finish writing all the code lengths into my code length table, I will have a one-to-one mapping between number of bits and the literal code that it represents. From this, I can completely recreate the actual code dictionary by building its Huffman tree as described above.

So, all we need to do now is find out how the length codes 0 - 18 are represented in the code length table descriptor.


### Getting the code length codes
All this background, and we have only read 29 bits of our stream!

After the 5 bits that describe the length of the literal codes and the 5 bits that describe the distance codes come 4 bits telling us how many "code length codes" are about to be described. In our example, the four bits are `1100`, made from the lowest order bit of `data[4]` and the 3 highest order bits of `data[3]`. This is equal to 12, but since we only have 4 bits trying to describe a number of code lengths that may be as high as 19 (since there may be a code length for each of the numbers 0 through 18), we add 4 to this. In our case, this means we can expect 16 code lengths to follow.

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
This order is chosen because these are the most-to-least commonly used symbols in the literal-length encoding system. If we have less than 19 three-bit sequences, we assume the lengths at the end of the array are zeros. For our data then, we would have:

```
 order {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}
 value {6,  3,  4,  3, 0, 0, 0, 2, 0,  2, 0,  3, 0,  6, 0,  5, 0,  0, 0 }
```
Now rearranging the values according to the given order, we have:

```
code_lengths = {3, 0, 5, 6, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 3, 4};
```

This is sufficient to recreate the actual codes we are about to read to recreate the literal table. We do this by using these codes to build a Huffman tree as detailed above to get the symbols for our code length table descriptor. In our case, the given set of lengths gives us the following Huffman tree:

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
We have already read the first bit of byte 10. We want to read the remaining bits until we have a match. Now, here's the kicker: the Huffman codes are packed into the bytes backwards. That means that when we take a chunk of bits we need to interpret it backwards. If we read the three low order bits of `11100110` then we need to interpret this as `011` for the purposes of looking up the Huffman code. I know, I know, this is confusing, but that's just how it is.

I did promise that you can keep reading the bits in the same order all the way through. This is true; but to do so, you're going to need to reverse the bit chunks when you store the Huffman-generated codes for comparison. I think this is preferable to bit-reversing the codes when you are reading the code stream.

Let's bit reverse our Huffman codes:

| code | bits   |
|------|--------|
|   0  | 001    |
|   2  | 01111  |
|   3  | 011111 |
|   4  | 101    |
|   5  | 00     |
|   6  | 10     |
|  16  | 111111 |
|  17  | 011    |
|  18  | 0111   |

Remember, none of our codes are less than 2 bits long, so we start reading from the 7th bit in byte 10, using two bits, and ask whether we have a match. `11` isn't in our table, so how about three bits? `111` isn't in our table either. How about `0111`? Yes! That represents code 18. 

Codes 16, 17 and 18 are different from the rest. They don't represent actual code lengths, but repeat sequences of actual lengths. Code 18 means we read the next seven bits and add 11 to get the number of zeros we want to add to our literal array. We get `101` from the most significant 3 bits of byte 10, with the next 4 bits from the lower end of byte 11 `0010` being placed to the right to give `0010101` or 21. Adding 11 gives us 32, which means we want to start our literal length array with 32 zeros. 

Now we can read another code. `11`, `111`, `1111` and `11111` don't match, but `011111` does: it's code 3, so we place a 3 in our literal length array at literal_length[32]. This means that of all the ascii characters in our final message, the lowest will be 32, or 0x20, which is the space character. It will be represented by 3 bits when we come to decoding the actual compressed data.

Next comes `011` which is code 17. This means we have more zero repeats. The rules for code 17 are that we take the next three bits and add 3 to get the number of repeats. Here the next three bits are `011`, or 3, so we want to add 6 zeros to our array. 

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

Now we have all of our lengths, we can generate our actual literal and distance tables by reconstructing their Huffman trees. We need to ensure that we generate the Huffman trees seperately for the two dictionaries. The first 269 lengths will be our literal codes and the last 12 will be our distance codes.

Here are the final literal and distance codes from which we will construct our message. Please note *the bit codes have been reversed so we can continue to read the stream in the standard order throughout.*

---

### Literal Code Dictionary (Note bits are reversed to allow easy look-up)

literal code| bits
------------|----
 32 | 000
 39 | 00001
 44 | 10001
 46 | 001011
 73 | 01001
 97 | 0100
 99 | 11001
100 | 101011
101 | 1100
103 | 011011
104 | 00101
105 | 10101
107 | 111011
108 | 01101
109 | 11101
110 | 0010
111 | 1010
112 | 00011
114 | 000111
115 | 0110
116 | 1110
117 | 100111
121 | 010111
256 | 110111
257 | 001111
259 | 101111
260 | 011111
263 | 111111
268 | 10011



---

### Distance codes (with bits reversed to simplify lookup)

|Distance Code | Bits  |
|--------------|-------|
|    8         |   00  |
|    9         |   10  |
|    10        |   01  |
|    11        |   11  |
    
    
---

## Reading the compressed data
We now have our literal dictionary and distance dictionary. It is time to use them to read the actual data. At last!

If you have followed the example so far, we have read this table by consuming 179 bits, or 22 bytes + 3 bits. That takes us from the second bit of byte 10 to the 4th bit of byte 32. We now have 48 and 1/2 bytes left to squeeze in our compressed message:

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

Now we are in a position to read the message. Let's try. The sequence starts `01001`, which corresponds to 73. We write this to our output. Next comes `00001` which is 39, `11101` which is 109, and `000` which is a 32. This spells out "I'm " in ascii, so we're clearly on the right track. Let's keep going.

Bits read | code | Ascii
----------|------|-------
01001     | 73   |   I
00001     | 39   |   '
11101     | 109  |   m
000       | 32   |
0010      | 110  |   n
1010      | 111  |   o
1110      | 116  |   t
000       | 32   |
0100      | 97   |   a
000       | 32   |
00011     | 112  |   p
00101     | 104  |   h
1100      | 101  |   e 
0100      | 97   |   a
0110      | 115  |   s
0100      | 97   |   a
0010      | 110  |   n
1110      | 116  |   t
000       | 32   |
00011     | 112  |   p
01101     | 108  |   l
100111    | 117  |   u
11001     | 99   |   c
111011    | 107  |   k
1100      | 101  |   e
000111    | 114  |   r 
10001     | 44   |   ,
000       | 32   |
01001     | 73   |   I
001111    | 257  |   ...
    
Ah, but now what's happened? We have come across a non-ascii code, 257, starting at the 4th bit of byte 48. What does that mean?

## Length codes and distance codes
Almost any reasonably-lengthed message you might want to compress will have repeated sequences in it. Even a short paragraph with just a couple of sentences might have repeated sequences in it.

Take that last paragraph. It is made of two sentences and has 193 characters in it. However, the 31-character phrase _" have repeated sequences in it."_ appears twice. I could substantially shorten the message if I set up a system whereby I tell you the length of the repeat sequence and the distance back it starts. All I have to do is let you know I am entering "pointer mode", then give you the length of the repeated sequence (31 characters) and the distance back it starts (92 characters). I could then transmit the above paragraph as:

> Almost any reasonably-lengthed message you might want to compress will have repeated sequences in it. Even a short paragraph with just a couple of sentences [31, 92]

I have just chopped 31 bytes off the message length. Even assuming I need to add 3 bytes for the pointer (one to flag that we are entering pointer mode, one to give the length of the repeat, and one to say how far back the repeat is), I have saved 28 bytes.

A pointer method very much like this is used in deflate compression. The clever part is that you don't need a specific marker to say you are entering pointer mode. Instead, the literal codes between 257 and 285 are also length codes. When the decompressor comes across a length code, it not only knows that it is entering pointer mode, it knows the length of the repeat sequence. It also knows that the next code it needs is a distance code, so it searches the distance code table instead of the literal code table after it has the length.

Now, you may see a problem with this. There are only 29 length codes, but we might have a repeat that is much longer than that. Even the modest repeat in the above example wouldn't fit in if there were 29 length codes representing repeat lengths of 1 to 29. The way this problem is resolved is similar to the way the repeat lengths were calculated in our code length table using the numbers 16, 17 and 18: depending on the length code, you read some extra bits afterwards to give you the actual length.

The extra bits that you need to read are shown in the table below:

If you read a... | Read this number of extra bits | The length is
-----------------|--------------------------------|-------------
257              |       0                        |      3
258              |       0                        |      4
259              |       0                        |      5
260              |       0                        |      6
261              |       0                        |      7
262              |       0                        |      8
263              |       0                        |      9
264              |       0                        |      10
265              |       1                        |      11 + extra bits
266              |       1                        |      13 + extra bits
267              |       1                        |      15 + extra bits
268              |       1                        |      17 + extra bits
269              |       2                        |      19 + extra bits
270              |       2                        |      23 + extra bits
271              |       2                        |      27 + extra bits
272              |       2                        |      31 + extra bits
273              |       3                        |      35 + extra bits
274              |       3                        |      43 + extra bits
275              |       3                        |      51 + extra bits
276              |       3                        |      59 + extra bits
277              |       4                        |      67 + extra bits
278              |       4                        |      83 + extra bits
279              |       4                        |      99 + extra bits
280              |       4                        |      115 + extra bits
281              |       5                        |      131 + extra bits
282              |       5                        |      163 + extra bits
283              |       5                        |      195 + extra bits
284              |       5                        |      227 + extra bits
285              |       0                        |      258
      
If you tot up the number of extra bits required to encode each length, it turns out that the average number of extra bits we need for a random length between 3 and 257 will be 4. However, most repeats are short, and the actual number of extra bits required will average at considerably less than this. We can therefore enter pointer mode and get our repeat length for just the price of the literal code (257-285) plus a couple of extra bits. This will often amount to less than one byte.

Once we have our length, we know we are now looking for a distance to tell us how far back in the output stream to start copying. For this we have a separate lookup table - the distance code table. We have already created this table at the same time we created our literal code table.

If we look up the distance code, we find it gives us a number between 0 and 31. This is interpreted in a similar way to the length code. You read the code, then you may read some extra bits to get the exact distance back to start reading your repeat sequence.

The following table shows what to do when you get each of the distance codes:

Code   | Extra Bits | Represents distance of
-------|------------|-----------------------
0      |     0      |     1
1      |     0      |     2
2      |     0      |     3
3      |     0      |     4
4      |     1      |     5 + extra bits
5      |     1      |     7 + extra bits
6      |     2      |     9 + extra bits
7      |     2      |     13 + extra bits
8      |     3      |     17 + extra bits
9      |     3      |     25 + extra bits
10     |     4      |     33 + extra bits
11     |     4      |     49 + extra bits
12     |     5      |     65 + extra bits
13     |     5      |     97 + extra bits
14     |     6      |     129 + extra bits
15     |     6      |     193 + extra bits
16     |     7      |     257 + extra bits
17     |     7      |     385 + extra bits
18     |     8      |     513 + extra bits
19     |     8      |     769 + extra bits
20     |     9      |     1025 + extra bits
21     |     9      |     1537 + extra bits
22     |     10     |     2049 + extra bits
23     |     10     |     3073 + extra bits
24     |     11     |     4097 + extra bits
25     |     11     |     6145 + extra bits
26     |     12     |     8193 + extra bits
27     |     12     |     12289 + extra bits
28     |     13     |     16385 + extra bits
29     |     13     |     24577 + extra bits


## Back to writing our stream
We came across code number 257, which, from our table, means we only want three bytes of our earlier output copied to the end of the output string. We now read the distance code. This is `10`, which from looking up our distance table, gives us code number 9. What does code number 9 mean? Looking up what the various code numbers mean tells us that we should take the next three bits and add 25 to the number they represent. The next three bits are `011`, or 3, so we want to look back 28 places in our output and start copying three bytes. Our bytes were 39, 109, 32 or "'m ". Now we can go back to reading literals.

The next code we get is `10011`. This is another distance code, and it's our biggest one - 268. Looking this up in our length interpretation table, we see that this means we read one extra bit and add 17 to its value. The next bit is `1`, so our length is 18. How far back is it? The distance code we now read is `00`, or length code 8. Looking this up, we want to read the next three bits and add 17. Our next three bits are `111` which is 7, so our repetition starts 24 places earlier than the end of the output stream. This means we want to copy "a pheasant plucker" to the end of our sequence.

Our updated output is "I'm not a pheasant plucker, I'm a pheasant plucker". Weird. Next code is `00001`, or "'", then `0110` or "s", then `000` for " ", followed by `0110` for "s" again. Now we have consumed the first bit of byte 53.
