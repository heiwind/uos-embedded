/*
FUNCTION
        <<memcpy>>---copy memory regions

ANSI_SYNOPSIS
        #include <string.h>
        void* memcpy(void *<[out]>, const void *<[in]>, size_t <[n]>);

TRAD_SYNOPSIS
        void *memcpy(<[out]>, <[in]>, <[n]>
        void *<[out]>;
        void *<[in]>;
        size_t <[n]>;

DESCRIPTION
        This function copies <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<memcpy>> returns a pointer to the first byte of the <[out]>
        region.

PORTABILITY
<<memcpy>> is ANSI C.

<<memcpy>> requires no supporting OS subroutines.

QUICKREF
        memcpy ansi pure
	*/

#include <runtime/lib.h>

#undef memcpy

#define CPU_ALIGN sizeof (long)
#define CPU_ALIGN_MASK (CPU_ALIGN-1)

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  ( ((long)X | ((long)Y) & CPU_ALIGN_MASK )

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (CPU_ALIGN << 2)
#define BIGBLOCKN       (BIGBLOCKSIZE/sizeof(long))

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (CPU_ALIGN)

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

void *
memcpy(void *dst0, const void *src0, size_t len0)
{
  unsigned char *dst = (unsigned char *)dst0;
  const unsigned char *src = (unsigned char *)src0;
  long *aligned_dst;
  const long *aligned_src;
  size_t   len =  len0;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len)) {
      unsigned dmis = (long)dst & CPU_ALIGN_MASK;
      unsigned smis = (long)src & CPU_ALIGN_MASK;
      if (dmis == smis)
    {
          if (dmis > 0)
          for (dmis = CPU_ALIGN-dmis; dmis > 0; dmis--, len--)
            *dst++ = *src++;

      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* Copy 4X long words at a time if possible.  */
      while (len >= BIGBLOCKSIZE)
        {
#         ifdef MIPS32 
          aligned_dst[0] = aligned_src[0];
          aligned_dst[1] = aligned_src[1];
          aligned_dst[2] = aligned_src[2];
          aligned_dst[3] = aligned_src[3];
          aligned_dst += BIGBLOCKN;
          aligned_src += BIGBLOCKN;
#         else
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
#         endif
          len -= BIGBLOCKSIZE;
        }

      /* Copy one long word at a time if possible.  */
      while (len >= LITTLEBLOCKSIZE)
        {
          *aligned_dst++ = *aligned_src++;
          len -= LITTLEBLOCKSIZE;
        }

       /* Pick up any residual with a byte copier.  */
      dst = (unsigned char*)aligned_dst;
      src = (unsigned char*)aligned_src;
    } // if (dmis == smis)
#   if CPU_HARD_MISALIGN > 0
        // try to operate with longs and shifts
    else if (len > BIGBLOCKSIZE){
        if (dmis > smis){
            const unsigned shift = (dmis - smis)*8;
            const unsigned deshift = 32-shift;
            const unsigned shmask= (1 << shift)-1; 
            if (smis > 0)
            for (; smis < CPU_ALIGN; smis++, len--)
              *dst++ = *src++;
            /* Copy one long word at a time if possible.  */
            aligned_dst = (long*)((unsigned)dst & ~CPU_ALIGN_MASK);
            aligned_src = (long*)src;
            unsigned tmp = *aligned_dst & shmask;

#           if UOS_FOR_SPEED > 1
            while (len >= BIGBLOCKSIZE)
              {
                unsigned sd = aligned_src[0];
                aligned_dst[0] = (sd << shift) | tmp;
                unsigned tmp2 = (sd >> deshift);

                unsigned sd2 = aligned_src[1];
                aligned_dst[1] = (sd2 << shift) | tmp2;
                tmp = (sd2 >> deshift);

                sd = aligned_src[2];
                aligned_dst[2] = (sd << shift) | tmp;
                tmp2 = (sd >> deshift);

                sd2 = aligned_src[3];
                aligned_dst[3] = (sd2 << shift) | tmp2;
                tmp = (sd2 >> deshift);

                len -= BIGBLOCKSIZE;
                aligned_src += BIGBLOCKN;
                aligned_dst += BIGBLOCKN;
              }
#           endif

            while (len >= LITTLEBLOCKSIZE)
              {
                unsigned sd = *aligned_src++;
                uint64_t shtmp = (uint64_t)sd << shift; 
                *aligned_dst++ = ((uint32_t)shtmp) | tmp;
                tmp = shtmp >> 32;
                /*
                *aligned_dst++ = (sd << shift) | tmp;
                tmp = (sd >> deshift)&shmask;
                */
                len -= LITTLEBLOCKSIZE;
              }
            /* Pick up any residual with a byte copier.  */
            *aligned_dst = (*aligned_dst & ~shmask) | tmp;
           dst = (unsigned char*)aligned_dst+(shift/8);
           src = (unsigned char*)aligned_src;
        }
        else {//(dmis > smis)
            const unsigned deshift = (smis - dmis)*8;
            const unsigned shift = 32-deshift;
            const unsigned shmask= (1 << shift)-1;
            if (dmis > 0)
            for (; dmis < CPU_ALIGN; dmis++, len--)
              *dst++ = *src++;
            /* Copy one long word at a time if possible.  */
            aligned_src = (long*)((unsigned)src & ~CPU_ALIGN_MASK);
            aligned_dst = (long*)dst;
            unsigned tmp = ((*aligned_src++) >> deshift) & shmask;

#           if UOS_FOR_SPEED > 1
            while (len >= BIGBLOCKSIZE)
              {
                unsigned sd = aligned_src[0];
                aligned_dst[0] = (sd << shift) | tmp;
                unsigned tmp2 = (sd >> deshift); //& shmask;

                unsigned sd2 = aligned_src[1];
                aligned_dst[1] = (sd2 << shift) | tmp2;
                tmp = (sd2 >> deshift); //& shmask;

                sd = aligned_src[2];
                aligned_dst[2] = (sd << shift) | tmp;
                tmp2 = (sd >> deshift); //& shmask;

                sd2 = aligned_src[3];
                aligned_dst[3] = (sd2 << shift) | tmp2;
                tmp = (sd2 >> deshift); //& shmask;

                len -= BIGBLOCKSIZE;
                aligned_src += BIGBLOCKN;
                aligned_dst += BIGBLOCKN;
              }
#           endif

            while (len >= LITTLEBLOCKSIZE)
              {
                unsigned sd = *aligned_src++;
                *aligned_dst++ = (sd << shift) | tmp;
                tmp = (sd >> deshift); //& shmask;
                len -= LITTLEBLOCKSIZE;
              }
            /* Pick up any residual with a byte copier.  */
            dst = (unsigned char*)aligned_dst;
            src = (unsigned char*)aligned_src-(shift/8);
        }
    }
#   endif
  }//!TOO_SMALL(len)

  while (len--)
    *dst++ = *src++;

  return dst0;
}
