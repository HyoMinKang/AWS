/*
 * key_store.h
 *
 *  Created on: 2018. 1. 25.
 *      Author: joondong
 */

#ifndef MAIN_INCLUDE_OPKEY_STORE_H_
#define MAIN_INCLUDE_OPKEY_STORE_H_

#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

esp_err_t opkey_store_init();
esp_err_t set_opkey(const char * op_key);
char * get_opkey();

#endif /* MAIN_INCLUDE_OPKEY_STORE_H_ */
