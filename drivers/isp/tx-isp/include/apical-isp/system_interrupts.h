#if !defined(__SYSTEM_INTERRUPTS_H__)
#define __SYSTEM_INTERRUPTS_H__

#include "apical_types.h"
#define APICAL_IRQ_COUNT 16
typedef uint32_t system_fw_interrupt_mask_t;
typedef void (*system_interrupt_handler_t)(void* ptr);

#define APICAL_IRQ_MASK(num) (1 << num)
void system_init_interrupt(void);

void system_set_interrupt_handler(uint8_t source, system_interrupt_handler_t handler, void* param);

void system_hw_interrupts_enable(void);
void system_hw_interrupts_disable(void);

#endif /* __SYSTEM_INTERRUPTS_H__ */
