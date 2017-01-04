# nRF24L01-sniffer

This sniffer allows to passively sniff the communication of nRF24 - like RF modules.

The project was inspired by and is a complete rewrite of [Yveaux's nRF24L01+ sniffer](http://yveaux.blogspot.com/2014/07/nrf24l01-sniffer-part-1.html), using only a Raspberry Pi and a nRF24L01+ module. As I couldn't get Yveaux's wireshark dissector to compile under linux, I implemented it as a platform independent LUA script.

# Overview

nRF Promiscuous mode: the internal packet, CRC and and retransmit functionality of the module is switched off. Any burst of data with the right preamble and the right address will be received by the hardware. Note that only the upper 2 out of 5 address bytes need to be known in advance. Always the maximum length of 32 bytes is received, regardless of the real packet length. The data is fed to wireshark, which will implement the low level functions of the `Shock Burst` protocol, like reconstructing payload, checking CRC, marking invalid packets, etc.

The Raspi runs a small C program, communicating with the nRF over SPI. If a packet was received, the 32 byte blob is read and forwarded to _stdout_ and _stderr_. The former is in binary [libpcap](https://wiki.wireshark.org/Development/LibpcapFileFormat) format (understood by wireshark), the latter is a human readable hex dump.

The netcat tool is used to forward _stdout_ to another (linux) PC, running Wireshark. The data is piped into wireshark, which runs a custom lua script to dissect the nRF packets and check their CRC.

Alternatively wireshark can be run locally on the Raspberry Pi -- which will be slower.

# Hardware

You'll need a Raspberry Pi (any model) and a nRF24L01+ RF module. Hook it up as follows ...

# Installation

## Dependencies

  * [bcm2835](http://www.airspayce.com/mikem/bcm2835/)

## Compile on the Raspberry Pi
    
    $ make
    
## Install wireshark lua script

    $ cp ./nRF24_dissector.lua ~/.wireshark/plugins/
    
# Live capture

## Local

__Commandline arguments__

    ./nRFsniffer <nRF_channel> <nRF_mbps> <adr_4> <adr_3> [ <adr_2> <adr_1> <adr_0> ]

 * __nRF_channel__  the RF channel to spy on ( 0 - 127 )
 * __nRF_mbps__ run with 1 or 2 mbps bit rate ( 1 / 2 )
 * __adr_x__ the most significant address byte is __adr_4__ ( 0x00 - 0xFF ). Note you must specify at least __adr_4__ and __adr_3__, the other ones are optional. You will only capture data with packets starting with that particular address. 
 
Note that checksum calculation only works if the __adr_x__ values are also configured in the wireshark dissector under
 
     Edit --> Preferences --> Protocols --> NRF24 --> Upper Address Byte

__Capture to console__

    sudo ./nRFsniffer 64 2 0xE7 0xE7 0xE7 > /dev/null

This should initialize the nRF module, show its register values and start capturing to console

__Run Wireshark locally__

    $ sudo ./nRFsniffer 64 2 0xE7 0xE7 0xE7 | wireshark -k -i -

## Remote

For a remote capture session, first startup wireshark on a PC like this

    $ nc -l -p 5000 | wireshark -k -i -
    
Then start capturing on the Raspberry Pi like this

    $ sudo ./nRFsniffer 64 2 0xE7 0xE7 0xE7 | nc -v 192.168.1.1 5000
    
Where `192.168.1.1` is the IP address of your PC.

# Example session

## Cmndline output

    ----------------------
     nRF24 RF sniffer
    ----------------------
    Arguments > ./nRFsniffer <nRF_channel (0-127)> <nRF_mbps (1/2)> <adr_4> <adr_3> [ <adr_2> <adr_1> <adr_0> ] 
      Example > sudo ./nRFsniffer 64 2 0xE7 0xE7 0xE7 > /dev/null 
    Returns human readable stuff on stderr, pcap binary data on stdout.

    !!! Listening to RF Ch. 64 with 2 Mbps for anyhing starting with Adr. 0xE7E7E7 [3 bytes] !!!

    All nRF24 registers:
    0x00:  73 00 01 01 00 40 0f 2e 
    0x08:  0f 00 e7 e1 e2 e3 e4 e5 
    0x10:  e2 20 00 00 00 00 00 11 
    0x18:  00 00 00 00 00 00 

    (1483525210) [dd 57 af 2f 7d ab 59 7b d6 ff 5b 5f fb a6 ed b7 aa b5 d7 55 6d f5 6a e6 49 f5 d3 4a dd b6 da ea]
    (1483525211) [e7 e1 33 02 00 30 87 54 15 23 aa 40 00 2d 74 5d 77 37 76 ea b7 9b af 6f ff 55 9d b6 9b b6 3a a8]
    (1483525211) [e7 e1 33 02 00 30 87 54 15 23 aa 40 00 2d 74 5d 77 25 6a e6 9b 88 8a db 7a aa 94 a6 12 92 a2 ad]


## Wireshark output

![Example Wireshark capture](https://github.com/yetifrisstlama/nRF24L01-sniffer/raw/master/nrf_ws.png)


