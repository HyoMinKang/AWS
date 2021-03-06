# AWS LED Button Example

This project, implemented on ESP-IDF, communicate with [**Android app client**](https://github.com/JoonDong2/Android/tree/master/AWSLEDButton) through AWS IoT and is responsible for `Thing` client.

To do this, subscribing, publishing and getting/updating shadow of the [AWS IoT C SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C) are used.

## Video

You can see the video of the entire projects, including Android app client at the bottom of [this post](http://joondong.tistory.com/61?category=651762).  
This blog is written Korean, but I have plan to translate to English.

## Preparations

You have to install private key and certificate and enter custom endpoint in the ESP32.  
To do this, see the README.md in the parent directory.

Also you have to create policy like [this line](https://github.com/JoonDong2/AWS/tree/master/esp32_led_button#policy-in-the-aws-iot) in the AWS IoT, and attach this policy to the certificate downloaded to the ESP32.

## Set Thing Name

For this example, you will need to set a Thing Name to `LEDButton12345670` under `make menuconfig` -> `Example Configuration` -> `AWS IoT Thing Name`.

`12345670` following `LEDButton` is device serial numver(DSN). You can chage DSN. If you change DSN, you should change `COMMAND_TOPIC`, `RESULT_TOPIC` in the `led_button.c`

You should create Thing named `LEDButton12345670` in the AWS IoT.

## JSON documents

### Thing Shadow

#### This project does not subscribe `delta` of `Thing Shadow` for receiving command from Android app client because `operation key` is included in the command, unlike [thing_shadow example](https://github.com/espressif/esp-idf/tree/master/examples/protocols/aws_iot/thing_shadow). If operation key is included in the `Thing Shadow`, it may be subscribed or published by any clients.

<pre><code>
{
	"reported":{
		"ledStatus":&#60;bool&#62;
	}
}
</code></pre>

### `LEDButton<DSN>/command` topic

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

"operation_data"  
Entered new operation key. This is valid, only if "operation_code" is 2.

### `LEDButton<DSN>/result` topic

<pre><code>
{
	"operaion_code":&#60Integer&#62,
	"operation_result":&#60bool&#62,
	"result_message":"&#60String&#62"
}
</code></pre>

## Control LED by pressed boot button

1. User press boot button.  
2. Updated `Thing Shadow` in the AWS IoT.  
3. Changed led status, only if `Thing Shadow` update success.  
4. Published `LEDButton<DSN>/result` topic to report result to `Android app client`. 

## Control LED by Android app client

1. `Thing`(this project) subscribe `LEDButton<DSN>/command` topic.  
2. `Android app client` publish `LEDButton<DSN>/command` topic("operation_code" is 0(LED OFF) or 1(LED ON) at this time).  
3. `Thing` update `Thing Shadow` in the AWS IoT according to command, then change LED status, only if `Thing Shadow` update success.  
4. `Thing` publish `LEDButton<DSN>/result` topic to report result to Android app client.  

## Change operation key proccess by Android app client

1. `Thing`(this project) subscribe `LEDButton<DSN>/command` topic.  
2. `Android app client` publish `LEDButton<DSN>/command` topic("operation_code" is 2(Change operation key) at this time) with new operation key to the "operation_data".  
3. `Thing` change operation key saved in non-volatile storage(nvs), then publish `LEDButton<DSN>/result` topic to report result to Android app client, only if nvs operaion is done successfully.

### As mentioned earlier, [this project does not subscribe `delta`](https://github.com/JoonDong2/AWS/tree/master/esp32_led_button#this-project-does-not-subscribe-delta-of-thing-shadow-for-receiving-command-from-android-app-client-because-operation-key-is-included-in-the-command-unlike-thing_shadow-example-if-operation-key-is-included-in-the-thing-shadow-it-may-be-subscribed-or-published-by-any-clients).

### All of the operations are executed only when "operation_key" sent from `Android app client` is matched with operation key saved in the `Thing`(this project).

## Policy in the AWS IoT

<pre><code>
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:&#60;account id&#62;:client/LEDButton12345670"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": [
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/LEDButton12345670/command",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/update/accepted",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/update/rejected",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/get/accepted",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/get/rejected"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topicfilter/LEDButton12345670/command",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topicfilter/$aws/things/LEDButton12345670/shadow/update/accepted",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topicfilter/$aws/things/LEDButton12345670/shadow/update/rejected",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topicfilter/$aws/things/LEDButton12345670/shadow/get/accepted",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topicfilter/$aws/things/LEDButton12345670/shadow/get/rejected"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/LEDButton12345670/result",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/update",
        "arn:aws:iot:us-east-1:&#60;account id&#62;:topic/$aws/things/LEDButton12345670/shadow/get"
      ]
    }
  ]
}
</code></pre>
