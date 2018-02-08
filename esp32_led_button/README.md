# AWS LED Button Example

This project, implemented on ESP-IDF, communicate with Android app client through AWS IoT and is responsible for `Thing` client.

To do this, it is used subscribing, publishing and getting/updating shadow functionalities of the [AWS IoT C SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C).

## Preparations

You must install private key and certificate and enter custom endpoint.

To do this, see the README.md in the parent directory.

## Set Thing Name

For this example, you will need to set a Thing Name to `LEDButton12345670` under `make menuconfig` -> `Example Configuration` -> `AWS IoT Thing Name`.

`12345670` following `LEDButton` is device serial numver(DSN). You can chage DSN on your mind. If you change DSN, you should change `COMMAND_TOPIC`, `RESULT_TOPIC` in the `ed_button.c`

You should create Thing named `LEDButton12345670` in the AWS IoT.

## JSON documents

### Thing Shadow

This project does not subscribe `delta` of `Thing Shadow` for receiving command from Android app client because `operation key` include in command, unlike [thing_shadow example](https://github.com/espressif/esp-idf/tree/master/examples/protocols/aws_iot/thing_shadow)
<pre><code>
{
	"reported":{
		"ledStatus":&#62;bool&#62;
	}
}
</code></pre>

### `LEDButton<DSN>/command` topic

>{
>
>	"operation_key":"\<String\>",
>
>	"operation_code":\<Integer\>,
>
>	"operation_data":"\<String\>"
>
>}

### `LEDButton<DSN>/command` topic

>{
>
>	"operaion_code":\<Integer\>,
>
>	"operation_result":\<bool\>,
>
>	"result_message":"\<String\>"
>
>}

## Control LED by pressed boot button

### in progress..

## Control LED by Android app client

### in progress..

## Change operation key proccess by Android app client