# piclib
MPLAB library for programming Microchip 8-bits PIC microcontrollers

# Services

## CAN
can.h is the header file containing all methods to use CAN on the microchip. Look at the documentation there for more details

## DAO
dao.h is the header file containing all methods to access the EEPROM as the data storage of this microchip. Look at the documentation there for more details

## Usage
MPLAB libraries do not work cleanly with XC8. There is an option to create regular project and change output, but this has few disadvantages:
* No way to debug the code of the library since only binary is included
* A need to copy over the header files anyway

So to use this library in your project, just add the path to this project to your includes (project settings in MPLAB X). And then add the source files manually by "Add Existing". 
This way you use the files directly in your project and you can directly include the header files as needed, for example simple #include "can.h" would then work