/*
 * alexrayne <alexraynepe196@gmail.com> 2016/03/04
 * utf8 ru
 * */
#ifndef ELVEES_UTCP_CLIEN_TEST_SOURCE_H_
#define ELVEES_UTCP_CLIEN_TEST_SOURCE_H_

#include <kernel/uos.h>

#ifdef __cplusplus

namespace Measures {

    namespace Raw{
        static const unsigned sample_len = 256;
        static const unsigned ch_limit   = 16;

        // см Statistics1Ch::Tap
        typedef unsigned    tap_id;
        //размер семла сохраняемого в треке сырья от отвода
        inline
        unsigned    sizeof_sample(tap_id tap) {
            return sizeof(float)*sample_len;
        }

        //вычисляет общий размер семплов по всем каналам
        unsigned frame_size(unsigned chs = ~0);
    }; // namespace Raw

};//namespace Measures
#endif



// TODO need to move it to better place
inline
unsigned pop(uint32_t x){
    return __builtin_popcount(x);
}



#endif
