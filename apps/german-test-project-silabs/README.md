
# Build example
Tracking device N1:
make clean
make tsb0 DEFAULT_AM_ADDR=0x1234 DESTINATION_GATEWAY_ADDRESS=0xABCD
make tsb0 install TSB019

Tracking device N2:
make clean
make tsb0 DEFAULT_AM_ADDR=0xABCD DESTINATION_GATEWAY_ADDRESS=0x1234
make tsb0 install TSB018
