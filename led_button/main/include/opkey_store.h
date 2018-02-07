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

/*
 * @ brief initialize key store
 *
 * This module is only responsible for saving and loadong operation key.
 * To do this, this function set configuration related to nvs library.
 *
 * @ return		Reference "esp_err.h" and "nvs.h"
 * */
esp_err_t opkey_store_init();

/*
 * @ brief change the operation key
 *
 * It may need to check current key outside this module.
 *
 * @ param 	op_key	new key to change, including '\0'
 *
 * @ return
 * */
esp_err_t set_opkey(const char * op_key);

/*
 * @ brief get current operation key
 *
 * It may need to check current key outside this module.
 *
 * return 	current operation key string including '\0'
 * */
char * get_opkey();

#endif /* MAIN_INCLUDE_OPKEY_STORE_H_ */
