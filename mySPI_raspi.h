/*
 Copyright (C) 2014 M. Betz

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file mySPI.h
 *
 * Helper class for SPI transfers
 */

#ifndef __AVR_ATmega328P__   //non AVR version

#ifndef __MYSPI_H__
#define __MYSPI_H__

//----------------------------------------------
// Defines for the nRF24 module driven on RasPi
//----------------------------------------------
// Note: These are the Broadcom GPIO pin numbers, as printed on the Raspi silkscreen
#define GPIO_RF24_CE    25      //Chip Transmit / Receive Enable
#define GPIO_RF24_CSN    8      //SPI chip select (inverted)

#define NRF_CHIP_SELECT()
#define NRF_CHIP_DESELECT()
#define NRF_CE_ON()         bcm2835_gpio_write( GPIO_RF24_CE,  HIGH )
#define NRF_CE_OFF()        bcm2835_gpio_write( GPIO_RF24_CE,  LOW )

#define rprintf( ... )      fprintf( stderr, __VA_ARGS__ )
#define _delay_ms( del )    bcm2835_delay( del )
#define _delay_us( del )    bcm2835_delayMicroseconds( del )

#define SBI(reg, bit) ( reg |=  ( 1 << bit ) )
#define CBI(reg, bit) ( reg &= ~( 1 << bit ) )
#define IBI(reg, bit) ( reg &   ( 1 << bit ) )


//-----------------------------------------------------------------------------
// My own SPI library, RasPi Style
//-----------------------------------------------------------------------------
// Command can be:
// R_REGISTER, W_REGISTER, ACTIVATE, R_RX_PL_WID, R_RX_PAYLOAD, W_TX_PAYLOAD,
// W_ACK_PAYLOAD, FLUSH_TX, FLUSH_RX, REUSE_TX_PL, NOP


//-----------------------------------------------------------------------------
// Private stuff
//-----------------------------------------------------------------------------
uint8_t do_SPItransfer( uint8_t len );
void copySansCommand( uint8_t *target, uint8_t len );
void fillWriteBuffer( const uint8_t *source, uint8_t len );

//-----------------------------------------------------------------------------
// Public stuff
//-----------------------------------------------------------------------------
void initSPI( void );   //Must be called before doing other stuff
uint8_t nRfRead_registers(uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t nRfRead_register( uint8_t reg );
uint8_t nRfWrite_registers( uint8_t reg, const uint8_t* buf, uint8_t len );
uint8_t nRfWrite_register(uint8_t reg, uint8_t value);
uint8_t nRfWrite_payload( const void* buf, uint8_t len, uint8_t command );
uint8_t nRfRead_payload( void* buf, uint8_t len );
uint8_t nRfFlush_rx(void);
uint8_t nRfFlush_tx(void);
uint8_t nRfGet_status(void);
uint8_t nRfGet_RX_Payload_Width(void);

#endif // __MYSPI_H__

#endif
