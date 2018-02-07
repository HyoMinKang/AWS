#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "cJSON.h"

#include "aws_iot_helper.h"
#include "boot_button_configure.h"

static const char *TAG_SHADOW = "shadow";

static void shadow_params_init(ShadowInitParameters_t * sp,
							  const char * certificate_pem_crt_start,
							  const char * private_pem_key_start,
							  const char * aws_root_ca_pem_start) {
	ESP_LOGI(TAG_SHADOW, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
	sp->pHost = AWS_IOT_MQTT_HOST;
	sp->port = AWS_IOT_MQTT_PORT;

#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)
	sp->pClientCRT = certificate_pem_crt_start;
	sp->pClientKey = private_pem_key_start;
	sp->pRootCA = aws_root_ca_pem_start;
#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)
	sp->pClientCRT = DEVICE_CERTIFICATE_PATH;
	sp->pClientKey = DEVICE_PRIVATE_KEY_PATH;
	sp->pRootCA = ROOT_CA_PATH;
#endif
	sp->enableAutoReconnect = false;
	sp->disconnectHandler = NULL;

#ifdef CONFIG_EXAMPLE_SDCARD_CERTS
	ESP_LOGI(TAG_SHADOW, "Mounting SD card...");
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
			.format_if_mount_failed = false,
			.max_files = 3,
	};
	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG_SHADOW, "Failed to mount SD card VFAT filesystem.");
		abort();
	}
#endif
}

IoT_Error_t shadow_init(AWS_IoT_Client * shadowClient,
					   const char * certificate_pem_crt_start,
					   const char * private_pem_key_start,
					   const char * aws_root_ca_pem_start) {
	IoT_Error_t rc = FAILURE;

	ShadowInitParameters_t sp;
	shadow_params_init(&sp, certificate_pem_crt_start, private_pem_key_start, aws_root_ca_pem_start);

	ESP_LOGI(TAG_SHADOW, "Shadow Init");
	rc = aws_iot_shadow_init(shadowClient, &sp);
	if(SUCCESS != rc) {
		ESP_LOGE(TAG_SHADOW, "aws_iot_shadow_init returned error %d, aborting...", rc);
		abort();
	}

	ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
	scp.pMyThingName = CONFIG_AWS_EXAMPLE_THING_NAME;
	scp.pMqttClientId = CONFIG_AWS_EXAMPLE_CLIENT_ID;
	scp.mqttClientIdLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);

	ESP_LOGI(TAG_SHADOW, "Connecting to AWS...");
	rc = aws_iot_shadow_connect(shadowClient, &scp);
	if(SUCCESS != rc) {
		ESP_LOGE(TAG_SHADOW, "aws_iot_shadow_connect returned error %d, aborting...", rc);
		abort();
	}

	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_shadow_set_autoreconnect_status(shadowClient, true);
	if(SUCCESS != rc) {
		ESP_LOGE(TAG_SHADOW, "Unable to set Auto Reconnect to true - %d, aborting...", rc);
		abort();
	}

	if(rc == SUCCESS) {
		ESP_LOGI(TAG_SHADOW, "Connected to AWS");
	}

	return rc;
}

size_t serialize_payload(char * payload, size_t len) {
	size_t original_index = 0;
	size_t new_index = 0;

	for(; original_index < len; original_index++) {
		if(payload[original_index] != ' '
				&& payload[original_index] != '\t'
						&& payload[original_index] != '\n'
								&& payload[original_index] != '\r') {
			payload[new_index] = payload[original_index];
			new_index++;
		}
	}
	payload[new_index] = '\0';
	return new_index;
}

