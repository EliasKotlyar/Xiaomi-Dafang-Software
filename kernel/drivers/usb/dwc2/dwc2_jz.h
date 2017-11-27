#ifndef __DRIVERS_USB_DWC2_JZ4780_H
#define __DRIVERS_USB_DWC2_JZ4780_H

void jz_set_charger_current(struct dwc2 *dwc);
void jz_set_vbus(struct dwc2 *dwc, int is_on);
int dwc2_get_id_level(struct dwc2 *dwc);
void dwc2_input_report_power2_key(struct dwc2 *dwc);
void dwc2_gpio_irq_mutex_lock(struct dwc2 *dwc);
void dwc2_gpio_irq_mutex_unlock(struct dwc2 *dwc);

void dwc2_clk_enable(struct dwc2 *dwc);
void dwc2_clk_disable(struct dwc2 *dwc);
int dwc2_clk_is_enabled(struct dwc2 *dwc);
int dwc2_suspend_controller(struct dwc2 *dwc);
int dwc2_resume_controller(struct dwc2* dwc);
#endif	/* __DRIVERS_USB_DWC2_JZ4780_H */
