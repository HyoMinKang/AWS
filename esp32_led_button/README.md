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
		"ledStatus":&#60;bool&#62;
	}
}
</code></pre>

### 'LEDButton<DSN>/command' topic

<pre><code>
{
	"operation_key":"&#60;String&#62",
	"operation_code":&#60Integer&#62,
	"operation_data":"&#60String&#62"
}
</code></pre>

"operation_code"
0 -> LED OFF  
1 -> LED ON  
2 -> Change operation key

### 'LEDButton<DSN>/result' topic

<pre><code>
{
	"operaion_code":&#60Integer&#62,
	"operation_result":&#60bool&#62,
	"result_message":"&#60String&#62"
}
</code></pre>

## Control LED by pressed boot button

1. User press boot button.  
2. Updated `Thing Shadow' in the AWS IoT.  
3. Changed led status, only if `Thing Shadow` update success.  
4. Published `LEDButton<DSN>/result` topic to report result to 1Android app client1, 

## Control LED by Android app client

1. `Thing`(this project) subscribe `LEDButton<DSN>/command` topic.  
2. `Android app client` publish `LEDButton<DSN>/command` topic("operation_code" is 0(LED OFF) or 1(LED ON) at this time).  
3. `Thing` update `Thing Shadow` in the AWS IoT according to command, then change LED status only if `Thing Shadow` update success.  
4. `Thing` publish `LEDButton<DSN>/result` topic to report result to Android app client.  

### in progress..

## Change operation key proccess by Android app client