/*
 * boot_button_setting.h
 *
 *  Created on: 2018. 1. 19.
 *      Author: joondong
 */

#define GPIO_INPUT_IO_0     0
void boot_button_configure(gpio_isr_t gpio_isr_handler, TaskFunction_t gpio_task, UBaseType_t priority ,const uint32_t StackDepth, TaskHandle_t * gpio_task_handle);
