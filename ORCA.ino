/*
 *  UWRF ORCA Flight Computer
 *  Written by Adam Hendel, Trevor J Hoglund
 *  
 *  References:
 *  [1] Altimeter data sheet: http://cdn.sparkfun.com/datasheets/Sensors/Pressure/MPL3115A2.pdf
 *  
 */

//  Include Libraries
    #include <SPI.h>
    #include <SD.h>
    #include <SparkFunMPL3115A2.h>
    #include "I2Cdev.h";
    File myFile;
    MPL3115A2 altimeter;

//  Drag system setup
    double  prevApogee  = 1000,         //  Previous apogee in meters
            altitudeErr = 10;           //  Error in the altitude reading
    boolean runDrag     = true,         //  Run drag system this run
            dragOpen    = false;        //  Grag system activated
    int     tests[][2]  = {             //  Array of tests: {precent to run test at, percent to aim for}
                {50,85},
                {65,75}
            };
    int     activeTest  = 0;            //  Currently running test
    
void setup(){
    //  SD Card Setup
    int i = 0;
    while(SD.exists("flight" + i + ".txt")) i++;
    File dataFile = SD.open("flight" + i + ".txt", FILE_WRITE);

    //  Altimeter Setup
    altimeter.begin();
    altimeter.setModeAltimeter();       //  Measures in meters
    altimeter.setOversampleRate(7);     //  Set Oversample to the recommended 128
    altimeter.enableEventFlags();       //  Enable all three pressure and temp event flags
}

void loop(){
    double altitude = altimeter.getAltitude();  //  Get altitude
    unsigned long time = millis();              //  Get time in milliseconds since run began
    if(dragOpen){
        if(shouldClose()) closeDragSystem();
    }else
        for(int i = 0; i < (int)(sizeof tests / sizeof tests[0]); i++)
            if(altitude > (prevApogee * (tests[i][0] / 100)) - altitudeErr
            && altitude < (prevApogee * (tests[i][0] / 100)) + altitudeErr){
                if(!dragOpen) openDragSystem();
                activeTest = i;
            }
    delay(1000);  //  Delay based on data aquisition rate. [1]
}

void openDragSystem(){
    //  TODO (Interface)
}

void closeDragSystem(){
    //  TODO (Interface)
}

boolean shouldClose(){
    //  TODO (Algorithm)
}

