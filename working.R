library(R6)

# Converts strings in the form "101010" to the integer they represent. Useful
# function for reasoning about bits and binary representations. Accepts
# vetorized entry.

BinCharToInt <- function(char_binary)
{
  convert_one_string = function(string)
  {
    # Add leading zeroes
    while(nchar(string) < 32) string = paste0("0", string);

    # create a 32-length vector of zeroes and ones
    binary_vector = as.numeric(charToRaw(string)) - 48;

    # Sums the appropriate powers of two
    return(sum(binary_vector * 2^(31:0)));
   }

  # Use lapply() so we can vectorize the function. This creates a list
  # whereby we have one entry for each slot on the list. Each slot's entry
  # is then processed via a vector stage before being converted to an integer.
  # We return via unlist() to convert the list back into a vector.
  return(unlist(lapply(as.list(char_binary), convert_one_string)));
}

# This does the opposite to BinCharToInt(). It takes a second argument that
# specifies the number of bits you want to see, rather than chopping off
# leading zeroes. This IntToBinChar(7, 4) == "0111". Accepts vectorized entry.
IntToBinChar <- function(y, n = 8)
{
  if(max(y) > 2^64 | n > 64)
  {
    stop("IntToBinChar displays numbers of 64 bits or less")
  }

  convert_one_int = function(x)
  {
    if(x == 0)
    {
      return(paste0(rep("0", n), collapse = ""));
    }
    else
    {
      bit = 63;
      bit_string = "";
      while(bit >= 0)
      {
        if(x %/% 2^bit == 1)
        {
          bit_string = paste0(bit_string, "1");
          x = x %% 2^bit;
        }
        else
        {
          bit_string = paste0(bit_string, "0");
        }
        bit = bit - 1;
      }
      first_one = regexec("1", bit_string)[[1]][1];
      sig_dig = 65 - first_one;
      if(n < sig_dig)
      {
        n = sig_dig;
      }
      return(substr(bit_string, 65 - n, 65));
    }
  }

  return(unlist(lapply(as.list(y), convert_one_int)));
}


BitFlip = function(code, n_bits)
{
  code2 = 0;
  for(i in 1:n_bits)
  {
    code2 = bitwOr(bitwShiftL(code2, 1), bitwAnd(code, 1));
    code  = bitwShiftR(code, 1);
  }
  return(code2);
}


DeflateStream <- R6Class("DeflateStream", public = list(
  data = raw(),
  buffer = 0,
  size = 0,
  position = 0,
  unused_bits = 0,
  value_of_unused_bits = 0,
  EOF = FALSE,
  BufferLength = function() { return(length(self$buffer));},
  EnsureBuffer = function(new_length)
  {
    if(self$BufferLength() < new_length)
    {
      tmp = numeric(new_length);
      tmp[1:self$BufferLength()] <- self$buffer;
      self$buffer <- tmp;
    }
  },

  PrintBits = function(a = 1, b = length(self$data))
  {
    cat("LSB -> MSB\n ");
    mydata = self$data[a:b];
    for(i in seq_along(mydata))
    {
      byte = as.numeric(mydata[i]);
      for(j in 0:7)
      {
        cat(bitwShiftR(bitwAnd(byte,bitwShiftL(1, j)), j), sep = "")
      };
      cat("  ");
    }
  },

  MarkEOF = function()
  {
    self$EOF = TRUE;
  },

  GetByte = function()
  {
    if((self$position + 1) > self$size) return(256);
    self$position = self$position + 1;
    return(self$data[self$position]);
  },

  CheckHeader = function()
  {
    cmf = as.numeric(self$GetByte());
    flg = as.numeric(self$GetByte());
    if(bitwAnd(cmf, 0x0f) != 8)
    {
      stop("Invalid compression method.");
    }
    if((bitwShiftL(cmf, 8) + flg) %% 31 != 0)
    {
      stop("Invalid check flag");
    }
    if(bitwAnd(flg, 32) != 0)
    {
      stop("FDICT bit set in stream header");
    }
  },

  GetBits = function(n_bits)
  {
    unused_bits = self$unused_bits;
    value_of_unused_bits = self$value_of_unused_bits;

    while(unused_bits < n_bits)
    {
      result = as.numeric(self$GetByte());
      value_of_unused_bits = bitwOr(value_of_unused_bits,
                                    bitwShiftL(result, unused_bits));
      unused_bits = unused_bits + 8;
    }

    result = bitwAnd(value_of_unused_bits, (bitwShiftL(1, n_bits) - 1));
    self$value_of_unused_bits = bitwShiftR(value_of_unused_bits, n_bits);
    self$unused_bits = unused_bits - n_bits;
    return(result);
  },

  BitFlip = function(code)
  {
    code2 = 0;
    for(i in 1:8)
    {
      code2 = bitwOr(bitwShiftL(code2, 1), bitwAnd(code, 1));
      code  = bitwShiftR(code, 1);
    }
    return(code2);
  },

  reset = function()
  {
    self$position = 2;
  },

  initialize = function(my_data)
  {
    if(class(my_data) != "raw")
    {
      stop("Deflate stream requires raw vector.")
    }
    self$data = my_data;
    self$size = length(self$data);
    self$CheckHeader();
  },

  code_length_map = c(16, 17, 18, 0, 8,  7,  9, 6, 10, 5,
                      11, 4,  12, 3, 13, 2, 14, 1, 15),
));


