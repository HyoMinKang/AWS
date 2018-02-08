#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

/** @brief Initialize AWS IoT Client instance with certificate and private key
 *
 * @ param[out] shadowClient 				Uninitialized AWS IoT Client instance
 * @ param[in] certificate_pem_crt_start 		Location of Device certs signed by AWS IoT service
 * @ param[in] private_pem_key_start			Location of Device private key
 * @ param[in] aws_root_ca_pem_start			Location with the Filename of the Root CA
 *
 * @ return Reference IoT_Error_t in the aws_iot_error.h
 * */
IoT_Error_t shadow_init(AWS_IoT_Client * shadowClient,
					   const char * certificate_pem_crt_start,
					   const char * private_pem_key_start,
					   const char * aws_root_ca_pem_start);

/** @brief Remove ' ', '\t', '\n', '\r' in the payload string.
 *
 * @param payload Target string
 * @param len Length of payload except of '\0';
 *
 * @return Length of new string except of '\0'
 * */
size_t serialize_payload(char * payload, size_t len);
