#include "include/led_gpio_configure.h"

#include "driver/gpio.h"

static uint64_t pin_bit_mask;

esp_err_t led_configure(uint32_t pin_num) {
	esp_err_t err = ESP_FAIL;
	pin_bit_mask = 1ULL << pin_num;
	gpio_config_t io_conf;

	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = pin_bit_mask;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	err = gpio_config(&io_conf);

	return err;
}

esp_err_t control_led(uint32_t pin_num, uint32_t control) {
	esp_err_t err = ESP_FAIL;

	if((pin_bit_mask & (1ULL << pin_num)) && (control == 0 || control == 1)) {
		err = gpio_set_level(pin_num, control);
	}

	return err;
}

esp_err_t get_led(uint32_t pin_num, bool * status) {
	esp_err_t err = ESP_FAIL;

	if(pin_bit_mask & (1ULL << pin_num)) {
		err = ESP_OK;
		if(gpio_get_level(pin_num)) {
			*status = true;
		} else {
			*status = false;
		}
	}

	return err;
}
