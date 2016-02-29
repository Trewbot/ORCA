/*
 *  UWRF ORCA Flight Computer
 *  Written by Adam Hendel, Trevor J Hoglund
 *  
 *  References:
 *  [1] Altimeter data sheet: http://cdn.sparkfun.com/datasheets/Sensors/Pressure/MPL3115A2.pdf
 *  [2] Altimeter commands: https://learn.sparkfun.com/tutorials/mpl3115a2-pressure-sensor-hookup-guide
 *  
 */

//  Include Libraries
    #include <SPI.h>
    #include <SD.h>
    #include <SparkFunMPL3115A2.h>
    MPL3115A2 altimeter;
    File dataFile;

//  Drag system setup
    double  prevApogee  = 1000,         //  Previous apogee in meters
            altitudeErr = 0.3,          //  Error in the altitude reading [1]
            prevAlt     = 0,            //  Set last loop variables
            prevAcc     = 0,
            prevVel     = 0,
            prevTime    = 0;
    boolean runDrag     = true,         //  Run drag system this run
            dragOpen    = false,        //  Grag system activated
            hasFired    = false,        //  If engine has been fired
            burnout     = false;        //  If engine has burned out
    int     tests[][2]  = {             //  Array of tests: {precent to run test at, percent to aim for}
                {50,85},
                {65,75}
            };
    int     activeTest  = 0;            //  Currently running test
    
void setup(){
    //  SD Card Setup
    int    i = 0;
    String filename,
           extension = ".txt";
    do{
        i++;
        filename  = "flight";
        filename += i;
        filename += extension;
    }   while(SD.exists(filename));
    dataFile = SD.open(filename, FILE_WRITE);

    //  Altimeter Setup
    altimeter.begin();
    altimeter.setModeAltimeter();       //  Measures in meters
    altimeter.setOversampleRate(7);     //  Set Oversample to the recommended 128
    altimeter.enableEventFlags();       //  Enable all three pressure and temp event flags
}

void loop(){
    //  Gather information
    double altitude = altimeter.readAltitude(); //  Get altitude
    unsigned long currTime = millis() / 1000;   //  Get time in seconds since run began

    //  Determing velocity and acceleration
    double deltTime = currTime - prevTime,
           deltAlt  = altitude - prevAlt,
           currVel  = deltAlt  / deltTime,
           deltVel  = currVel  - prevVel,
           currAcc  = deltVel  / deltTime;
    //  Update previous variables
           prevAlt  = altitude;
           prevTime = currTime;
           prevVel  = currVel;
           prevAcc  = currAcc;
    
    //  Check if burnout
    if(!hasFired && currAcc > 0){ //  If accelerating then say engine has fired
        hasFired = true;
        dataFile.println("ENGINE FIRED");
    }
    if(hasFired && currAcc < 8){  //  If decelerating after fired say burnout
        burnout = true;
        dataFile.println("BURNOUT");
    }

    //  Test if should open drag system or update test number
    for(int i = 0; i < (int)(sizeof tests / sizeof tests[0]); i++)
        if(altitude > (prevApogee * (tests[i][0] / 100)) - altitudeErr
        && altitude < (prevApogee * (tests[i][0] / 100)) + altitudeErr){
            if(!dragOpen) openDragSystem();
            activeTest = i;
        }

    //  Test if should close drag system
    if(dragOpen && shouldClose()) closeDragSystem();

    //  Finish loop
    dataFile.print("t = ");     dataFile.print(currTime);
    dataFile.print(", alt = "); dataFile.print(altitude);
    dataFile.print(", a = ");   dataFile.print(currAcc);
    dataFile.print(", v = ");   dataFile.print(currVel);
    dataFile.println();
    delay(600);  //  Delay based on data aquisition rate. [1]
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

