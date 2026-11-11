#ifndef __AXP210X_DRV_H__
#define __AXP210X_DRV_H__

typedef void (*irq_pwr_in_func)(void *arg);

int axp210x_reg_read(uint8_t addr, uint8_t reg, uint8_t *data);
int axp210x_reg_write(uint8_t addr, uint8_t reg, uint8_t data);
int axp210x_init(void);
void axp210x_deinit(void);
void axp210x_pwr_on_ctl(uint8_t level);
void axp210x_gpio_init(irq_pwr_in_func func);
uint8_t axp210x_pwr_in_read(void);

#endif
