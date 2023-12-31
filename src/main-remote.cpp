#ifdef REMOTE
#include <math.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"
static const char *TAG = "MAIN";
//------------ turn on generic serial printing

#define DEBUG_PRINTS
// data that will be sent to the receiver

typedef struct
{
	int16_t speedmotorLeft;
	int16_t speedmotorRight;
	int16_t speedmotorArm;
	int16_t packetArg2;
	int16_t packetArg3;
} packet_t;

packet_t sentData;
packet_t recData;

//---------------------------------------ESP_NOW Variables

String success;
esp_now_peer_info_t peerInfo;
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	if (status == 0)
	{
		success = "Delivery Success :)";
	}
	else
	{
		success = "Delivery Fail :(";
	}
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	memcpy(&recData, incomingData, sizeof(recData));
}

//---------------------------------------HARDWARE DEPENDANT Variables
// one ifdef case per hardware to speed up modularity of the code

// RAC standard remote
const int steerPot = 7;
const int accPot = 10;
const int leverPot = 8;
const int trimPot = 39;

const int rightBtn = 2;
const int leftBtn = 4;
const int topBtn = 5;
// const int lowSwitch = 32;
// const int topSwitch = 25;
LedUtility Led(21);

// customisable vars
int analogRes = 10;
int analogReadMax = (1 << analogRes) - 1;

// variables for the sketch
int leverValue = 0;

unsigned long current_time = 0;

void setup()
{
	// store_values(); // uncomment only to initialize mem
	analogReadResolution(analogRes);
	analogSetAttenuation(ADC_11db);
	pinMode(rightBtn, INPUT_PULLUP);
	pinMode(leftBtn, INPUT_PULLUP);
	pinMode(topBtn, INPUT_PULLUP);
	// pinMode(lowSwitch, INPUT_PULLUP);
	// pinMode(topSwitch, INPUT_PULLUP);
	Led.init();
	Led.setBlinks(1, 150);
	delay(2000);
#ifdef DEBUG_PRINTS
	Serial.begin(115200);
	Serial.println("RAC GENERIC BOT");
#endif

	//---------------------------------------ESP NOW setup
	WiFi.mode(WIFI_STA);
	esp_wifi_set_channel(2, WIFI_SECOND_CHAN_NONE);
	if (esp_now_init() != ESP_OK)
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_send_cb(OnDataSent);
	memcpy(peerInfo.peer_addr, robotAddress, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	if (esp_now_add_peer(&peerInfo) != ESP_OK)
	{
		Serial.println("Failed to add peer");
		return;
	}
	char macStr[18];
	Serial.print("Packet from: ");
	// Copies the sender mac address to a string
	snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
			 peerInfo.peer_addr[0], peerInfo.peer_addr[1], peerInfo.peer_addr[2], peerInfo.peer_addr[3], peerInfo.peer_addr[4], peerInfo.peer_addr[5]);
	Serial.print("sending to: ");
	Serial.println(macStr);
	esp_now_register_recv_cb(OnDataRecv);
	Led.setBlinks(0);
	Led.ledOn();
}

void loop()
{
	// read pots values
	int strValue = analogRead(steerPot);
	delay(3);
	int accValue = analogRead(accPot);
	delay(3);
	int leverValue = analogRead(leverPot);
	delay(3);
	current_time = millis();
	bool rightValue = !digitalRead(rightBtn);
	bool leftValue = !digitalRead(leftBtn);
	bool topValue = !digitalRead(topBtn);

	// vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

	//!!!!!!!!!!! BUTTON VARIABLES !!!!!!!!!!\\

	int topButton = 0;
	int rightButton = 0;
	int leftButton = 0;

	//!!!!!!!!!!! DEAD ZONE RANGE !!!!!!!!!\\

	const int deadZoneMax_X = 160;
	const int deadZoneMin_X = -160;

	const int deadZoneMax_Y = 160;
	const int deadZoneMin_Y = -160;

	const int deadZoneMax_L = 610;
	const int deadZoneMin_L = 450;

		 //!!!!!!!!!!! BUTTONS !!!!!!!!!!\\

	//  RIGHT BUTTON
		if (rightValue)
		{
			// Button is pressed, Devastator puts arm up
			sentData.speedmotorArm = 400;
		}
	
	// LEFT BUTTON	
		if (leftButton)
		{
			// Button is pressed, devastator puts arm full back
			sentData.speedmotorArm = 0;
		}

	// TOP BUTTON
		if (topValue)
		{
			// Button is pressed,  Devastator goes backward
			sentData.speedmotorRight = -512;
			sentData.speedmotorLeft = -512;
			
		}
	


	//!!!!!!!!!!! ANALOG !!!!!!!!!!\\

	int L_Axis = leverValue;
	int x_Axis = strValue;
	int y_Axis = accValue;

	L_Axis = map(L_Axis, 330, 500, 0, 870);
	L_Axis = constrain(L_Axis, 0, 870);
	x_Axis = map(x_Axis, 0, 1023, 1023, -1023);
	x_Axis = constrain(x_Axis, -600, 600);
	y_Axis = map(y_Axis, 0, 1023, 1023, -1023);
	// y_Axis = constrain(y_Axis, -512, 512);

	if (x_Axis >= deadZoneMin_X && x_Axis <= deadZoneMax_X)
	{
		x_Axis = 0; //+ Set idle position to 0 +
	}
	if (y_Axis >= deadZoneMin_Y && y_Axis <= deadZoneMax_Y)
	{
		y_Axis = 0; //+ Set idle position to 0 +
	} 
	
	// todo: make motors go ahead and backward
	
	sentData.speedmotorLeft = y_Axis + x_Axis;
	sentData.speedmotorRight = x_Axis - y_Axis;
	sentData.speedmotorArm = L_Axis;
	

	// + ------------------------------ +
	/*Serial.println("--------------------");
	Serial.print("x_axis: ");
	Serial.println(x_Axis);
	Serial.print("y_axis: ");
	Serial.println(y_Axis);
	Serial.print("right motor: ");
	Serial.println(sentData.speedmotorRight);
	Serial.print("left motor: ");
	Serial.println(sentData.speedmotorLeft);
	Serial.print("x ");
	Serial.println(x_Axis);
	Serial.print("\t");
	Serial.print("y ");
	Serial.println(y_Axis);
	Serial.print("RIGHT BUTTON: ");
	Serial.println(rightButton);
	Serial.print("LEFT BUTTON: ");
	Serial.println(leftButton);
	Serial.print("TOP BUTTON: ");
	Serial.println(topButton);
	Serial.print("LEVA Value: ");
	Serial.println(L_Axis);
	Serial.println("--------------------");*/
	Serial.print("LEVA Value: ");
	Serial.print(leverValue);
	Serial.print("\t");
	Serial.print("l_axis: ");
	Serial.println(L_Axis);

	// ! -------------------------------------------- //

	esp_err_t result = -1;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	if (result == ESP_OK)
	{
		// Serial.println("Sent with success");
	}
	else
	{
		// Serial.println("Error sending the data");
	}
	delay(10);
}
#endif