Huffmanize <- function(lengths)
{
  all_codes = numeric(length(lengths));
  current_code = 0;
  for(i in 1:15)
  {
    need_codes = which(lengths == i);
    if(length(need_codes) == 0) next;
    for(j in seq_along(need_codes))
    {
      all_codes[need_codes[j]] = current_code;
      current_code = current_code + 1;
    }
    current_code = bitwShiftL(current_code, 1);
  }
  bit_representation = character();
  for(i in seq_along(lengths))
  {
    bit_representation[i] = IntToBinChar(all_codes[i], lengths[i]);
  }
  return(list(bit_length = lengths[lengths != 0],
              codes = all_codes[lengths != 0],
              bits = bit_representation[lengths != 0],
              represents = (0:(length(lengths) - 1))[lengths != 0]));
}


GetCodes = function(stream)
{
  codes = list();
  # read block header
  hdr = stream$GetBits(3);
  if(bitwAnd(hdr, 1) == 1) stream$MarkEOF();

  hdr = bitwShiftR(hdr, 1);

  if (hdr == 0) # uncompressed block
  {
    codes = list(bit_length = rep(8, 255),
                 codes = 0:255,
                 bits = IntToBinChar(0:255, 8),
                 represents = 0:255);
  }
  else if (hdr == 1) # compressed block, fixed codes
  {
    codes = list(bit_length = c(rep(8, 144), rep(9, 112), rep(7, 24), rep(8, 8)),
                 codes = c(48:191, 400:511, 0:23, 192:199),
                 bits = c(IntToBinChar(48:191, 8), IntToBinChar(400:511, 9),
                          IntToBinChar(0:23, 7), IntToBinChar(192:199, 8)),
                 represents = 0:287);
  }
  else if (hdr == 2) # compressed block, dynamic codes
  {
    numLitCodes = stream$GetBits(5) + 257;
    numDistCodes = stream$GetBits(5) + 1;
    numCodeLenCodes = stream$GetBits(4) + 4;

    # build the code lengths code table
    codeLenCodeLengths = numeric(length(stream$code_length_map));

    for (i in 1:numCodeLenCodes)
    {
      codeLenCodeLengths[i] = stream$GetBits(3);
    }

    codeLenTab = numeric(length(codeLenCodeLengths));
    codeLenTab[stream$code_length_map + 1] = codeLenCodeLengths;

    codeLenCodeTab = Huffmanize(codeLenTab);
    maxbits = max(codeLenCodeTab$bit_length);
    minbits = min(codeLenCodeTab$bit_length);

    remaining_buffer = 0;
    unread_bits = 0;
    code_lengths = numeric();
    distances = numeric();

    while(length(code_lengths) <= (numLitCodes + numDistCodes))
    {
      new_chunk_size = maxbits - unread_bits;
      chunkstart = bitwShiftL(remaining_buffer, new_chunk_size);
      chunk = BitFlip(stream$GetBits(new_chunk_size), new_chunk_size);
      chunk = chunk + chunkstart;
      found = FALSE;
      n = 0;
      if(length(code_lengths) > 0 && code_lengths[length(code_lengths)] > 15)
      {
        if(code_lengths[length(code_lengths)] == 16)
        {
          unread_bits = maxbits - 2;
          mask = bitwShiftL(3, unread_bits);
          remaining_buffer = bitwAnd(bitwNot(mask), chunk);
          chunk = bitwShiftR(bitwAnd(chunk, mask), unread_bits);
          num_repeats = BitFlip(chunk, 2) + 2;
          repeat_this = code_lengths[length(code_lengths) - 1];
          code_lengths[length(code_lengths) + 0:num_repeats] <- repeat_this;
        }
        if(code_lengths[length(code_lengths)] == 17)
        {
          mask = bitwShiftL(7, maxbits - 3);
          repeat_bits = bitwShiftR(bitwAnd(chunk, mask), maxbits - 3);
          num_repeats = BitFlip(repeat_bits, 3) + 2;
          code_lengths[length(code_lengths) + 0:num_repeats] <- 0;
          unread_bits = maxbits - 3;
          remaining_buffer = bitwAnd(chunk, bitwShiftL(1, unread_bits) - 1);
        }
        if(code_lengths[length(code_lengths)] == 18)
        {
          num_repeats = 0;

          if(maxbits == 7)
          {
            num_repeats = BitFlip(chunk, 7) + 10;
            unread_bits = 0;
            remaining_buffer = 0;
          }

          if(maxbits < 7)
          {
            extra_bits = 7 - maxbits;
            chunk = BitFlip(chunk, maxbits);
            chunk = chunk + bitwShiftL(stream$GetBits(extra_bits), extra_bits);
            num_repeats = chunk + 10;
            unread_bits = 0;
            remaining_buffer = 0;
          }

          if(maxbits > 7)
          {
            unread_bits = maxbits - 7;
            mask = bitwShiftL(127, unread_bits);
            remaining_buffer = bitwAnd(bitwNot(mask), chunk);
            chunk = bitwAnd(chunk, mask);
            chunk = bitwShiftR(chunk, excess_bits);
            chunk = BitFlip(chunk, 7);
            num_repeats = chunk + 10;
          }
          code_lengths[length(code_lengths) + 0:num_repeats] <- 0;
        }
        next;
      }
      while(!found)
      {
        mask = bitwShiftL(1, maxbits) - bitwShiftL(1, maxbits - minbits - n);
        checknum = bitwShiftR(bitwAnd(chunk, mask), maxbits - minbits - n);
        readbits = which(codeLenCodeTab$codes == checknum &
                         codeLenCodeTab$bit_length == minbits + n);
        if(length(readbits) == 1)
        {
          code_lengths[length(code_lengths) + 1] = codeLenCodeTab$represents[readbits];
          found = TRUE;
          mask = bitwShiftL(1, maxbits - minbits - n) - 1;
          remaining_buffer = bitwAnd(chunk, mask);
          unread_bits = maxbits - minbits - n;
        } else {
          n = n + 1;
          if(n + minbits > maxbits)
          {
            stop("Couldn't read code")
          }
        }
      }
    }
    codes = Huffmanize(code_lengths);
  }
  return(codes);
};

stream_data = readBin("C:/Users/cameral931/R/PDFR/inst/extdata/gunzip.c.gz", "raw", 10e6);
stream <- DeflateStream$new(c(as.raw(c(120, 156)), stream_data[-(1:19)]));








