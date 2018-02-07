/*
 * gpio_configure.h
 *
 *  Created on: 2018. 1. 25.
 *      Author: joondong
 */

#ifndef MAIN_INCLUDE_LED_GPIO_CONFIGURE_H_
#define MAIN_INCLUDE_LED_GPIO_CONFIGURE_H_

#include "stdint.h"
#include "esp_system.h"

esp_err_t led_configure(uint32_t pin_num);
esp_err_t control_led(uint32_t pin_num, uint32_t control);
esp_err_t get_led(uint32_t pin_num, bool * status);

#endif /* MAIN_INCLUDE_LED_GPIO_CONFIGURE_H_ */
