# nRF24L01-sniffer

I'm using several nRF24L01+ RF modules around my house. This is a debugging tool to see what is beeing sent over the air.

Basically this is a simplified rewrite of [this](http://yveaux.blogspot.com/2014/07/nrf24l01-sniffer-part-1.html) project, using only a Raspberry Pi and a nRF24L01+ module. As I couldn't get Yveaux's wireshark dissector to compile under linux, I implemented it as a platform independent LUA script.

# Overview

nRF Promiscuous mode: the internal packet, CRC and and retransmit functionality of the module is switched off. Any burst of data with the right preamble and the right address is received. Note that only the upper 3 out of 5 address bytes need to be known in advance. Always the maximum length of 32 bytes is received, regardless of the real packet length. Wireshark will implement the low level functions of the `Shock Burst` protocol, like reconstructing payload, checking CRC, marking invalid packets, etc.

More details here: http://yveaux.blogspot.com/2014/07/nrf24l01-sniffer-part-1.html

The Raspi runs a small C program, communicating with the nRF over SPI. If a packet was received, the 32 byte blob is read and forwarded to _stdout_ and _stderr_. The former is in binary [libpcap](https://wiki.wireshark.org/Development/LibpcapFileFormat) format (understood by wireshark), the latter is a human readable hex dump.

The netcat tool is used to forward _stdout_ to another (linux) PC, running Wireshark. The data is piped into wireshark, which runs a custom lua script to dissect the nRF packets and check their CRC.

Alternatively wireshark can be run locally on the Raspberry Pi -- which is a bit slower and less convenient if you're running `headless`.

# Hardware

You'll need a Raspberry Pi (any model) and a nRF24L01+ RF module. Hook it up as follows ...

# Installation

## Dependencies

  * [bcm2835](http://www.airspayce.com/mikem/bcm2835/)

## Compile on the Raspberry Pi
    
    $ make
    $ sudo ./myNRFsniffer | /dev/null
    
This should initialize the nRF module, show its register values and start capturing to console
    
## Install wireshark lua script

    $ cp ./xxx.lua ~/.wireshark/plugins
    
# Live capture

## Local

Run everything on the Raspberry Pi

    $  > wireshark

## Remote

For a remote capture session, first startup wireshark on a PC like this

    $ nc ..
    
Then start capturing on the Raspberry Pi like this

    $ nc .. 192.168.1.1 5000
    
Where `192.168.1.1` is the IP address of your PC.



