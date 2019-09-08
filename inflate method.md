# Inflating a deflate stream in a pdf

## The first byte

The first byte declares the compression method. The first 4 bits (which, remember, are the low order bits) declare the compression method (CM). The almost universal compression method is CM = 8, so the first byte should look like `xxxx1000`. The second four bits (the four x to the left of the 1000) give the compression info. For CM = 8, you take these 4 bits and add 8. This gives you the log (to the base 2) of the LZ77 window size that was used to create the deflate stream. For example, if xxxx = 0111 = 7, the window size used was 2^(7 + 8) = 32,768.

In practice, since 32K is the maximum window size that can be used in CM = 8, this means the 4 high order bits are almost always `0111`, so the first byte is almost always `01111000` or `78`
