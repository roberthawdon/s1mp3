#pragma once

// -----------------------------------------------------------------------------------------------
class Sum32 {
  public: unsigned __int32 generate(const void *_ptr, unsigned long size)
  {
    register unsigned __int32 sum = 0;
    register unsigned __int32 *ptr = (unsigned __int32 *)_ptr;
    for(; size >= 4; size -= 4) sum += *ptr++;
    switch(size)
    {
      case 1: return sum + *((unsigned __int8 *)ptr);
      case 2: return sum + *((unsigned __int16 *)ptr);
      case 3: return sum + *((unsigned __int16 *)ptr) + (((unsigned __int8 *)ptr)[2] << 16);
      default: return sum;
    }
  }
};
