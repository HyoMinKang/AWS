/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Additions Copyright 2016 Espressif Systems (Shanghai) PTE LTD
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
/**
 * @file thing_shadow_sample.c
 * @brief A simple connected window example demonstrating the use of Thing Shadow
 *
 * See example README for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "cJSON.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#include "include/aws_iot_helper.h"
#include "include/boot_button_configure.h"
#include "include/opkey_store.h"
#include "include/led_gpio_configure.h"

/*!
 * The goal of this sample application is to demonstrate the capabilities of shadow, subscribe and publish
 * This device will turn on/off the led when received "LEDButton<DSN>/command" topic from client
 * or when button is pressed,
 * then respond to client by "LEDButton<DSN>/result" topic and update thing's shadow.

LEDButton<DSN>/command JSON document
{
	"operation_key": "<key value>",
	"operation_code": <int value>,
	"operation_data" : "<data>"(option, usually new key value is put)
}

LedButton<DSN>/result JSON document
{
	"operation_code: <int value>(operation_code performed),
	"operation_result: <bool value>,
	"result_data": "<result data>"
}

 * operation_code
 * 0 : LED OFF
 * 1 : LED ON
 * 2 : KEY CHANGE (need new key value in the operation_data)
 *
 * operation_result
 * true : success
 * false : fail
 *
 */

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


/* CA Root certificate, device ("Thing") certificate and device
 * ("Thing") key.

   Example can be configured one of two ways:

   "Embedded Certs" are loaded from files in "certs/" and embedded into the app binary.

   "Filesystem Certs" are loaded from the filesystem (SD card, etc.)

   See example README for more details.
*/
#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)

static const char * DEVICE_CERTIFICATE_PATH = CONFIG_EXAMPLE_CERTIFICATE_PATH;
static const char * DEVICE_PRIVATE_KEY_PATH = CONFIG_EXAMPLE_PRIVATE_KEY_PATH;
static const char * ROOT_CA_PATH = CONFIG_EXAMPLE_ROOT_CA_PATH;

#else
#error "Invalid method for loading certs"
#endif

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h which
 * uses menuconfig to find a default.
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h which
 * uses menuconfig to find a default.
 */
uint32_t port = AWS_IOT_MQTT_PORT;

// my project
#define 	DEVICE_SERIAL_NUMBER 	12345670

#define 		OP_LED_OFF 			0
#define 		OP_LED_ON			1
#define		OP_KEY_CHANGE		2

#define 	LED_GPIO_PIN	5

static const char *TAG_SHADOW = "shadow";
static const char *TAG_SUBPUB = "subpub";
static const char *TAG_YIELD = "yield";
static const char *TAG_WIFI = "wifi";
static const char *TAG_GPIO = "gpio";
static const char *TAG_KEYSTORE = "key_store";

static TaskHandle_t gpio_task_handle = NULL;
static TaskHandle_t aws_yield_task_handle = NULL;
static const char * COMMAND_TOPIC = "LEDButton12345670/command"; // for receiving
static const char * RESULT_TOPIC = "LEDButton12345670/result"; // for transmitting
__volatile static bool boot_button_in_progress;


//static const char * OP_KEY = "operation_key";
//static nvs_handle opkey_store_handle;
#define MAX_KEY_LENGTH 12

#define TYPE_VALID	(1U << 0) // top json validation
#define OPKEY_VALID (1U << 1) // "operation_key" json validation
#define OPCODE_VALID (1U << 2) // "operation_code" json valideation
#define OPDATA_VALID (1U << 3) // "operation_data" json validsation when "operation_code" = 2
#define VALID_OK 	TYPE_VALID | OPKEY_VALID | OPCODE_VALID | OPDATA_VALID

static AWS_IoT_Client shadowClient;
//static bool shadowUpdateInProgress;

// Initialize JSON structure
char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);

static bool led_status = false;
static jsonStruct_t ledStatus;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                                const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    ESP_LOGI(TAG_SHADOW, "Update Shadow Callback");
    boot_button_in_progress = false;

    if(SHADOW_ACK_TIMEOUT == status) {
        ESP_LOGI(TAG_SHADOW, "Update Timed out\n");
    } else if(SHADOW_ACK_REJECTED == status) {
        ESP_LOGI(TAG_SHADOW, "Update Rejected\n");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        ESP_LOGI(TAG_SHADOW, "Update Accepted");
        //ESP_LOGI(TAG_SHADOW, "On Device: led status %s\n", *(bool *)pContextData ? "true" : "false");
    }
}

void ShadowGetStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
							 const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(action);
	IOT_UNUSED(pContextData);

	ESP_LOGE(TAG_SHADOW, "Get Shadow Callback");
	boot_button_in_progress = false;

	bool json_valid = false;

	if(SHADOW_ACK_TIMEOUT == status) {
		ESP_LOGI(TAG_SHADOW, "Get Timed Out");
	} else if(SHADOW_ACK_REJECTED == status) {
		ESP_LOGI(TAG_SHADOW, "Get Rejected");
	} else if(SHADOW_ACK_ACCEPTED == status) {
		ESP_LOGI(TAG_SHADOW, "Get Accepted");
		cJSON * json = cJSON_Parse(pReceivedJsonDocument);
		cJSON * ledStatus_json = NULL;
		if(json != NULL && json->child != NULL && json->child->child != NULL)
			ledStatus_json= cJSON_GetObjectItem(json->child->child, "ledStatus");

		//ESP_LOGE(TAG_SHADOW, "%s", cJSON_Print(json));
		if(ledStatus_json != NULL) {
			// {"state":{"reported":{"ledStatus":<true/false>...
			if((strcmp("ledStatus", ledStatus_json->string) == 0)
					&& ledStatus_json->valuestring == NULL) {
				json_valid = true;
				if(ledStatus_json->type == cJSON_True) {
					led_status = true;
				} else {
					led_status = false;
				}
			}
		}

		if(json_valid) {
			esp_err_t err = control_led(LED_GPIO_PIN, (uint32_t)led_status);
			if(err != ESP_OK) {
				led_status = !led_status;
			} else {
				ESP_LOGI(TAG_SHADOW, "ledStatus = %s", led_status ? "true" : "false");
			}
		} else {
			ESP_LOGI(TAG_SHADOW, "Get shadow response json is wrong.");
		}

		if(json != NULL)
			cJSON_Delete(json);
	}
	printf("\n");
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG_WIFI, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
	esp_err_t err = ESP_FAIL;
	char cPayload[100];
	uint32_t stage = 0;

    ESP_LOGW(TAG_SUBPUB, "Subscribe callback");
    //ESP_LOGW(TAG_SUBPUB, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);

    // used stack instead of heap
    char buffer[params->payloadLen + 1]; // automatically buffer[params->payloadLen] == 0('\0')
    sprintf(buffer, "%.*s", (int) params->payloadLen, (char *)params->payload);
    ESP_LOGW(TAG_SUBPUB, "%s", buffer);
    serialize_payload(buffer, params->payloadLen);
    ESP_LOGW(TAG_SUBPUB, "%s", buffer);

    cJSON * json = cJSON_Parse(buffer);
    cJSON * operation_key = NULL;
    cJSON * operation_code = NULL;
    cJSON * operation_data = NULL;
    uint32_t valid_json = 0;
    bool led_operation = false;

    // check json validation
    if(json != NULL) {
    	valid_json |= TYPE_VALID;
    	operation_key = cJSON_GetObjectItem(json, "operation_key");
    	if(operation_key != NULL && operation_key->valuestring != NULL) {
    		valid_json |= OPKEY_VALID;
    	}

    	operation_code = cJSON_GetObjectItem(json, "operation_code");

    	if(operation_code != NULL &&
    			!(operation_code->valueint < OP_LED_OFF) &&
				!(operation_code->valueint > OP_KEY_CHANGE) &&
				operation_code->valuestring == NULL) {

    		valid_json |= OPCODE_VALID;

    		if(operation_code->valueint == OP_KEY_CHANGE) {
    			operation_data = cJSON_GetObjectItem(json, "operation_data");
    			if(operation_data != NULL && operation_data->valuestring != NULL) {
    				valid_json |= OPDATA_VALID;
    			}
    		} else {
    			valid_json |= OPDATA_VALID;
    		}
    	}
    }

    if(valid_json == (VALID_OK)) {
    	// json type is valid
    	stage = 1;

    	if(strcmp(operation_key->valuestring, get_opkey()) == 0)
    		err = ESP_OK;

    	if(err == ESP_OK) {
    		// confirmed key
    		stage = 2;

    		switch(operation_code->valueint) {
    		case OP_LED_OFF:
    			led_status = false;
    			led_operation = true;
    			break;
    		case OP_LED_ON:
    			led_status = true;
    			led_operation = true;
    			break;
    		case OP_KEY_CHANGE:
    			err = set_opkey(operation_data->valuestring);
    			ESP_LOGW(TAG_SUBPUB, "set_opkey err code = %d", err);
    			if(err == ESP_OK)
    				stage = 3;
    			ESP_LOGW(TAG_SUBPUB, "op_code : KEY CHANGE, result : %s", (err == ESP_OK) ? "success" : "fail");
    			break;
    		default:
    			ESP_LOGW(TAG_SUBPUB, "op_code : Unknown");
    			break;
    		}
    	} else {
    		ESP_LOGW(TAG_SUBPUB, "operation key is wrong");
    	}
    } else { // Reference line 176
    	ESP_LOGW(TAG_SUBPUB, "JSON document is wrong (%u%u%u%u)",
    			(valid_json & OPDATA_VALID) >> 3,
				(valid_json & OPCODE_VALID) >> 2,
				(valid_json & OPKEY_VALID) >> 1,
				(valid_json & TYPE_VALID));
    }

    IoT_Error_t rc = FAILURE;

    if(valid_json == (VALID_OK) && err == ESP_OK && led_operation) {
    	ESP_LOGI(TAG_SHADOW, "Updating shadow...");
    	rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    	if(SUCCESS == rc) {
    		rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, &ledStatus);
    		if(SUCCESS == rc) {
    			rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    			if(SUCCESS == rc) {
    				//ESP_LOGI(TAG_SHADOW, "Update Shadow: %s", JsonDocumentBuffer);
    				rc = aws_iot_shadow_update(&shadowClient, CONFIG_AWS_EXAMPLE_THING_NAME, JsonDocumentBuffer,
    						ShadowUpdateStatusCallback, (void *)&led_status, 4, true);
    			}
    		}
    	}

    	if(SUCCESS != rc) {
    		boot_button_in_progress = false;
    		led_status = !led_status; // return to previous status.
    	} else {
    		esp_err_t err = control_led(LED_GPIO_PIN, (uint32_t)led_status); // actual led toggle
    		if(err != ESP_OK) {
    			led_status = !led_status; // return to previous status.
    		} else {
    			ESP_LOGI(TAG_SHADOW, "op_code : LED %s, result : %s", led_status ? "ON" : "OFF" ,(err == ESP_OK) ? "success" : "fail");
    		}
    	}
    }

    ESP_LOGW(TAG_SUBPUB, "Result publishing...");

    IoT_Publish_Message_Params pubParams;
    int op_code = 3;
    char * result_data;

    if(valid_json & OPCODE_VALID)
    	op_code = operation_code->valueint;

    if(valid_json != (VALID_OK)) {
    	result_data = "json document is wrong";
    } else if(stage == 1) {
    	result_data = "operation key is wrong";
    } else if(stage == 3){
    	result_data = "operation key is exchanged";
    } else {
    	result_data = "nothing";
    }

    sprintf(cPayload, "{\"operation_code\":%d,\"operation_result\":%s,\"result_data\":\"%s\"}", op_code, (err == ESP_OK) ? "true" : "false", result_data);
    //ESP_LOGW(TAG_SUBPUB, "payload : %s", (char *)cPayload);

    pubParams.qos = QOS1;
    pubParams.payload = (void *) cPayload;
    pubParams.isRetained = 0;
    pubParams.payloadLen = strlen(cPayload);

    const int TOPIC_LEN = strlen(RESULT_TOPIC);

    rc = aws_iot_mqtt_publish(&shadowClient, RESULT_TOPIC, TOPIC_LEN, &pubParams);
    if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
    		ESP_LOGW(TAG_SUBPUB, "QOS1 publish ack not received.");
    }
    if(rc == SUCCESS) {
    		ESP_LOGW(TAG_SUBPUB, "Result published\n");
    }

    if(valid_json & TYPE_VALID) {
    		cJSON_Delete(json);
    }
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	if(boot_button_in_progress == false) {
		boot_button_in_progress = true;
		vTaskNotifyGiveFromISR(gpio_task_handle, NULL);
	}
}

