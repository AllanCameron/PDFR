library(R6)

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
    cat("Valid DeflateStream header.")
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

  GetCode = function(my_table)
  {
    codes = my_table[[1]];
    maxLen = my_table[[2]];
    unused_bits = self$unused_bits;
    value_of_unused_bits = self$value_of_unused_bits;

    b = 0;
    while (unused_bits < maxLen)
    {
      b = as.numeric(self$GetByte());
      if (b > 255) break;
      value_of_unused_bits = bitwOr(value_of_unused_bits, bitwShiftL(b, unused_bits));
      unused_bits = unused_bits + 8;
    }
    code = codes[bitwAnd(value_of_unused_bits, (bitwShiftL(1, maxLen) - 1))];
    codeLen = bitwShiftR(code, 16);
    codeVal = bitwAnd(code, 65535);
    if (codeLen < 1 | unused_bits < codeLen) stop("Bad encoding in flate stream");
    self$value_of_unused_bits = bitwShiftR(value_of_unused_bits, codeLen);
    self$unused_bits = (unused_bits - codeLen);
    return(codeVal);
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
  distDecode = c(1:2, cumsum(floor(2^((0:27 - 1) %/% 2))) +
                   floor(0:27/2) * 2^16 + 3),
  lengthDecode = c(0x00003, 0x00004, 0x00005, 0x00006, 0x00007, 0x00008,
                   0x00009, 0x0000a, 0x1000b, 0x1000d, 0x1000f, 0x10011,
                   0x20013, 0x20017, 0x2001b, 0x2001f, 0x30023, 0x3002b,
                   0x30033, 0x3003b, 0x40043, 0x40053, 0x40063, 0x40073,
                   0x50083, 0x500a3, 0x500c3, 0x500e3, 0x00102, 0x00102,
                   0x00102),
  fixedDistCodeTab = list(c( 0x50000, 0x50010, 0x50008, 0x50018, 0x50004,
                             0x50014, 0x5000c, 0x5001c, 0x50002, 0x50012,
                             0x5000a, 0x5001a, 0x50006, 0x50016, 0x5000e,
                             0x00000, 0x50001, 0x50011, 0x50009, 0x50019,
                             0x50005, 0x50015, 0x5000d, 0x5001d, 0x50003,
                             0x50013, 0x5000b, 0x5001b, 0x50007, 0x50017,
                             0x5000f, 0x00000), 5),
  fixedLitCodeTab = list(c(
    0x70100, 0x80050, 0x80010, 0x80118, 0x70110, 0x80070, 0x80030, 0x900c0,
    0x70108, 0x80060, 0x80020, 0x900a0, 0x80000, 0x80080, 0x80040, 0x900e0,
    0x70104, 0x80058, 0x80018, 0x90090, 0x70114, 0x80078, 0x80038, 0x900d0,
    0x7010c, 0x80068, 0x80028, 0x900b0, 0x80008, 0x80088, 0x80048, 0x900f0,
    0x70102, 0x80054, 0x80014, 0x8011c, 0x70112, 0x80074, 0x80034, 0x900c8,
    0x7010a, 0x80064, 0x80024, 0x900a8, 0x80004, 0x80084, 0x80044, 0x900e8,
    0x70106, 0x8005c, 0x8001c, 0x90098, 0x70116, 0x8007c, 0x8003c, 0x900d8,
    0x7010e, 0x8006c, 0x8002c, 0x900b8, 0x8000c, 0x8008c, 0x8004c, 0x900f8,
    0x70101, 0x80052, 0x80012, 0x8011a, 0x70111, 0x80072, 0x80032, 0x900c4,
    0x70109, 0x80062, 0x80022, 0x900a4, 0x80002, 0x80082, 0x80042, 0x900e4,
    0x70105, 0x8005a, 0x8001a, 0x90094, 0x70115, 0x8007a, 0x8003a, 0x900d4,
    0x7010d, 0x8006a, 0x8002a, 0x900b4, 0x8000a, 0x8008a, 0x8004a, 0x900f4,
    0x70103, 0x80056, 0x80016, 0x8011e, 0x70113, 0x80076, 0x80036, 0x900cc,
    0x7010b, 0x80066, 0x80026, 0x900ac, 0x80006, 0x80086, 0x80046, 0x900ec,
    0x70107, 0x8005e, 0x8001e, 0x9009c, 0x70117, 0x8007e, 0x8003e, 0x900dc,
    0x7010f, 0x8006e, 0x8002e, 0x900bc, 0x8000e, 0x8008e, 0x8004e, 0x900fc,
    0x70100, 0x80051, 0x80011, 0x80119, 0x70110, 0x80071, 0x80031, 0x900c2,
    0x70108, 0x80061, 0x80021, 0x900a2, 0x80001, 0x80081, 0x80041, 0x900e2,
    0x70104, 0x80059, 0x80019, 0x90092, 0x70114, 0x80079, 0x80039, 0x900d2,
    0x7010c, 0x80069, 0x80029, 0x900b2, 0x80009, 0x80089, 0x80049, 0x900f2,
    0x70102, 0x80055, 0x80015, 0x8011d, 0x70112, 0x80075, 0x80035, 0x900ca,
    0x7010a, 0x80065, 0x80025, 0x900aa, 0x80005, 0x80085, 0x80045, 0x900ea,
    0x70106, 0x8005d, 0x8001d, 0x9009a, 0x70116, 0x8007d, 0x8003d, 0x900da,
    0x7010e, 0x8006d, 0x8002d, 0x900ba, 0x8000d, 0x8008d, 0x8004d, 0x900fa,
    0x70101, 0x80053, 0x80013, 0x8011b, 0x70111, 0x80073, 0x80033, 0x900c6,
    0x70109, 0x80063, 0x80023, 0x900a6, 0x80003, 0x80083, 0x80043, 0x900e6,
    0x70105, 0x8005b, 0x8001b, 0x90096, 0x70115, 0x8007b, 0x8003b, 0x900d6,
    0x7010d, 0x8006b, 0x8002b, 0x900b6, 0x8000b, 0x8008b, 0x8004b, 0x900f6,
    0x70103, 0x80057, 0x80017, 0x8011f, 0x70113, 0x80077, 0x80037, 0x900ce,
    0x7010b, 0x80067, 0x80027, 0x900ae, 0x80007, 0x80087, 0x80047, 0x900ee,
    0x70107, 0x8005f, 0x8001f, 0x9009e, 0x70117, 0x8007f, 0x8003f, 0x900de,
    0x7010f, 0x8006f, 0x8002f, 0x900be, 0x8000f, 0x8008f, 0x8004f, 0x900fe,
    0x70100, 0x80050, 0x80010, 0x80118, 0x70110, 0x80070, 0x80030, 0x900c1,
    0x70108, 0x80060, 0x80020, 0x900a1, 0x80000, 0x80080, 0x80040, 0x900e1,
    0x70104, 0x80058, 0x80018, 0x90091, 0x70114, 0x80078, 0x80038, 0x900d1,
    0x7010c, 0x80068, 0x80028, 0x900b1, 0x80008, 0x80088, 0x80048, 0x900f1,
    0x70102, 0x80054, 0x80014, 0x8011c, 0x70112, 0x80074, 0x80034, 0x900c9,
    0x7010a, 0x80064, 0x80024, 0x900a9, 0x80004, 0x80084, 0x80044, 0x900e9,
    0x70106, 0x8005c, 0x8001c, 0x90099, 0x70116, 0x8007c, 0x8003c, 0x900d9,
    0x7010e, 0x8006c, 0x8002c, 0x900b9, 0x8000c, 0x8008c, 0x8004c, 0x900f9,
    0x70101, 0x80052, 0x80012, 0x8011a, 0x70111, 0x80072, 0x80032, 0x900c5,
    0x70109, 0x80062, 0x80022, 0x900a5, 0x80002, 0x80082, 0x80042, 0x900e5,
    0x70105, 0x8005a, 0x8001a, 0x90095, 0x70115, 0x8007a, 0x8003a, 0x900d5,
    0x7010d, 0x8006a, 0x8002a, 0x900b5, 0x8000a, 0x8008a, 0x8004a, 0x900f5,
    0x70103, 0x80056, 0x80016, 0x8011e, 0x70113, 0x80076, 0x80036, 0x900cd,
    0x7010b, 0x80066, 0x80026, 0x900ad, 0x80006, 0x80086, 0x80046, 0x900ed,
    0x70107, 0x8005e, 0x8001e, 0x9009d, 0x70117, 0x8007e, 0x8003e, 0x900dd,
    0x7010f, 0x8006e, 0x8002e, 0x900bd, 0x8000e, 0x8008e, 0x8004e, 0x900fd,
    0x70100, 0x80051, 0x80011, 0x80119, 0x70110, 0x80071, 0x80031, 0x900c3,
    0x70108, 0x80061, 0x80021, 0x900a3, 0x80001, 0x80081, 0x80041, 0x900e3,
    0x70104, 0x80059, 0x80019, 0x90093, 0x70114, 0x80079, 0x80039, 0x900d3,
    0x7010c, 0x80069, 0x80029, 0x900b3, 0x80009, 0x80089, 0x80049, 0x900f3,
    0x70102, 0x80055, 0x80015, 0x8011d, 0x70112, 0x80075, 0x80035, 0x900cb,
    0x7010a, 0x80065, 0x80025, 0x900ab, 0x80005, 0x80085, 0x80045, 0x900eb,
    0x70106, 0x8005d, 0x8001d, 0x9009b, 0x70116, 0x8007d, 0x8003d, 0x900db,
    0x7010e, 0x8006d, 0x8002d, 0x900bb, 0x8000d, 0x8008d, 0x8004d, 0x900fb,
    0x70101, 0x80053, 0x80013, 0x8011b, 0x70111, 0x80073, 0x80033, 0x900c7,
    0x70109, 0x80063, 0x80023, 0x900a7, 0x80003, 0x80083, 0x80043, 0x900e7,
    0x70105, 0x8005b, 0x8001b, 0x90097, 0x70115, 0x8007b, 0x8003b, 0x900d7,
    0x7010d, 0x8006b, 0x8002b, 0x900b7, 0x8000b, 0x8008b, 0x8004b, 0x900f7,
    0x70103, 0x80057, 0x80017, 0x8011f, 0x70113, 0x80077, 0x80037, 0x900cf,
    0x7010b, 0x80067, 0x80027, 0x900af, 0x80007, 0x80087, 0x80047, 0x900ef,
    0x70107, 0x8005f, 0x8001f, 0x9009f, 0x70117, 0x8007f, 0x8003f, 0x900df,
    0x7010f, 0x8006f, 0x8002f, 0x900bf, 0x8000f, 0x8008f, 0x8004f, 0x900ff
  ), 9)
));


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

