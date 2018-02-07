# AWS LED Button Example

This is an combination of the [AWS IoT C SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C) "shadow_sample" and "subscribe_publish" examples for ESP-IDF.

## Configuring Your Device

### Installing Private Key & Certificate

As part of creating a device certificate, you downloaded a Private Key (`xxx-private.pem.key`) and a Certificate file (`xxx-certificate.pem.crt`). These keys need to be loaded by the ESP32 to identify itself.

There are currently two options for how to load the key & cert.

* Embed the files into the app binary (default)
* Load the files from SD card

### Option 1: Embedded Key & Cert into App Binary

Copy the `.pem.key` and `.pem.crt` files to the `main/certs` subdirectory of the example. Rename them by removing the device-specific prefix - the new names are `private.pem.key` and `certificate.pem.crt`.

As these files are bound to your AWS IoT account, take care not to accidentally commit them to public source control. In a commercial IoT device these files would be flashed to the device via a provisioning step, but for these examples they are compiled in.

### Option 2: Loading Key & Cert from SD Card

The alternative to embedding the key and certificate is to load them from a FAT filesystem on an SD card.

Before loading data from SD, format your SD card as FAT and run the `examples/storage/sd_card` example on it to verify that it's working as expected in ESP-IDF. This helps cut down the possible causes of errors in the more complex AWS IoT examples!

Run `make menuconfig`, navigate to "Example Configuration" and change "AWS IoT Certificate Source" to "Load from SD card".

Three new prompts will appear for filenames for the device key, device certificate and root CA certificate path. These paths start with `/sdcard/` as this is where the example mounts the (FAT formatted) SD card.

Copy the certificate and key files to the SD card, and make sure the file names match the names given in the example configuration (either rename the files, or change the config). For the Root CA certificate file (which is not device-specific), you can find the file in the `main/certs` directory or download it from AWS.

*Note: By default, esp-idf's FATFS support only allows 8.3 character filenames. However, the AWS IoT examples pre-configure the sdkconfig to enable long filenames. If you're setting up your projects, you will probably want to enable these options as well (under Component Config -> FAT Filesystem Support). You can also consider configure the FAT filesystem for read-only support, if you don't need to write to the SD card.*

## Find & Set AWS Endpoint Hostname

Your AWS IoT account has a unique endpoint hostname to connect to. To find it, open the AWS IoT Console and click the "Settings" button on the bottom left side. The endpoint hostname is shown under the "Custom Endpoint" heading on this page.

Run `make menuconfig` and navigate to `Component Config` -> `Amazon Web Service IoT Config` -> `AWS IoT MQTT Hostname`. Enter the host name here.

*Note: It may seem odd that you have to configure parts of the AWS settings under Component Config and some under Example Configuration.* The IoT MQTT Hostname and Port are set as part of the component because when using the AWS IoT SDK's Thing Shadow API (in examples or in other projects) the `ShadowInitParametersDefault` structure means the Thing Shadow connection will default to that host & port. You're not forced to use these config values in your own projects, you can set the values in code via the AWS IoT SDK's init parameter structures - `ShadowInitParameters_t` for Thing Shadow API or `IoT_Client_Init_Params` for MQTT API.

### (Optional) Set Client ID

Run `make menuconfig`. Under `Example Configuration`, set the `AWS IoT Client ID` to a unique value.

The Client ID is used in the MQTT protocol used to send messages to/from AWS IoT. AWS IoT requires that each connected device within a single AWS account uses a unique Client ID. Other than this restriction, the Client ID can be any value that you like. The example default should be fine if you're only connecting one ESP32 at a time.

In a production IoT app this ID would be set dynamically, but for these examples it is compiled in via menuconfig.

## Set Thing Name

For this example, you will need to set a Thing Name to `LEDButton12345670` under `make menuconfig` -> `Example Configuration` -> `AWS IoT Thing Name`.

`12345670` following `LEDButton` is device serial numver(DSN). You can chage DSN on your mind. If you change DSN, you should change `COMMAND_TOPIC`, `RESULT_TOPIC` in the `ed_button.c`

You should create Thing named `LEDButton12345670` in the AWS IoT.

# Control LED by pressed boot button

### in progress..

# Control LED by Android app client

### in progress..
