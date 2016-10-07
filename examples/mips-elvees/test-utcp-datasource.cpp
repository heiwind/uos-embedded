#include <runtime/lib.h>
#include <malloc.h>

#include "test-utcp-datasource.h"



namespace Measures{
namespace Raw{

//вычисляет общий размер семплов по всем каналам
unsigned frame_size(unsigned chs){
    unsigned result = 0;
    for (unsigned c = 0; c < ch_limit; c++, chs = chs >>1 ){
        if ((chs&1) == 0)
            continue;
        unsigned l = sizeof_sample(0);
        result += l;
    }
    return result;
}

}; //namespace Raw

}; //namespace Measures
