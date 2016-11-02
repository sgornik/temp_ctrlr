#include <msp430.h>
#include "onewire.h"
#include "delay.h"

uint8_t CS = BIT5;

uint8_t turnOn[2] = {0x0C, 0x01};
uint8_t medInt[2] = {0x0A, 0x0A};
uint8_t dispTest[2] = {0x0F, 0x01};
uint8_t normalOp[2] = {0x0F, 0x00};
uint8_t noDecode[2] = {0x09, 0x00};

uint8_t DispAllDgts[2] = {0x0B, 0x07};

//uint8_t numberFive[8] = {60, 4, 4, 60, 32, 32, 32, 60};
//uint8_t numberTwo[8] = {60, 32, 32, 60, 4, 4, 4, 60};
uint8_t heart[8] = {195, 165, 153, 129, 129, 66, 36, 24};
uint8_t error[8] = {};

uint8_t nums[10][8] = {
		{60, 36, 36, 36, 36, 36, 36, 60}, // Number 0
		{16, 24, 20, 16, 16, 16, 16, 124}, // Number 1
		{60, 32, 32, 60, 4, 4, 4, 60}, // Number 2
		{60, 32, 32, 60, 32, 32, 32, 60}, // Number 3
		{48, 40, 36, 34, 34, 254, 32, 32}, // Number 4
		{60, 4, 4, 60, 32, 32, 32, 60}, // Number 5
		{60, 4, 4, 4, 60, 36, 36, 60}, // Number 6
		{60, 32, 32, 32, 120, 32, 32, 32}, // Number 7
		{60, 36, 36, 60, 36, 36, 36, 60}, // Number 8
		{60, 36, 36, 36, 60, 32, 32, 60} // Number 9
};

void SPISendData(uint8_t* data, uint8_t length);
void writeMatrix(uint8_t* data);

int main()
{
  onewire_t ow;
  int i;
  uint8_t scratchpad[9];
  uint32_t temp;
  uint8_t num_bytes_to_read = 9;
  uint8_t temp_c;
  uint8_t temp_c_lower;
  float temp_f;

  WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
  BCSCTL1 = CALBC1_8MHZ;
  DCOCTL = CALDCO_8MHZ;

  ow.port_out = &P1OUT;
  ow.port_in = &P1IN;
  ow.port_ren = &P1REN;
  ow.port_dir = &P1DIR;
  ow.pin = BIT7;

  P1OUT |= CS;
  P1DIR |= CS;
  P1SEL = BIT1 | BIT2 | BIT4;
  P1SEL2 = BIT1 | BIT2 | BIT4;

  UCA0CTL1 = UCSWRST;
  UCA0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC; // 3-pin, 8-bit SPI master
  UCA0CTL1 |= UCSSEL_2; // SMCLK
  UCA0BR0 |= 0x02; // /2
  UCA0BR1 = 0; //
  UCA0MCTL = 0; // No modulation
  UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**

  SPISendData(turnOn, 2);
  SPISendData(medInt, 2);
  SPISendData(dispTest, 2);
  SPISendData(normalOp, 2);
  SPISendData(noDecode, 2);

  writeMatrix(heart);

  while (1)
  {
	  onewire_reset(&ow);
	  onewire_write_byte(&ow, 0xcc); // skip ROM command
	  onewire_write_byte(&ow, 0x44); // convert T command
	  onewire_line_high(&ow);
	  DELAY_MS(800); // at least 750 ms for the default 12-bit resolution
	  onewire_reset(&ow);
	  onewire_write_byte(&ow, 0xcc); // skip ROM command
	  onewire_write_byte(&ow, 0xbe); // read scratchpad command
	  for (i = 0; i < num_bytes_to_read; i++) scratchpad[i] = onewire_read_byte(&ow);

	  temp = (uint32_t)(
			  ( ((uint32_t)(scratchpad[1] << 8)) & 0xFF00 ) |
			  ( ((uint32_t)(scratchpad[0] << 0)) & 0xFF )
			  );

	  temp_f = (float)temp/16;
	  temp_c = temp / 16;

	  // Only display lower unit for temperature
	  temp_c_lower = temp_c % 10;
	  writeMatrix(nums[temp_c_lower]);

  }

  _BIS_SR(LPM0_bits + GIE);
  return 0;
}

void writeMatrix(uint8_t* data)
{
	int i;
	uint8_t spiData[2];

	for (i = 0;i<8;i++)
	{
		spiData[0] = i+1;
		spiData[1] = data[i];
		SPISendData(spiData, 2);
	}
}

void SPISendData(uint8_t* data, uint8_t length)
{
	int i;
	volatile char received_ch = 0;

	P1OUT &= (~CS); // Select Device
	for (i = 0;i<length;i++)
	{
		while (!(IFG2 & UCA0TXIFG)); // USCI_A0 TX buffer ready?
		UCA0TXBUF = data[i]; // Send data over SPI to Slave
		while (!(IFG2 & UCA0RXIFG)); // USCI_A0 RX Received?
		received_ch = UCA0RXBUF; // Store received data
	}
	P1OUT |= (CS); // Unselect Device
}