GenerateHuffmanTable = function(lengths)
{
  n = length(lengths);
  maxLen = max(lengths);

  # build the table
  size = bitwShiftL(1, maxLen);
  codes = numeric(size);
  code = 0;
  skip = 2;
  for (len in 1:maxLen)
  {
    for (val in 1:n)
    {
      if (lengths[val] == len)
      {
        # bit-reverse the code
        code2 = 0;
        t = code;
        for (i in 0:(len - 1))
        {
          code2 = bitwOr(bitwShiftL(code2, 1), bitwAnd(t, 1));
          t = bitwShiftR(t, 1);
        }

        # fill the table entries
        for (i in seq(code2 + 1, size + 1, skip))
        {
          codes[i] = bitwOr(bitwShiftL(len, 16), (val - 1));
        }
        code = code + 1;
      }
    }
    code = bitwShiftL(code, 1);
    skip = bitwShiftL(skip, 1);
  }

  return(list(codes[1:size], maxLen));
};

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
    bit_representation[i] = intToBinChar(all_codes[i], lengths[i]);
  }
  return(list(bit_length = lengths[lengths != 0],
              codes = all_codes[lengths != 0],
              bits = bit_representation[lengths != 0],
              represents = (0:length(lengths))[lengths != 0]));
}

ReadBlock = function(stream)
{
  buffer = 0;
  len = 0;

  # read block header
  hdr = stream$GetBits(3);
  if(bitwAnd(hdr, 1) == 1)
  {
    stream$MarkEOF();
  }
  hdr = bitwShiftR(hdr, 1);

  if (hdr == 0) # uncompressed block
  {
    b = as.numeric(stream$GetByte());
    if(b > 255) stop('Bad block header in stream');

    blockLen = b;

    b = as.numeric(stream$GetByte());
    if(b > 255) stop('Bad block header in stream');

    blockLen = bitwOr(blockLen, bitwShiftL(b, 8));

    b = as.numeric(stream$GetByte());
    if(b > 255) stop('Bad block header in stream');

    check = b;

    b = as.numeric(stream$GetByte());
    if(b > 255) stop('Bad block header in stream');

    check = bitwOr(check, bitwShiftL(b, 8));

    if (check != bitwAnd(bitwNot(blockLen), 0xffff) &&
        (blockLen != 0 | check != 0))
    {
      # Ignoring error for bad "empty" block (see issue 1277)
      stop('Bad uncompressed block length in flate stream');
    }

    stream$value_of_unused_bits = 0;
    stream$unused_bits = 0;

    bufferLength = stream$BufferLength();
    buffer = stream$EnsureBuffer(bufferLength + blockLen);
    end = bufferLength + blockLen;
    stream$bufferLength = end;
    if (blockLen == 0)
    {
      if (stream$GetByte() > 255)
      {
        return(raw());
      }
      stream$position = stream$position - 1;
    }
    else
    {
      for (n in bufferLength:end)
      {
        b = as.numeric(stream$GetByte());
        if(b > 255) return(buffer);
        buffer[n] = b;
      }
    }
    return(as.raw(buffer));
  }

  litCodeTable = list();
  distCodeTable = list();
  if (hdr == 1) # compressed block, fixed codes
  {
    litCodeTable = stream$fixedLitCodeTab;
    distCodeTable = stream$fixedDistCodeTab;
  } else if (hdr == 2) # compressed block, dynamic codes
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
    for(i in 1:length(codeLenTab))
    {
      codeLenTab[stream$code_length_map[i] + 1] = codeLenCodeLengths[i];
    }

    codeLenCodeTab = Huffmanize(codeLenTab);
    maxbits = max(codeLenCodeTab$bit_length);
    minbits = min(codeLenCodeTab$bit_length);

    remaining_buffer = 0;
    unread_bits = 0;
    literals = numeric();
    distances = numeric();

    for(i in 1:numLitCodes)
    {
      new_chunk_size = maxbits - unread_bits;
      chunkstart = bitwShiftL(remaining_buffer, new_chunk_size);
      chunk = BitFlip(stream$GetBits(new_chunk_size), new_chunk_size);
      chunk = chunk + chunkstart;
      found = FALSE;
      n = 0;
      cat("chunk is", intToBinChar(chunk, maxbits), "\n");
      if(i > 1 && literals[length(literals)] > 15)
      {
        if(literals[length(literals)] == 17)
        {
          mask = bitwShiftL(7, maxbits - 3);
          num_repeats = bitwShiftR(bitwAnd(chunk, mask), maxbits - 3) + 2;
          literals[length(literals) + 0:num_repeats] <- 0;
          unread_bits = maxbits - 3;
          remaining_buffer = bitwAnd(chunk, bitwShiftL(1, unread_bits) - 1);
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
          literals[length(literals) + 1] = codeLenCodeTab$represents[readbits];
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

    # build the literal and distance code tables
    len = 0;
    i = 1;
    codes = numLitCodes + numDistCodes;
    codeLengths = numeric(codes);

    bitsLength = 2;
    bitsOffset = 0;
    what = 0;
    while (i <= codes)
    {
      code = stream$GetCode(codeLenCodeTab);
      if (code == 16)
      {
        bitsLength = 2; bitsOffset = 3; what = len;
      }
      else if (code == 17)
      {
        bitsLength = 3; bitsOffset = 3; what = 0; len = 0;
      }
      else if (code == 18)
      {
        bitsLength = 7; bitsOffset = 11; what = 0; len = 0;
      }
      else
      {
        len = code;
        codeLengths[i] = len;
        i = i + 1;
        next;
      }

      repeatLength = stream$GetBits(bitsLength) + bitsOffset;
      while (repeatLength > 0)
      {
        codeLengths[i] = what;
        i = i + 1;
        repeatLength = repeatLength - 1;
      }
    }

    litCodeTable = GenerateHuffmanTable(codeLengths[1:numLitCodes]);
    distCodeTable = GenerateHuffmanTable(codeLengths[(numLitCodes+1):codes]);
  } else {
    stop('Unknown block type in flate stream');
  }

  buffer = stream$buffer;
  limit = length(buffer);
  pos = stream$BufferLength();
  while (TRUE)
  {
    if(stream$position == stream$size) return(buffer);
    code1 = stream$GetCode(litCodeTable);
    if (code1 < 256)
    {
      if (pos + 1 >= limit)
      {
        stream$buffer = buffer;
        stream$EnsureBuffer(pos + 1);
        buffer = stream$buffer;
        limit = length(buffer);
      }
      buffer[pos] = code1;
      pos = pos + 1;
      next;
    }
    if (code1 == 256)
    {
      stream$EnsureBuffer(pos);
      return(buffer);
    }
    code1 = code1 - 257;
    code1 = stream$lengthDecode[code1];
    code2 = bitwShiftR(code1, 16);
    if (code2 > 0)
    {
      code2 = stream$GetBits(code2);
    }
    len = bitwAnd(code1, 0xffff) + code2;
    code1 = stream$GetCode(distCodeTable);
    code1 = stream$distDecode[code1];
    code2 = bitwShiftR(code1, 16);
    if (code2 > 0)
    {
      code2 = stream$GetBits(code2);
    }
    dist = bitwAnd(code1, 0xffff) + code2;
    if (pos + len >= limit)
    {
      stream$buffer = buffer;
      stream$EnsureBuffer(pos + len);
      buffer = stream$buffer;
      limit = length(buffer);
    }
    for (k in 1:len)
    {
      buffer[pos] = buffer[pos - dist];
      pos = pos + 1;
    }
  }
  return(buffer);
};






lexical_sort <- function(character_table)
{
  character_table = sort(character_table);
  frequency_levels = unique(character_table);
  for(i in 1:length(frequency_levels))
  {
    this_group = which(character_table == frequency_levels[i]);
    labels_ = names(character_table)[this_group];
    first_chars = paste(substr(labels_, 1, 1), collapse = "");
    group_order = order(as.numeric(charToRaw(first_chars)));
    labels_ = labels_[group_order];
    names(character_table)[this_group] = rev(labels_);
  }
  return(character_table)
}

charToBin <- function(char_binary)
{
  return(unlist(lapply(as.list(char_binary),
                       function(x)
                       {
                         while(nchar(x) < 33) x = paste0("0", x);
                         return(sum((as.numeric(charToRaw(x)) - 48) * 2^(32:0)));
                       })));
}

intToBinChar <- function(y, n = 8)
{
  if(max(y) > 2^64 | n > 64)
  {
    stop("intToBinChar displays numbers of 64 bits or less")
  }
  return(unlist(lapply(as.list(y),
                       function(x)
                       {
                         if(x == 0)
                         {
                           return(paste0(rep("0", n), collapse = ""));
                         } else {
                           bit = 63;
                           bit_string = "";
                           while(bit >= 0)
                           {
                             if(x %/% 2^bit == 1)
                             {
                               bit_string = paste0(bit_string, "1");
                               x = x %% 2^bit;
                             } else {
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
                         }})));
}

combine_lowest_two <- function(character_table)
{
  if(length(character_table) > 1)
  {
    freq_sum = character_table[1] + character_table[2];
    new_label = paste0(names(character_table)[1:2], collapse = "");
    character_table = character_table[2:length(character_table)];
    character_table[1] = freq_sum;
    names(character_table)[1] = new_label;
  }
  return(lexical_sort(character_table));
}





stream_data = readBin( path.expand("~/PDFR/inst/extdata/gunzip.c.gz"), "raw", 10e6);
stream <- DeflateStream$new(c(as.raw(c(120, 156)), stream_data[-(1:19)]));








