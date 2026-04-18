/*******************************************************************************
Created by profi-max (Oleg Linnik) 2024
https://profimaxblog.ru
https://github.com/profi-max

*******************************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SoftwareSerial.h"

// modbus defines
#define MB_PORT 502
#define MB_BUFFER_SIZE 256
#define MB_RTU_SLAVE_RESPONSE_TIMEOUT_MS 1000

#define  MB_FC_NONE                     0
#define  MB_FC_READ_REGISTERS           3
#define  MB_FC_WRITE_REGISTER           6
#define  MB_FC_WRITE_MULTIPLE_REGISTERS 16
#define  MB_FC_ERROR_MASK 				128

//  user defines
#define MB_UART_RX_PIN 13
#define MB_UART_TX_PIN 15

// uncomment the line below if you need debug via serial
//#define MB_DEBUG

WiFiServer mbServer(MB_PORT);
WiFiClient client;
SoftwareSerial swSerial;
uint8_t mbByteArray[MB_BUFFER_SIZE]; // send and recieve buffer
uint8_t tcp_req[9];
bool wait_for_rtu_response = false;
uint32_t timeStamp;


/*=================================================================================================*/
static const char aucCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40
};

static const char aucCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40
};

/*=================================================================================================*/
/* Calculate CRC16 */
uint16_t crc16( uint8_t * pucFrame, uint16_t usLen )
{
    uint8_t           ucCRCHi = 0xFF;
    uint8_t           ucCRCLo = 0xFF;
    int             iIndex;

    while( usLen-- )
    {
        iIndex = ucCRCLo ^ *( pucFrame++ );
        ucCRCLo = ( uint8_t )( ucCRCHi ^ aucCRCHi[iIndex] );
        ucCRCHi = aucCRCLo[iIndex];
    }
    return ( uint16_t )( ucCRCHi << 8 | ucCRCLo );
}

/*=================================================================================================*/
void mb_debug(String str, uint16_t start, uint16_t len)
{
#ifdef MB_DEBUG
	Serial.print(str);
	for (uint16_t i = 0; i < len; i++) 
	{
		Serial.printf(" %X", mbByteArray[i + start]);
	}
	Serial.println("");
#endif
}

