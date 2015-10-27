#include <usb/usbdev.h>
#include "usbdevhal.h"
#include <kernel/internal.h>

ARRAY (hp_stack, 1000);
ARRAY (lp_stack, 1000);

#if 0
static void dump (const void *addr, int size)
{
    const unsigned char *p = addr;
    int i;
    for (i = 0; i < size; ++i)
        debug_printf ("%02X ", p[i]);
    debug_printf ("\n");
}
#endif

static void stm32l_set_addr (unsigned addr)
{
    USB->DADDR = USB_EF | USB_ADD(addr);
}

static void stm32l_ep_attr (unsigned ep, int dir, unsigned attr, int max_size, int interval)
{
#if 0
    ep_state_t *eps = &ep_state[ep];

//debug_printf ("stm32l_ep_attr, ep = %d, dir = %d, attr = 0x%X, max_size = %d\n", ep, dir, attr, max_size);
    switch (attr & EP_ATTR_TRANSFER_MASK) {
    case EP_ATTR_CONTROL:

    break;
    case EP_ATTR_ISOCH:
        // TODO
    break;
    case EP_ATTR_BULK:
    case EP_ATTR_INTR:
        if (dir == USBDEV_DIR_IN) {
            eps->max_size[USBDEV_DIR_IN] = max_size;
            ARM_USB->SEPS[ep].CTRL = ARM_USB_EPEN | ARM_USB_EPRDY | ARM_USB_EPDATASEQ;
        } else {
            eps->max_size[USBDEV_DIR_OUT] = max_size;
            ARM_USB->SEPS[ep].CTRL = ARM_USB_EPEN | ARM_USB_EPRDY | ARM_USB_EPDATASEQ;
        }
    break;
    }
#endif
}

static void stm32l_ep_wait_out (unsigned ep, int ack)
{
//debug_printf ("ep_wait_out, ep = %d\n", ep);
    USB->EPR[ep] = (USB->EPR[ep] & (USB_EPR_RW_MASK | USB_STAT_RX_MASK)) ^ USB_STAT_RX_VALID;
}

static void stm32l_ep_wait_in (unsigned ep, int pid, const void *data, int size, int last)
{
//debug_printf ("ep_wait_in, ep = %d, pid = %d, data @ %p, size = %d, last = %d\n", ep, pid, data, size, last);
    volatile uint32_t *packet = (volatile uint32_t *)(USB_PACKET_BUF + EP0_TX_BUF_OFFSET);
    const uint16_t *p = data;
    USB_BTABLE_item_t *btable = (USB_BTABLE_item_t *) USB_PACKET_BUF;
    int i;
    
    btable[ep].COUNT_TX = size;
    for (i = 0; i < ((size + 1) >> 1); ++i)
        *packet++ = *p++;
        
    USB->EPR[ep] = (USB->EPR[ep] & (USB_EPR_RW_MASK | USB_STAT_TX_MASK)) ^ USB_STAT_TX_VALID;
}

static void stm32l_ep_stall (unsigned ep, int dir)
{
//debug_printf ("STALL EP%d\n", ep);
    USB->EPR[ep] = (USB->EPR[ep] & USB_EPR_RW_MASK) |
        ((USB->EPR[ep] & USB_STAT_RX_MASK) ^ USB_STAT_RX_STALL) |
        ((USB->EPR[ep] & USB_STAT_TX_MASK) ^ USB_STAT_TX_STALL);
}

static int stm32l_in_avail_bytes (unsigned ep)
{
    if ((USB->EPR[ep] & USB_STAT_TX_MASK) == USB_STAT_TX_VALID) {
//debug_printf("avail = 0\n");
        return 0;
    }
    else {
//debug_printf("avail = %d\n", USBDEV_EP0_MAX_SIZE);
        return USBDEV_EP0_MAX_SIZE;
    }
}

static usbdev_hal_t hal = {
    .set_addr       = stm32l_set_addr,
    .ep_attr        = stm32l_ep_attr,
    .ep_wait_out    = stm32l_ep_wait_out,
    .ep_wait_in     = stm32l_ep_wait_in,
    .ep_stall       = stm32l_ep_stall,
    .in_avail       = stm32l_in_avail_bytes
};

/*
static void usb_hp_interrupt (void *arg)
{
    stm32l_usbdev_t *u = arg;
    
    mutex_lock_irq (&hp_lock, IRQ_USB_HP, 0, 0);
    
    for (;;) {
        mutex_wait(&hp_lock);
        debug_printf("hp interrupt 1, ISTR = %08X\n", USB->ISTR);
        USB->ISTR = 0;
        debug_printf("hp interrupt 2, ISTR = %08X\n", USB->ISTR);
    }
}
*/

