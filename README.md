# _Auredia MB-Slave_


## info
This was developed using the ESP-IDF v5.3 (required) extension on vs-code, other IDEs have not been tested.
Original documentation of the APK/APIs is here:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/console.html - CLI
https://docs.espressif.com/projects/esp-modbus/en/latest/esp32/slave_api_overview.html - MB
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi-security.html - WPA2 enterprise

## how to edit modbus
Overview - 
  The function "slave_init()" holds the processes to initialize the modbus controller, communication parameters, and the registers.
  The function "slave_operation_func()" is a while loop that continuously checks for reads/writes and handles/logs them.

How to use the CLI - 
  Start the modbus service through the CLI, there are three steps:
  1. Connect to wifi - run connect_wifi_enterprise/connect_wifi
  2. initialize modbus = run init_modbus
  3. start modbus - run start_modbus

How to use multiple ADC channels - 
  In ADC.h, there's an array that holds the registers you want to read, the maximum amount is 8, as there are only 8 channels in the ESP32's ADC1. To know which 
  register corresponds to which ADC channel, look at its place in the array. The channel number is assigned to the register number based on its index in the array,
  so the register number at index 0, would be assigned channel 0, which is GPIO36.


How to initialize registers - 
  Register areas are initialized by populating a mb_register_area_descriptor_t type struct then inputting that into the0 "mbc_slave_set_descriptor()" function.
  In the slave_demo, this is found in the "slave_init()" function, where you specify the type, starting index, memory address, and size.
  Note: you can initialize multiple areas of the same type with different starting indexes.

How to define registers you want to read - 
  There is an array in modbus.c, this array contains a list of which register indices that you need to be read. Each index should have an input from an ADC channel.
  
Misc. Notes:
  If you'd like the modbus slave to only handle requests to its specific Slave ID, then remove the parenthesis around "1" in the "MB_Slave_ID" Define. I don't know why this happens.

## Versions
  ***1.0.0***
 - Support for WPA2 enterprise wifi
 - Hosts a MB slave server and is able to be read+changed from any master
 - Modbus register storage is initialized through predefined arrays
   
  ***1.1.0***
 - Added CLI, Reorganized files - more reorganization soon.
   
  ***1.2.0***
 - Added WPA wifi support
 - Concurrency support
 - added arguments to wifi commands
 - ***only works with ESP32 not ESP32S2 mini***
   
  ***1.2.1***
 - wifi is setup on launch - just need to run connect command
 - kill_slave command added
 - created logging/regular slaves
 - created flags to prevent uninitialized functions from running

 ***1.3***
 - ADC added - works with multiple ADC channels
 - Multiple ADC channels implementation is scuffed - its all manually done

***1.3.1***
- Multiple ADC channels made less manual - needs more bug testing


# esp32-modbus-slave