/*=================================================================================================*/
/* TCP client poll, if TCP request received -> send RTU request */
void modbusTcpServerTask(void)
{
  	uint16_t crc;
  	uint8_t mb_func = MB_FC_NONE;
  	uint16_t len = 0;
  
	if (mbServer.hasClient()) // Check if there is new client
	{
		// if client is free - connect
		if (!client || !client.connected()){
			if(client) client.stop();
			client = mbServer.accept(); //.available();
			
		// client is not free - reject
		} 
		else 
		{
			WiFiClient serverClient = mbServer.accept(); //.available();
			serverClient.stop();
		}
	}

  	//-------------------- Read from socket --------------------
  	if (client && client.connected() && client.available())
  	{
		delay(1);
		uint16_t bytesReady;
		while((bytesReady = client.available()) && (len < MB_BUFFER_SIZE))
		{
			len += client.readBytes(&mbByteArray[len], bytesReady);
		}
		if (len > 8)  mb_func = mbByteArray[7];  //Byte 7 of request is FC
		mb_debug("TCP RX:", 0, len);
	}
	else 
	{
		return;
	}	
        //-------------------- Read Registers (3) --------------------
        //-------------------- Write Register (6) --------------------
    if ((mb_func == MB_FC_READ_REGISTERS) || (mb_func == MB_FC_WRITE_REGISTER) ) 
	{
		crc =  crc16(&mbByteArray[6], 6);
		mbByteArray[12] = lowByte(crc);
		mbByteArray[13] = highByte(crc);
		swSerial.write(&mbByteArray[6], 8);
		swSerial.flush();
		timeStamp = millis();
		memcpy(tcp_req, mbByteArray, sizeof(tcp_req));
		wait_for_rtu_response = true;
		mb_debug("RTU TX:", 6, 8);
    }
		//-------------------- Write Multiple Registers (16) --------------------
	else if (mb_func == MB_FC_WRITE_MULTIPLE_REGISTERS) 
	{
		uint8_t len = mbByteArray[12] + 7;
		crc =  crc16(&mbByteArray[6], len);
		mbByteArray[len + 6] = lowByte(crc);
		mbByteArray[len + 7] = highByte(crc);
		swSerial.write(&mbByteArray[6], len + 2);
		swSerial.flush();
		timeStamp = millis();
		memcpy(tcp_req, mbByteArray, sizeof(tcp_req));
		wait_for_rtu_response = true;
		mb_debug("RTU TX:", 6, len + 2);
	}
}
/*=================================================================================================*/
/* RTU master poll, if RTU response received -> send TCP response */
void modbusRtuMasterTask(void)
{
	uint16_t len = 0;
	bool timeout = true;
	bool crc_err = false;
  	uint8_t mb_func = MB_FC_NONE;

	digitalWrite(LED_BUILTIN, HIGH); // LED off
	while(millis() - timeStamp < MB_RTU_SLAVE_RESPONSE_TIMEOUT_MS)
	{
		uint16_t bytesReady = swSerial.available();
		if (bytesReady > 0)
		{
			len += swSerial.readBytes(&mbByteArray[6 + len], bytesReady);
			mb_func = mbByteArray[7];
			if ((len >= 5) && (mb_func >= MB_FC_ERROR_MASK)) // error code 
				{timeout = false; crc_err = crc16(&mbByteArray[6], 5) != 0; break;} 
			if ((len >= 8) && ((mb_func == MB_FC_WRITE_MULTIPLE_REGISTERS) || (mb_func == MB_FC_WRITE_REGISTER))) 
				{timeout = false; crc_err = crc16(&mbByteArray[6], 8) != 0; break;} 
			if ((len >= 7) && (mb_func == MB_FC_READ_REGISTERS) && (len >= mbByteArray[8] + 3 + 2)) 
				{timeout = false; crc_err = crc16(&mbByteArray[6], mbByteArray[8] + 3 + 2) != 0; break;} 
		}
	}

	if (timeout)
	{
		mb_debug("RTU RX timeout", 6, 0);
		tcp_req[5] = 3; // rest bytes
		tcp_req[7] |= MB_FC_ERROR_MASK;
		tcp_req[8] = 0x04; // error code
		if (client && client.connected()) 
		{
			client.write(tcp_req, 9);
			client.flush();
		}
	}
	else if (crc_err)
	{
		mb_debug("RTU RX:", 6, len);
		tcp_req[5] = 3; // rest bytes
		tcp_req[7] |= MB_FC_ERROR_MASK;
		tcp_req[8] = 0x08; // error code
		if (client && client.connected()) 
		{
			client.write(tcp_req, 9);
			client.flush();
		}
	}
	else
	{
		mb_debug("RTU RX:", 6, len);
		mbByteArray[4] = 0;
		if (mb_func >= MB_FC_ERROR_MASK)
		{
			mbByteArray[5] = 3; // rest bytes
			if (client && client.connected()) 
			{
				client.write(mbByteArray, 9);
				client.flush();
			}
		}
        	//-------------------- Response Write Register (6) --------------------
			//-------------------- Response Write Multiple Registers (16) --------------------
		if ((mb_func == MB_FC_WRITE_MULTIPLE_REGISTERS) || (mb_func == MB_FC_WRITE_REGISTER))
		{
			mbByteArray[5] = 6; // rest bytes
			if (client && client.connected()) 
			{
				client.write(mbByteArray, 12);
				client.flush();
				mb_debug("TCP TX:", 0, 12);
			}
		}
        	//-------------------- Response Read Registers (3) --------------------
		if (mb_func == MB_FC_READ_REGISTERS) 
		{
			mbByteArray[5] = mbByteArray[8] + 3; // rest bytes
			if (client && client.connected()) 
			{
				client.write(mbByteArray, mbByteArray[8] + 3 + 6);
				client.flush();
				mb_debug("TCP TX:", 0, mbByteArray[8] + 3 + 6);
			}
		}
	}
	wait_for_rtu_response = false;
	digitalWrite(LED_BUILTIN, LOW); // LED on
}

/*=================================================================================================*/
void setup()
{
  	swSerial.begin(9600, SWSERIAL_8N1, MB_UART_RX_PIN, MB_UART_TX_PIN);
  	pinMode(LED_BUILTIN, OUTPUT);
  	digitalWrite(LED_BUILTIN, HIGH); // LED off

	Serial.begin(115200);
  	WiFi.begin("ssidname","password");   // start etehrnet interface
  	while (WiFi.status() != WL_CONNECTED) {
    	delay(1000);
    	Serial.print(".");
  	}
  	Serial.println("");
  	Serial.print("IP address: ");
  	Serial.println(WiFi.localIP()); // print your local IP address:
  	mbServer.begin();
  	mbServer.setNoDelay(true);
  	digitalWrite(LED_BUILTIN, LOW); // LED on
}

/*=================================================================================================*/
void loop()
{
	modbusTcpServerTask();
	if (wait_for_rtu_response) 	modbusRtuMasterTask();		
}

/*=================================================================================================*/
