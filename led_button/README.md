# AWS LED Button Example

This is an combination of the [AWS IoT C SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C) "shadow_sample" and "subscribe_publish" examples for ESP-IDF.

## Preparations

You must install private key and certificate and enter custom endpoint.

To do this, see the README.md in the parent directory.

## Set Thing Name

For this example, you will need to set a Thing Name to `LEDButton12345670` under `make menuconfig` -> `Example Configuration` -> `AWS IoT Thing Name`.

`12345670` following `LEDButton` is device serial numver(DSN). You can chage DSN on your mind. If you change DSN, you should change `COMMAND_TOPIC`, `RESULT_TOPIC` in the `ed_button.c`

You should create Thing named `LEDButton12345670` in the AWS IoT.

## Control LED by pressed boot button

### in progress..

## Control LED by Android app client

### in progress..