static void boot_button_task(void* arg)
{
	IoT_Error_t rc = FAILURE;
	char cPayload[100];

	esp_err_t err = opkey_store_init();
	switch(err) {
	case ESP_OK:
		ESP_LOGE(TAG_KEYSTORE, "Success getting operation key ( %s )", get_opkey());
		break;
	case ESP_ERR_NOT_FOUND:
		ESP_LOGE(TAG_KEYSTORE, "Success setting default operation key ( %s )", get_opkey());
		break;
	default:
		ESP_LOGE(TAG_KEYSTORE, "Error occurred( %d )", err);
		while(1);
		break;
	}

	ledStatus.cb = NULL;
	ledStatus.pData = &led_status;
	ledStatus.pKey = "ledStatus";
	ledStatus.type = SHADOW_JSON_BOOL;

	/* Wait for WiFI to show as connected */
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	rc = shadow_init(&shadowClient, (const char *)certificate_pem_crt_start, (const char *)private_pem_key_start, (const char *)aws_root_ca_pem_start);

	// TODO : get shadow.
	if(SUCCESS == rc) {
		ESP_LOGI(TAG_SHADOW, "Getting Shadow...");
		// last parameter must be set to true
		rc = aws_iot_shadow_get(&shadowClient, CONFIG_AWS_EXAMPLE_THING_NAME, ShadowGetStatusCallback, NULL, 5, true);
		boot_button_in_progress = true;
	}

	const int TOPIC_LEN = strlen(COMMAND_TOPIC);
	if(SUCCESS == rc) {
		ESP_LOGW(TAG_SUBPUB, "Subscribing...");
		rc = aws_iot_mqtt_subscribe(&shadowClient, COMMAND_TOPIC, TOPIC_LEN, QOS0, iot_subscribe_callback_handler, NULL);
		if(SUCCESS != rc) {
			ESP_LOGE(TAG_SUBPUB, "Error subscribing : %d ", rc);
			abort();
		} else {
			xTaskNotifyGive(aws_yield_task_handle);
		}
	}

	vTaskDelay(100);

	while(1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    	ESP_LOGE(TAG_GPIO, "boot button !!");

    	//err = control_led(LED_GPIO_PIN, (uint32_t)(!led_status));

    	// led state toggle.
    	led_status = !led_status;

    	rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    	if(SUCCESS == rc) {
    		rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, &ledStatus);
    		if(SUCCESS == rc) {
    			rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    			if(SUCCESS == rc) {
    				rc = aws_iot_shadow_update(&shadowClient, CONFIG_AWS_EXAMPLE_THING_NAME, JsonDocumentBuffer,
    						ShadowUpdateStatusCallback, (void *)&led_status, 4, true);
    			}
    		}
    	}

    	if(SUCCESS != rc) {
    		boot_button_in_progress = false;
    		led_status = !led_status; // return to previous status.
    		ESP_LOGE(TAG_SHADOW, "An error occurred in aws_iot functions. %d", rc);
    	} else {
    		err = control_led(LED_GPIO_PIN, (uint32_t)(led_status)); // actual led toggle
    		if(err != ESP_OK) {
    			led_status = !led_status; // return to previous status.
    		} else {
    			ESP_LOGW(TAG_SUBPUB, "Result publishing...");

    			// Pulblish result
    			IoT_Publish_Message_Params pubParams;

    			sprintf(cPayload, "{\"operation_code\":%d,\"operation_result\":%s,\"result_data\":\"nothing\"}", (int)led_status, (err == ESP_OK) ? "true" : "false");
    			//ESP_LOGW(TAG_SUBPUB, "payload : %s", (char *)cPayload);

    			pubParams.qos = QOS1;
    			pubParams.payload = (void *) cPayload;
    			pubParams.isRetained = 0;
    			pubParams.payloadLen = strlen(cPayload);

    			const int TOPIC_LEN = strlen(RESULT_TOPIC);

    			rc = aws_iot_mqtt_publish(&shadowClient, RESULT_TOPIC, TOPIC_LEN, &pubParams);
    			if(rc == MQTT_REQUEST_TIMEOUT_ERROR) {
    				ESP_LOGW(TAG_SUBPUB, "QOS1 publish ack not received.");
    			}
    			if(rc == SUCCESS) {
    				ESP_LOGW(TAG_SUBPUB, "Result published\n");
    			}
    		}
    	}
	}
}

static void aws_yield_task(void* arg) {
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	ESP_LOGW(TAG_YIELD, "aws_yield_task start !!");

	while(1) {
		aws_iot_shadow_yield(&shadowClient, 75);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void app_main()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = led_configure(LED_GPIO_PIN);
    if(err != ESP_OK)
    	ESP_LOGE(TAG_GPIO, "LED configuration erorr");

    initialise_wifi();

    /* Temporarily pin "boot_button_task" to Core 1, due to FPU uncertainty */
    boot_button_configure(gpio_isr_handler, &boot_button_task, 5, 35000, &gpio_task_handle);
    xTaskCreate(aws_yield_task, "aws_yiled_task", 10000, NULL, 6, &aws_yield_task_handle);
}
