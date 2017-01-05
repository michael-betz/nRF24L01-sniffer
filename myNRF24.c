/*
 * myNRF24.c
 *
 *  Created on: May 8, 2014
 *      Author: michael
 */

#include <inttypes.h>
#include <stdio.h>
#ifdef __AVR_ATmega328P__   //AVR version
    #define    AVR_VERSION
    #include "mySPI_avr.h"
    #include <util/delay.h>
    #include "rprintf.h"
#else                       //Raspi version
    #define    RASPI_VERSION
    #include "mySPI_raspi.h"
    #include <bcm2835.h>    //Raspi GPIO library
#endif
#include "nRF24L01.h"
#include "myNRF24.h"

uint8_t cacheCONFIG;        //We cache the config register as global variable, so it does not need to be read over SPI each time it is changed

void nRfInit(){    //Init with default config
//    const uint8_t rxTxAdr[5] = { 0xE1, 0xE7, 0xE7, 0xE7, 0xE7 };
    initSPI();
    NRF_CE_OFF();
    NRF_CHIP_DESELECT();
    _delay_ms( 5 );
    cacheCONFIG = 0b00001100;                       //3xIRQs on, 16bit CRC, PDown, PTX mode
    nRfWrite_register( CONFIG,      cacheCONFIG );
    nRfWrite_register( EN_AA,       0b00111111 );   //Enable auto ACK on pipe0 - pipe5
    nRfWrite_register( EN_RXADDR,   0b00000001 );   //Only enable data pipe ERX_P0 for the start
    nRfWrite_register( SETUP_AW,    0b00000011);    //5 byte address width
    nRfWrite_register( SETUP_RETR,  0x3F );         //Automatic retransmit up to 15 times with delay of 1000 us
    nRfWrite_register( RF_CH,       64 );           //Set RF channel to 64
    nRfWrite_register( RF_SETUP,    0b00001111 );   //2 Mbps data rate, 0dBm power, LNA_HCURR=1
    //nRfWrite_register( RX_PW_P0,   6 );           //6 byte static RX payload length (only needed for RX when dyn payload is off)
    nRfWrite_register( DYNPD,       0b00111111 );   //Enable dynamic payload length on all pipes
    nRfWrite_register( FEATURE,     0b00000111 );   //Enable features: Dynamic payload length, Ack packets with payload, Disabled Ack for some packets
}

//Init for Promiscuous reception 
//(Everything which matches the addrLength MSBs of the 5 byte address)
void nRfInitProm(  uint8_t addrWidth, uint8_t channel, uint8_t is2mbpsRate ){
    uint8_t temp, aw_value = addrWidth - 2;         // datasheet says awValue: 0=illegal, 1=3bytes, 2=4bytes, 3=5bytes
    if ( aw_value>3 ){
        rprintf("error: addrWidth must be 2, 3, 4 or 5!\n");
        return;
    }
    initSPI();                              
    NRF_CE_OFF();
    NRF_CHIP_DESELECT();
    _delay_ms( 5 );
    cacheCONFIG = 0b01110001;                    //3xIRQs OFF, No CRC, Power Down, PRX mode
    nRfWrite_register( CONFIG,    cacheCONFIG );
    nRfWrite_register( EN_AA,     0x00 );        //Disable auto ACK on pipe0 - pipe5
    nRfWrite_register( EN_RXADDR, 0b00000001 );  //Enable ERX_P0 pipe
    nRfWrite_register( SETUP_AW,  aw_value );    //n bytes address width !
    nRfWrite_register( SETUP_RETR,0x00 );        //Automatic retransmit disabled
    nRfWrite_register( RF_CH,     channel );     //Set RF channel to x
    temp = 0b00000111;
    if( is2mbpsRate ){
        SBI( temp, 3 );
    }   
    nRfWrite_register( RF_SETUP,    temp );      //1 or 2 Mbps data rate, 0dBm power, LNA_HCURR=1
    nRfWrite_register( RX_PW_P0,    32 );        //32 byte static RX payload length
    nRfWrite_register( DYNPD,        0x00 );     //Disable dynamic payload length on all pipes
    nRfWrite_register( FEATURE,     0x00 );      //Disable: Dynamic payload length, Ack payload, Dynamic noack
    NRF_PWR_UP();
    _delay_ms( 3 );                              //Wait for Powerup
    NRF_RX_MODE();
    NRF_CE_ON();
}

void nRfInitTX(){    //Init for sleeping and sending on demand
    nRfInit();
}

void nRfInitRX(){    //Init for continuously receiving data
    nRfInit();
    NRF_PWR_UP();
    _delay_ms( 3 );  //Wait for Powerup
    NRF_RX_MODE();
    NRF_CE_ON();
}

// The RX module monitors the air for packets which match its address
// This is done for 6 data pipes in parallel, which have individual settings
// pipeNumber: ERX_P0 - ERX_P5
void nRfSetupRXPipe( uint8_t pipeNumber, uint8_t *rxAddr ){
    if( pipeNumber <= 5){
//        Enable the pipe
//        ----------------
        uint8_t temp = nRfRead_register( EN_RXADDR );    //We have to do a read modify write
        SBI( temp, pipeNumber);
        nRfWrite_register( EN_RXADDR, temp );            //Enable the data pipe
//        Set the pipe address
//        --------------------
        if( pipeNumber <= 1 ){                           //First 2 pipes got a 5 byte address
            nRfWrite_registers( RX_ADDR_P0+pipeNumber, rxAddr, 5 );
        } else {                                         //The other 4 pipes got a 1 byte address
            nRfWrite_registers( RX_ADDR_P0+pipeNumber, rxAddr, 1 );
        }
    }
}