/*
static bool_t usb_lp_fast_handler (void *arg)
{
    stm32l_usbdev_t *u = arg;
    
    u->istr = USB->ISTR & USB->CNTR;
    if (u->istr & USB_RESET)
        u->epr_save = USB->EPR[0];

//debug_printf("#\n");
    
    USB->ISTR = 0;
    
    arch_intr_allow(IRQ_USB_LP);
    
    return 0;
}
*/

static void usb_lp_interrupt_task (void *arg)
{
    stm32l_usbdev_t *u = arg;
    unsigned istr;
    unsigned epr;
    unsigned bytes;
    unsigned i;
    volatile uint32_t *packet;
    
    //mutex_lock_irq (&u->lock, IRQ_USB_LP, usb_lp_fast_handler, u);
    mutex_lock_irq (&u->lock, IRQ_USB_LP, 0, 0);
    
    usbdevhal_reset(u->usbdev);
    
    SYSCFG->PMC |= SYSCFG_USB_PU;
    
    for (;;) {
        mutex_wait(&u->lock);
        
        istr = USB->ISTR & USB->CNTR;
        //debug_printf("lp interrupt, ISTR = %04X\n", USB->ISTR);
        if (istr == 0)
            continue;

        if (istr & USB_RESET) {
            //debug_printf("=== reset\n");
                
            USB->EPR[0] = USB_STAT_RX_VALID | USB_STAT_TX_NAK | USB_EP_TYPE_CONTROL;
            if (USBDEV_EP0_MAX_SIZE < 32)
                u->btable[0].COUNT_RX = USB_NUM_BLOCK(USBDEV_EP0_MAX_SIZE / 2);
            else
                u->btable[0].COUNT_RX = USB_BL_SIZE | USB_NUM_BLOCK(USBDEV_EP0_MAX_SIZE / 32 - 1);
            USB->DADDR = USB_EF;
            
            usbdevhal_reset(u->usbdev);
            
            USB->ISTR = ~USB_RESET;
            
            continue;
        }
        
        if (istr & USB_CTR) {
            epr = USB->EPR[0];

            if (epr & USB_CTR_TX) {
                usbdevhal_in_done (u->usbdev, 0, u->btable[0].COUNT_TX);
            }
            
            if (epr & USB_CTR_RX) {
                //debug_printf("--r %04X\n", u->btable[0].COUNT_RX);
                bytes = USB_GET_COUNT_RX(u->btable[0].COUNT_RX);
                packet = (volatile uint32_t *)(USB_PACKET_BUF + EP0_RX_BUF_OFFSET);
                for (i = 0; i < ((bytes + 1) >> 1); ++i)
                    u->rx_buf[i] = packet[i];
                if (epr & USB_SETUP) {
                    usbdevhal_out_done (u->usbdev, 0, USBDEV_TRANSACTION_SETUP, u->rx_buf, bytes);
                } else {
                    usbdevhal_out_done (u->usbdev, 0, USBDEV_TRANSACTION_OUT, u->rx_buf, bytes);
                }
            } 
        }
    }
}


void stm32l_usbdev_init (stm32l_usbdev_t *u, usbdev_t *owner, int io_prio, mem_pool_t *pool)
{
    u->usbdev = owner;
    u->mem = pool;
    
    usbdevhal_bind (owner, &hal, &u->lock);

    RCC->APB1ENR |= RCC_USBEN;
    RCC->APB2ENR |= RCC_SYSCFGEN;
    asm volatile ("dmb");
    
    USB->CNTR = USB_FRES;   // removing PWDN bit
    udelay(1);              // waiting for analog part to calm down
    USB->CNTR = 0;          // removing reset
    USB->ISTR = 0;          // clearing all spurious interrupts during startup
    
    USB->CNTR = USB_CTRM | USB_RESETM;
    
    u->btable = (USB_BTABLE_item_t *) USB_PACKET_BUF;
    u->btable[0].ADDR_TX = EP0_TX_BUF_OFFSET / 2;
    u->btable[0].ADDR_RX = EP0_RX_BUF_OFFSET / 2;
    USB->BTABLE = 0;
        
    task_create (usb_lp_interrupt_task, u, "usb_lp", io_prio, lp_stack, sizeof (lp_stack));
    //task_create (usb_hp_interrupt, u, "usb_hp", io_prio + 1, hp_stack, sizeof (hp_stack));
}
