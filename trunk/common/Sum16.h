#pragma once

// -----------------------------------------------------------------------------------------------
class Sum16 {
  public: unsigned __int16 generate(const void *_ptr, unsigned long size)
  {
    register unsigned __int16 sum = 0;
    register unsigned __int16 *ptr = (unsigned __int16 *)_ptr;
    for(; size >= 2; size -= 2) sum += *ptr++;
    return(size == 0)? sum : sum + *((unsigned __int8 *)ptr);
  }
};
