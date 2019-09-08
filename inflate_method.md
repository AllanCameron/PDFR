# Inflating a deflate stream in a pdf

We will imagine we have an array of bytes called `data` containing the deflate stream, as you might find it lifted directly from a pdf file.

Since the algorithm described below is implemented in C++, we will describe it using zero-indexed notation. This means the first byte will be denoted `data[0]`, the second byte `data[1]` etc.

Although bytes are read left to right, the bits within the bytes are read right to left, which can be confusing. This means if I talk about the first 4 bits of a byte, I mean the 4 low order bytes. For example, the first 4 bits of the first byte would be obtained by `data[0] & 0x0f` in C++ or `bitwAnd(data[1], 0x0f)` in R. The bits themselves are **not** reversed though, so if the first byte was made of the bits `00001010` then the first 4 bits should be interpreted as `1010` or 10, not as `0101` or 5.

## The first byte

The first byte declares the compression method flags, so is known as CMF. The first 4 bits declare the compression method (CM). The almost universal compression method is CM = 8, so the first byte should look like `xxxx1000`. 

The second four bits give the compression info. For CM = 8, you take these 4 bits and add 8. This gives you the log (to the base 2) of the LZ77 window size that was used to create the deflate stream. For example, if xxxx = 0111 = 7, the window size used was 2^(7 + 8) = 32,768.

In practice, since 32K is the maximum window size that can be used in CM = 8, this means the 4 high order bits are almost always `0111`, so the first byte is almost always `01111000` or `78`

## The second byte

The first 5 bits of the second byte are known as FCHECK and are used as a checksum for the first two bytes. If you make a 16-bit integer using `256 * data[0] + data[1]`, then the resultant number should be divisible by 31 (i.e. `(data[0] << 8) | data[1] % 31 == 0`). If it isn't, then something has gone wrong.

The 6th bit is a single-bit flag indicating whether a preset dictionary is in use (i.e. whether the flag is set is tested by `((data[1] & 0x10) >> 5) == 1`).

The remaining 2 highest order bits (`(data[1] & 0xc0) >> 6`) in the second byte denote the level of compression used, from 0 (fastest) to 3 (best compression). The default is 2.

