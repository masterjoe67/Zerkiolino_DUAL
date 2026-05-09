//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Endianess handling, swapping 16bit and 32bit.
//


#ifndef __I_SWAP__
#define __I_SWAP__

#ifdef __DJGPP__


#define SHORT(x)  ((signed short) (x))
#define LONG(x)   ((signed int) (x))

#define SYS_LITTLE_ENDIAN


#else  // __DJGPP__


#if ( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ )
  #define SYS_LITTLE_ENDIAN

  #define SHORT(x) ({ \
      const unsigned char *__p = (const unsigned char *)&(x); \
      (int16_t)(__p[0] | (__p[1] << 8)); \
  })

  #define LONG(x) ({ \
      const unsigned char *__p = (const unsigned char *)&(x); \
      (int32_t)(__p[0] | (__p[1] << 8) | (__p[2] << 16) | (__p[3] << 24)); \
  })
#endif


// cosmito from lsdldoom
#define doom_swap_s(x) \
        ((short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) 

#if ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
#define doom_wtohs(x) doom_swap_s(x)
#else
#define doom_wtohs(x) (short int)(x)
#endif


#endif  // __DJGPP__


#endif

