#!/bin/bash
echo "We'll use netcat to pipe stdout over port 5000 to the receiver"
echo "Note that the receiver must be started first with:"
echo "nc -l -p 5000 | hexdump -C"
echo "or"
echo "nc -l -p 5000 | wireshark -k -i -"
echo " "
sudo ./nRFsniffer 64 2 0xE7 0xE7 0xE7 | nc -v kimchi 5000

