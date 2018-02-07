/*
 * key_store.c
 *
 *  Created on: 2018. 1. 25.
 *      Author: joondong
 */

#include "include/opkey_store.h"

#include "string.h"
#include "esp_log.h"
#include "stdlib.h"

//static const char *TAG_KEYSTORE = "key_store";
static const char * default_key_value = "0000";

static nvs_handle handle = 0;
static const char * KEY_STORE_NAME = "operation_key";
static const char * OP_KEY_KEY = "operation_key";
static char * key_value = NULL;

esp_err_t opkey_store_init() {
	esp_err_t err = nvs_open(KEY_STORE_NAME, NVS_READWRITE, &handle);

	if (err == ESP_OK) {
		//ESP_LOGE(TAG_KEYSTORE, "Key Store is opened");
		size_t value_len = 0;
		// Read the size of memory space required for
		err = nvs_get_str(handle, OP_KEY_KEY, NULL, &value_len);

		if(err == ESP_OK && value_len > 0) {
			key_value = (char *)malloc(value_len); // value_len include '\0'
			err = nvs_get_str(handle, OP_KEY_KEY, key_value, &value_len);
		} else if (err == ESP_ERR_NVS_NOT_FOUND) {
			key_value = (char *)default_key_value;
			err = nvs_set_str(handle, OP_KEY_KEY, default_key_value);
			if(err == ESP_OK)
				err = ESP_ERR_NOT_FOUND;
		}
	}
	return err;
}

esp_err_t set_opkey(const char * op_key) {
	esp_err_t err = ESP_FAIL;

	if(handle != 0 && key_value != NULL) {
		err = nvs_set_str(handle, OP_KEY_KEY, op_key);
		if(err == ESP_OK) {
			free(key_value);
			key_value = malloc(strlen(op_key) + 1);
			if(key_value != NULL)
				strcpy(key_value, op_key);
			err = nvs_commit(handle);
		}
	}

	return err;
}

char * get_opkey() {
	if(key_value != NULL) {
		return key_value;
	} else {
		return NULL;
	}
}
