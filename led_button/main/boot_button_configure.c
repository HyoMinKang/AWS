/*
 * boot_button_setting.c
 *
 *  Created on: 2018. 1. 19.
 *      Author: joondong
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boot_button_configure.h"

static const char *TAG_GPIO = "gpio";

void boot_button_configure(gpio_isr_t gpio_isr_handler, TaskFunction_t gpio_task, UBaseType_t priority ,const uint32_t StackDepth, TaskHandle_t * gpio_task_handle) {
	gpio_config_t io_conf;

	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	//bit mask of the pins, use GPIO4/5 here
	io_conf.pin_bit_mask = (1ULL<<GPIO_INPUT_IO_0);
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	// Temporarily pin task to Core 1, due to FPU uncertainty
	xTaskCreatePinnedToCore(gpio_task, "button_task", StackDepth, NULL, priority, gpio_task_handle, 1);
	//install gpio isr service
	gpio_install_isr_service(0);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, NULL);
}
