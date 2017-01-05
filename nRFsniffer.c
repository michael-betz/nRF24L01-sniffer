#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <bcm2835.h>       //Raspi GPIO library
#include "mySPI_raspi.h"
#include "myNRF24.h"
#include "nRF24L01.h"

#define RX_LENGTH 32       //nRF is configured to return fixed payload length of 32 byte

//--------------------------------------------------------------------------
// pcap file format headers
//--------------------------------------------------------------------------
static struct {
  uint32_t magic_number;   /* magic number */
  uint16_t version_major;  /* major version number */
  uint16_t version_minor;  /* minor version number */
  int32_t  thiszone;       /* GMT to local correction */
  uint32_t sigfigs;        /* accuracy of timestamps */
  uint32_t snaplen;        /* max length of captured packets, in octets */
  uint32_t network;        /* data link type */
} pcap_hdr = { 0xa1b2c3d4, 2, 4, 0, 0, 65535, 147 /*LINKTYPE_USER0*/ };

typedef struct _pcaprec_hdr {
  uint32_t ts_sec;         /* timestamp seconds */
  uint32_t ts_usec;        /* timestamp microseconds */
  uint32_t incl_len;       /* number of octets of packet saved in file */
  uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr;

//--------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------
int keepRunning = true;
void intHandler( int dummy ){
    fprintf(stderr, "\n... Shutting down! \n");
    keepRunning = false;
}

uint8_t hexCharToInt( char hexChar ){
    hexChar = tolower( hexChar );
    if( hexChar>='0' && hexChar<='9' ){
        return( hexChar - '0' );
    }
    if( hexChar>='a' && hexChar<='f' ){
        return( hexChar - 'a' + 10 );
    }
    return 0;
}

uint16_t bytesToHex( uint8_t *binaryIn, uint16_t size, char *stringOut ) {
    const char hex_str[]= "0123456789abcdef";
    uint16_t i;
    uint8_t currentByte;
    for (i=0; i<size; i++){
        currentByte = *binaryIn++;
        *stringOut++ = hex_str[ currentByte >> 4 ];
        *stringOut++ = hex_str[ currentByte & 0x0F ];
        *stringOut++ = ' ';
    }
    stringOut--;
    *stringOut = 0x00;
    return( size * 3 ); 
}

int main( int argc, char **argv ){ 
    uint8_t recBuffer[64], topic[255], message[255], msgLen, retVal, i;
    uint8_t rxAddrs[5];                         //User configurable RX address (2 - 5 byte long)
    uint8_t rxAddrLength, rfChannel, is2mbps;   //User arguments
    struct timeval tsCapture;
    FILE *outFile;
    
    outFile = stdout;
    fprintf(stderr, "----------------------\n");
    fprintf(stderr, " nRF24 RF sniffer\n");
    fprintf(stderr, "----------------------\n");
    fprintf(stderr, "%s <nRF_channel (0-127)> <nRF_mbps (1/2)> <adr_4> <adr_3> [ <adr_2> <adr_1> <adr_0> ] \n", argv[0] );
    fprintf(stderr, "Returns human readable stuff on stderr, pcap binary data on stdout.\n\n" );
    
    //--------------------------------------------------------------------------
    // 'Parse' cmd line arguments
    //--------------------------------------------------------------------------
    rxAddrLength = argc - 3;
    if( rxAddrLength < 2 || rxAddrLength > 5 ){
        fprintf(stderr, "Wrong number of arguments: %d \n", argc-1 );
        return 1;
    }
    rfChannel = strtol( argv[1], NULL, 0 );
    if( rfChannel > 127 ){
        fprintf(stderr, "nRF_channel must be <= 127 \n");
        return 1;
    }
    is2mbps = strtol( argv[2], NULL, 0 ) >= 2;
    fprintf(stderr, "!!! Listening to RF Ch. %d with %d Mbps for anyhing starting with Adr. 0x", rfChannel, is2mbps?2:1 );
    for ( i=0; i<rxAddrLength; i++ ){
        rxAddrs[rxAddrLength-1-i] = strtol( argv[3+i], NULL, 0 );
        fprintf(stderr, "%02X", rxAddrs[rxAddrLength-1-i] );
    }
    fprintf(stderr, " [%d bytes] !!!\n\n", rxAddrLength);
    
    //--------------------------------------------------------------------------
    // Initialize GPIO lib and CTRL+c handler
    //--------------------------------------------------------------------------
    pcaprec_hdr recHdr;
    signal(SIGINT, intHandler);
    if ( !bcm2835_init() ){
        fprintf(stderr, "Could not initialize bcm2835 library. Exiting!\n");
        return 1;
    }
    
    //--------------------------------------------------------------------------
    // Hardware Address Match length setting
    //--------------------------------------------------------------------------
    // The nRF will compare [2,3,4,5] upper bytes of the adr
    // any remaining bytes will be returned as payload
    // We listen to anything that ends with 3*[0xE7]
    nRfInitProm( rxAddrLength, rfChannel, is2mbps );
	nRfWrite_registers( RX_ADDR_P0, rxAddrs, rxAddrLength );//write the address to nRF register
    nRfFlush_tx();
    nRfFlush_rx();
    nRfWrite_register( STATUS, (1<<RX_DR) );		        //Clear Data ready flag
    fwrite( &pcap_hdr, sizeof(pcap_hdr), 1, outFile );
    nRfHexdump();
    
    while( keepRunning ){
        if( nRfIsDataReceived() ){
            while( !nRfIsRXempty() && keepRunning ){        //Readout and empty the RX FIFO
                gettimeofday( &tsCapture, NULL );
	            nRfRead_payload( recBuffer, RX_LENGTH );
                msgLen = bytesToHex( recBuffer, RX_LENGTH, message );
                fprintf(stderr, "(%4d) [%s]\n", tsCapture.tv_sec, message );
                recHdr.ts_sec = tsCapture.tv_sec;
                recHdr.ts_usec = tsCapture.tv_usec;
                recHdr.incl_len = RX_LENGTH;
                recHdr.orig_len = RX_LENGTH;
                fwrite( &recHdr, sizeof(recHdr), 1, outFile );
                fwrite( recBuffer, RX_LENGTH, 1, outFile );
                fflush( outFile );
            }
            nRfWrite_register( STATUS, (1<<RX_DR) );		//Clear Data ready flag
        }
        bcm2835_delay( 1 );                                 //Poll with 1 ms for new FIFO data
    }
    fclose( outFile );
    bcm2835_spi_end();
    bcm2835_close();
    fprintf(stderr, "\n\nFinito\n\n");
    return 0;
}

