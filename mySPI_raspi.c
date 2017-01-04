#ifndef __AVR_ATmega328P__   //Ignore on AVR version

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bcm2835.h>    //Raspi GPIO library
#include "nRF24L01.h"
#include "mySPI_raspi.h"

//------------------------------------------------------------------------------
// Private stuff
//------------------------------------------------------------------------------
char memoryBuffer[255];

uint8_t do_SPItransfer( uint8_t len ) {
  bcm2835_spi_transfern( memoryBuffer, len+1 );
  return( memoryBuffer[0] );                    //Return status byte
}

// Copy content of internal memory buffer to target, omitting the first byte, which is the SPI command
void copySansCommand( uint8_t *target, uint8_t len ){
    memcpy( target, &memoryBuffer[1], len );
}

void fillWriteBuffer( const uint8_t *source, uint8_t len ){
    memcpy( &memoryBuffer[1], source, len );
}

//------------------------------------------------------------------------------
// Public stuff
//------------------------------------------------------------------------------

void initSPI() {
    // Initialize pins
    bcm2835_gpio_fsel(  GPIO_RF24_CE,  BCM2835_GPIO_FSEL_OUTP );    //Transceiver enable pin
    bcm2835_gpio_fsel(  GPIO_RF24_CSN, BCM2835_GPIO_FSEL_OUTP );    //SPI /ChipSelect pin
    bcm2835_gpio_write( GPIO_RF24_CE,  LOW );
    bcm2835_gpio_write( GPIO_RF24_CSN, HIGH );

    // Initialize SPI bus
    bcm2835_spi_begin( );
    bcm2835_spi_setBitOrder( BCM2835_SPI_BIT_ORDER_MSBFIRST );      // The default
    bcm2835_spi_setDataMode( BCM2835_SPI_MODE0 );                   // The default
    bcm2835_spi_setClockDivider( BCM2835_SPI_CLOCK_DIVIDER_64 );    // 250 MHz / 64 = 4 MHz
    bcm2835_spi_chipSelect( BCM2835_SPI_CS0 );                      // The default
    bcm2835_spi_setChipSelectPolarity( BCM2835_SPI_CS0, LOW );      // the default
}

uint8_t nRfRead_registers( uint8_t reg, uint8_t* buf, uint8_t len ) {
    memoryBuffer[0] = R_REGISTER | ( REGISTER_MASK & reg );
    uint8_t status = do_SPItransfer( len );
    copySansCommand( buf, len );
    return status;
}

uint8_t nRfRead_register( uint8_t reg ) {
  memoryBuffer[0] = R_REGISTER | ( REGISTER_MASK & reg );
  do_SPItransfer( 1 );
  return memoryBuffer[1];
}

uint8_t nRfWrite_registers(uint8_t reg, const uint8_t* buf, uint8_t len) {
  memoryBuffer[0] = W_REGISTER | ( REGISTER_MASK & reg );
  fillWriteBuffer( buf, len );
  return do_SPItransfer( len );
}

uint8_t nRfWrite_register(uint8_t reg, uint8_t value) {
  memoryBuffer[0] = W_REGISTER | ( REGISTER_MASK & reg );
  fillWriteBuffer( &value, 1 );
  return do_SPItransfer( 1 );
}

//Do a command like W_TX_PAYLOAD, return status
uint8_t nRfWrite_payload( const void* buf, uint8_t len, uint8_t command ) {
  const uint8_t* current = (const uint8_t*)(buf);
  memoryBuffer[0] = command;
  fillWriteBuffer( current, len );
  return do_SPItransfer( len );
}

uint8_t nRfRead_payload(void* buf, uint8_t len) {
  uint8_t* current = (uint8_t*)(buf);
  memoryBuffer[0] = R_RX_PAYLOAD;
  uint8_t status = do_SPItransfer( len );
  copySansCommand( current, len );
  return status;
}

uint8_t nRfFlush_rx(void){
  memoryBuffer[0] = FLUSH_RX;
  return do_SPItransfer( 0 );
}

uint8_t nRfFlush_tx(void){
  memoryBuffer[0] = FLUSH_TX;
  return do_SPItransfer( 0 );
}

uint8_t nRfGet_status(void){
  memoryBuffer[0] = NOP;
  return do_SPItransfer( 0 );
}

uint8_t nRfGet_RX_Payload_Width(void){
  uint8_t plw;
  memoryBuffer[0] = R_RX_PL_WID;
  do_SPItransfer( 1 );
  plw = memoryBuffer[1];
  if( plw > 32 ){   //Datasheet says: Flush RX FIFO if the read value is larger than 32 bytes.
    nRfFlush_rx();
    return 0;
  }
  return plw;
}

#endif
