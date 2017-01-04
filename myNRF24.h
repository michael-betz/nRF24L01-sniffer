/*
 * myNRF24.c
 *
 *  Created on: May 8, 2014
 *      Author: michael
 */

#ifndef MYNRF24_C_
#define MYNRF24_C_
#include <inttypes.h>

extern uint8_t cacheCONFIG;
#define NRF_PWR_UP()   { SBI( cacheCONFIG, PWR_UP);  nRfWrite_register( CONFIG, cacheCONFIG ); }
#define NRF_PWR_DOWN() { CBI( cacheCONFIG, PWR_UP);  nRfWrite_register( CONFIG, cacheCONFIG ); }
#define NRF_RX_MODE()  { SBI( cacheCONFIG, PRIM_RX); nRfWrite_register( CONFIG, cacheCONFIG ); }
#define NRF_TX_MODE()  { CBI( cacheCONFIG, PRIM_RX); nRfWrite_register( CONFIG, cacheCONFIG ); }

void nRfInit();
void nRfInitProm(  uint8_t addrWidth, uint8_t channel, uint8_t is2mbpsRate );
void nRfInitTX();
void nRfInitRX();
void nRfSendBytes( uint8_t *bytesToSend, uint8_t len, uint8_t *adr,  uint8_t noAck );
void nRfSendAccPayload( uint8_t *bytesToSend, uint8_t len, uint8_t pipeNumber );
uint8_t nRfIsDataSent();
uint8_t nRfIsDataReceived();
uint8_t nRfGetRetransmits();
uint8_t nRfIsRXempty();
uint8_t nRfIsTXempty();
void nRfHandleISR();

void nRfHexdump( void );
void hexdump( uint8_t *data, uint16_t len );

#endif /* MYNRF24_C_ */
