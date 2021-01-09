
# Build example
Tracking device N1:  
make clean  
make tsb0 DEFAULT_AM_ADDR=0x1234 DESTINATION_GATEWAY_ADDRESS=0xABCD  
make tsb0 install TSB019  

Tracking device N2:  
make clean  
make tsb0 DEFAULT_AM_ADDR=0xABCD DESTINATION_GATEWAY_ADDRESS=0x1234  
make tsb0 install TSB018  

# Testing
It is necessary to install Ruby to compile unit tests  
apt install ruby  

./sensor-simulator --input ~/car.csv serial@/dev/tsb01801 serial@/dev/tsb01901  
