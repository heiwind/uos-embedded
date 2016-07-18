// ID: SPO-UOS-milandr-mil-std-1553_rt.h VER: 1.0.0
//
// История изменений:
//
// 1.0.0	Начальная версия
//
#ifndef __MIL1553_MILANDR_RT_H__
#define __MIL1553_MILANDR_RT_H__

void mil_std_1553_rt_handler(struct milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg);
#endif
