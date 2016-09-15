// ID: SPO-UOS-milandr-mil-std-1553_bc.h VER: 1.0.0
//
// История изменений:
//
// 1.0.0	Начальная версия
//
#ifndef __MIL1553_MILANDR_BC_H__
#define __MIL1553_MILANDR_BC_H__

void start_slot(milandr_mil1553_t *mil, mil_slot_desc_t slot, uint16_t *pdata);
void mil_std_1553_bc_handler(struct milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg);

#endif
