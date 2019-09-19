library(R6)
library(PDFR)

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

Huffmanize <- function(lengths)
{
  all_codes = numeric(length(lengths));

  current_code = 0;

  for(i in 1:max(lengths))
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

Huffmanise <- function(lengths)
{
  all_codes = numeric(length(lengths));

  current_code = 0;

  for(i in 1:max(lengths))
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

  return(bitwShiftL(lengths[i], 16) + all_codes[i])
}

DeflateStream <- R6Class("DeflateStream", public = list(
  data = raw(),
  output = numeric(),
  size = 0,
  position = 0,
  unused_bits = 0,
  unused_value = 0,
  EOF = FALSE,
  literal_codes = list(bit_length = rep(8, 255),
                       codes = 0:255,
                       represents = 0:255),
  dist_codes = list(bit_length = rep(5, 32),
                    codes = 0:31,
                    represents = 0:31),

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
    unused_value = self$unused_value;

    while(unused_bits < n_bits)
    {
      result = as.numeric(self$GetByte());
      unused_value = bitwOr(unused_value,
                                    bitwShiftL(result, unused_bits));
      unused_bits = unused_bits + 8;
    }

    result = bitwAnd(unused_value, (bitwShiftL(1, n_bits) - 1));
    self$unused_value = bitwShiftR(unused_value, n_bits);
    self$unused_bits = unused_bits - n_bits;
    return(result);
  },

  ReadCodes = function()
  {
    EOF = self$EOF;
    self$EOF = FALSE;
    while(self$EOF == FALSE)
    {
      code = self$GetCode(self$literal_codes);
      if(code < 256)
      {
        self$output[length(self$output) + 1] = code;
      }
      if(code > 256) self$HandlePointer(code);
      if(code == 256) self$EOF = TRUE;
    }
    self$EOF = EOF;
  },

  ReadBlock = function()
  {
    three_bit_header = self$GetBits(3);
    if(bitwAnd(three_bit_header, 1) == 1) self$EOF = TRUE;
    three_bit_header = bitwShiftR(three_bit_header, 1);

    if (three_bit_header == 1) # compressed block, fixed codes
    {
      self$literal_codes = list(bit_length = c(rep(8, 144),
                                               rep(9, 112),
                                               rep(7, 24),
                                               rep(8, 8)),
                                codes      = c(48:191, 400:511, 0:23, 192:199),
                                represents = 0:287);
    }
    if (three_bit_header == 2) self$BuildDynamicCodeTable();
    self$ReadCodes();
  },

  BuildDynamicCodeTable = function()
  {
    hlit = self$GetBits(5) + 257;
    hdist = self$GetBits(5) + 1;
    numcodes = hlit + hdist;
    numCodeLenCodes = self$GetBits(4) + 4;

    # build the code lengths code table
    codeLenCodeLengths = numeric(length(self$code_length_map));

    for (i in 1:numCodeLenCodes)
    {
      codeLenCodeLengths[i] = self$GetBits(3);
    }

    codeLenTab = numeric(length(codeLenCodeLengths));
    codeLenTab[self$code_length_map + 1] = codeLenCodeLengths;

    code_length_table = Huffmanize(codeLenTab);
    print(code_length_table)
    maxbits = max(code_length_table$bit_length);
    minbits = min(code_length_table$bit_length);
    code_lengths = numeric();

    while(length(code_lengths) < numcodes)
    {
      found = FALSE;
      read_bits = minbits;
      read_value = BitFlip(self$GetBits(read_bits), read_bits);

      while(!found)
      {
        matches = which(code_length_table$bit_length == read_bits &
                        code_length_table$codes == read_value);
        if(length(matches) == 1)
        {
          code = code_length_table$represents[matches];
          cat("read code", code, "\n")
          if(code > 15)
          {
            repeat_this = 0;
            num_bits = 3 * (code %/% 18) + code - 14;
            cat("Reading", num_bits, "bits at byte", self$position, ":", self$unused_bits);
            num_repeats = self$GetBits(num_bits);
            cat(" with value", num_repeats, "\n");
            num_repeats = num_repeats + 3 + (8 * (code %/% 18));
            cat("I make that", num_repeats, "repeats\n")
            if (code == 16) repeat_this = code_lengths[length(code_lengths)];
            code_lengths = c(code_lengths, rep(repeat_this, num_repeats));
          }
          else
          {
            code_lengths[length(code_lengths) + 1] = code;
          }
          found = TRUE;
        }
        else
        {
          read_bits = read_bits + 1;
          read_value = bitwShiftL(read_value, 1) + self$GetBits(1);
          if(read_bits > maxbits) stop("Couldn't read code");
        }
      }
    }
    self$literal_codes = Huffmanize(code_lengths[1:hlit]);
    self$dist_codes = Huffmanize(code_lengths[hlit + 1:hdist]);
  },

  GetCode = function(huffman_table)
  {
    minbits = min(huffman_table$bit_length);
    maxbits = max(huffman_table$bit_length);
    chunk = BitFlip(self$GetBits(minbits), minbits);
    found = FALSE;
    current_bits = minbits;
    while(found == FALSE & current_bits <= maxbits)
    {
      match = which(huffman_table$bit_length == current_bits &
                    huffman_table$codes == chunk);
      if(length(match) > 0)
      {
        return(huffman_table$represents[match]);
      }
      else
      {
        current_bits = current_bits + 1;
        chunk = bitwShiftL(chunk, 1) + self$GetBits(1);
      }
    }
    stop("No code match found");
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
    #while(self$EOF == FALSE) self$ReadBlock();
  },


  HandlePointer = function(code)
  {
    length_value = 0;
    distance_value = 0;
    extrabits = 0;
    if(code < 265)
    {
      length_value = code - 254;
    }
    else
    {
      if(code == 285)
      {
        length_value = 258;
      }
      else
      {
        extrabits = (code - 261) %/% 4;
        read_value = self$GetBits(extrabits);
        length_value = read_value + self$length_table[code - 264];
      }
    }
    distance_code = self$GetCode(self$dist_codes);
    if(distance_code < 4)
    {
      distance_value = distance_code + 1;
    }
    else
    {
      extrabits = (distance_code %/% 2) - 1;
      read_value = self$GetBits(extrabits);
      distance_value = read_value + self$distance_table[distance_code - 3];
    }
    if(distance_value < length_value)
    {
      while(length_value > 0)
      {
        self$output[length(self$output) + 1] =
          self$output[length(self$output) - distance_value + 1];
        length_value = length_value - 1;
      }
    }
    else
    {
      piece = self$output[length(self$output) - distance_value + 1:length_value];
      self$output[length(self$output) + 1:length_value] <- piece;
    }
  },

  Output = function()
  {
    return(rawToChar(as.raw(self$output)));
  },

  code_length_map = c(16, 17, 18, 0, 8,  7,  9, 6, 10, 5,
                      11, 4,  12, 3, 13, 2, 14, 1, 15),
  length_table = c(0, cumsum(2^((265:284 - 261) %/% 4)))[-21] + 11,
  distance_table = c(0, cumsum(2^((4:29 %/% 2) - 1)))[-27] + 5
));

sentence <- paste0("I'm not a pheasant plucker, I'm a pheasant plucker's son, ",
                   "and I'm only plucking pheasants til the pheasant plucker comes.")
stream_data <- memCompress(charToRaw(sentence), "gzip")
self <- DeflateStream$new(stream_data);



c(459136, 0, 459137, 459138, 459139, 459140, 459141, 459142, 459143, 459144,
  459145, 459146, 459147, 459148, 1, 2, 459149, 459150, 459151) -> a

c(196608,
0,
0,
393246,
262147,
196612,
196610,
196614,
196609,
262155,
262151,
196613,
0,
0,
0,
0,
393278,
393217,
393249)