// Transfers data from bytesToSend to the TX FIFO
// len = 1 ... 32
// if noAck == 1 then the sender will not wait for an ack packet, even if ack is enabled
// adr = 5 byte destination address
void nRfSendBytes( uint8_t *bytesToSend, uint8_t len, uint8_t *adr, uint8_t noAck ){
    uint8_t cacheRX0[5], cacheEnRx;                      //We cache address of RX pipe 0, which will be overiden when doing a TX with acc
    uint8_t nTries, tempStatus;
    NRF_CE_OFF();
    NRF_TX_MODE();
    nRfFlush_tx();
    nRfWrite_register( STATUS, (1<<MAX_RT)|(1<<TX_DS) ); //Clear Int flags
    cacheEnRx = nRfRead_register( EN_RXADDR );           //Cache enabled pipes
    nRfWrite_register(  EN_RXADDR, 0x01 );               //Disable all pipes except RX0 to avoid address conflicts
    nRfRead_registers(  RX_ADDR_P0, cacheRX0, 5 );       //Cache RXPipe0 address (will be overriden)
    nRfWrite_registers( RX_ADDR_P0, adr,      5 );       //Set RXPipe0 address to capture the Ack. response
    nRfWrite_registers( TX_ADDR,    adr,      5 );       //Set transmit pipe address
    if( noAck ){
        nRfWrite_payload( bytesToSend, len, W_TX_PAYLOAD_NO_ACK );
    } else {
        nRfWrite_payload( bytesToSend, len, W_TX_PAYLOAD );
    }
    NRF_CE_ON();
    for ( nTries=0; nTries<=10; nTries++ ){
        _delay_ms( 2 );
        tempStatus = nRfGet_status();
        if( tempStatus & (1<<TX_DS) ){
            rprintf( " TX_ACK[%d] ", nRfGetRetransmits() );
            nRfWrite_register( STATUS, (1<<TX_DS) );     //Clear Data ready flag
            break;
        }
        if( tempStatus & (1<<MAX_RT) ){
            rprintf( " MAX_RT " );
            nRfWrite_register( STATUS, (1<<MAX_RT) );    //Clear MAX_RT flag
            break;
        }
    }
    NRF_CE_OFF();
    nRfWrite_registers( RX_ADDR_P0, cacheRX0,  5 );      //Restore RXPipe0 address
    nRfWrite_register( EN_RXADDR, cacheEnRx );           //Re-enable RX pipes
    NRF_RX_MODE();
    NRF_CE_ON();
}

// Transfers data from bytesToSend to the ACC payload FIFO
// Note that ACC payloads are only transmitted in RX mode
// len = 1 ... 32
void nRfSendAccPayload( uint8_t *bytesToSend, uint8_t len, uint8_t pipeNumber ){
    if( pipeNumber > 5 ){ return; }
    nRfWrite_payload( bytesToSend, len, (W_ACK_PAYLOAD|pipeNumber) );
}

uint8_t nRfIsDataSent(){
    return( nRfGet_status()&(1<<TX_DS) );                //Check interrupt flag
}

uint8_t nRfGetRetransmits(){
    return( nRfRead_register(OBSERVE_TX) & 0x0F );       //Returns number of restransmitted packets
}

uint8_t nRfIsDataReceived(){
    return( nRfGet_status()&(1<<RX_DR) );                //Check interrupt flag
}

uint8_t nRfIsRXempty(){
    return( nRfRead_register(FIFO_STATUS)&(1<<RX_EMPTY) );      //Check FIFO status
}

uint8_t nRfIsTXempty(){
    return( nRfRead_register(FIFO_STATUS)&(1<<TX_EMPTY) );      //Check FIFO status
}

uint8_t nRfgetPipeNo(){                                         //Pipe number of current RX fifo entry
    return( ( nRfGet_status() & 0b00001110 ) >> 1 );
}

//------------------------------------------------
// Debug functions
//------------------------------------------------
void nRfHexdump( void ){
    uint8_t x;
    rprintf( "All nRF24 registers:\n" );
    rprintf("0x00:  ");
    for( x=0; x<=0x1D; x++){
        rprintf( "%02x ", nRfRead_register(x) );
        if( ((x+1)%8) == 0 ){
            rprintf( "\n" );
            rprintf( "0x%02x:  ", x+1);
        }
    }
    rprintf("\n\n");
}

void hexdump( uint8_t *data, uint16_t len ){
    uint8_t x;
    rprintf("\n0x00:  ");
    for( x=0; x<len; x++){
        printf( "%02x ", *data++ );
        if( ((x+1)%8) == 0 ){
            rprintf( "\n" );
            rprintf( "0x%02x:  ", x+1);
        }
    }
    rprintf("\n\n");
}

