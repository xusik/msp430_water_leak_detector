#include "Energia.h"


//nRF24L01 - MSP430 Launchpad
//3 CE   - P2.0:
//4 CSN  - P2.1:
//5 SCK  - P1.5:
//6 MOSi - P1.7:
//7 MISO - P1.6:
//8 IRQ  - P2.2:
//
//
// 	                               +-\/-+
// 	                        VCC   1|    |20  GND
// 	                        P1.0  2|    |19  XIN
// 	                        P1.1  3|    |18  XOUT
// 	                        P1.2  4|    |17  TEST
// 	                 BUTTON P1.3  5|    |16  RST#
// 	                        P1.4  6|    |15  P1.7 MOSI  6
// 	                 5  SCK P1.5  7|    |14  P1.6 MISO  7
// 	                 3   CE P2.0  8|    |13  P2.5
// 	   	 	         4  CSN P2.1  9|    |12  P2.4
//		 	         8  IRQ P2.2 10|    |11  P2.3
// 			                       +----+
//
//

#include "Enrf24.h"
#include "nRF24L01.h"
#include <string.h>
#include <SPI.h>
#include "Energia.h"

#define _pinPushButton  PUSH2
#define LED RED_LED

Enrf24 radio(P2_0, P2_1, P2_2);  // P2.0=CE, P2.1=CSN, P2.2=IRQ
const uint8_t txaddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x01 };

volatile int push_flag = LOW;

const char *str_sens1 = "SENS1";
int button_now_pressed, msp_ps = 0;
unsigned long work_time = 0;

void power_savings ();
void ps_out();
uint16_t Msp430_GetSupplyVoltage(void);

uint16_t Msp430_GetSupplyVoltage(void)
{
	uint16_t raw_value;
	// first attempt - measure Vcc/2 with 1.5V reference (Vcc < 3V )
	ADC10CTL0 = SREF_1 | REFON | ADC10SHT_2 | ADC10SR | ADC10ON;
	ADC10CTL1 = INCH_11 | SHS_0 | ADC10DIV_0 | ADC10SSEL_0;
	// start conversion and wait for it
	ADC10CTL0 |= ENC | ADC10SC;
	while (ADC10CTL1 & ADC10BUSY) ;
	// stop conversion and turn off ADC
	ADC10CTL0 &= ~ENC;
	ADC10CTL0 &= ~(ADC10IFG | ADC10ON | REFON);
	raw_value = ADC10MEM;
	// check for overflow
	if (raw_value == 0x3ff) {
		// switch range - use 2.5V reference (Vcc >= 3V)
		ADC10CTL0 = SREF_1 | REF2_5V | REFON | ADC10SHT_2 | ADC10SR | ADC10ON;
		// start conversion and wait for it
		ADC10CTL0 |= ENC | ADC10SC;
		while (ADC10CTL1 & ADC10BUSY) ;
		raw_value = ADC10MEM;
		// end conversion and turn off ADC
		ADC10CTL0 &= ~ENC;
		ADC10CTL0 &= ~(ADC10IFG | ADC10ON | REFON);
		// convert value to mV
		return ((uint32_t)raw_value * 5000) / 1024;
	} else
		return ((uint32_t)raw_value * 3000) / 1024;
}

void setup() {

  pinMode(PUSH2, INPUT_PULLUP); //Enable internal pullup
  attachInterrupt(PUSH2, ps_out, RISING);
  pinMode(LED, OUTPUT);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  radio.begin();  // Defaults 1Mbps, channel 0, max TX power
  radio.setChannel (120);
  radio.setTXaddress((void*)txaddr);
}

void loop() {

	if (push_flag) {

		work_time = millis();

		int sensorValue = Msp430_GetSupplyVoltage ();

		  if (sensorValue < 2500){
		      digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
		  }else{
		      digitalWrite(LED, LOW);   // turn the LED off
		  }


		button_now_pressed = !digitalRead(PUSH2);

		delay(10);

		if (button_now_pressed + !digitalRead(PUSH2) == 2){
			delay(100);
			if (button_now_pressed + !digitalRead(PUSH2) == 2){

				    radio.print(str_sens1);
				    radio.flush();  //
				    delay(200);
			}
		}


		push_flag = LOW;
		button_now_pressed = 0;
	}
	if (!digitalRead(PUSH2) == HIGH){
		push_flag = HIGH;
	}

	msp_ps =  millis() - work_time;
	if (msp_ps > 500){ power_savings (); }
}

void ps_out(){
	push_flag = HIGH;
	if (stay_asleep == true){
		wakeup();
		SPI.begin();
		SPI.setDataMode(SPI_MODE0);
		SPI.setBitOrder(MSBFIRST);
		radio.begin();  // Defaults 1Mbps, channel 0, max TX power
		radio.setChannel (120);
		radio.setTXaddress((void*)txaddr);
	}
}

void power_savings (){
	attachInterrupt(PUSH2, ps_out, FALLING);
	radio.deepsleep();
	suspend();
}


