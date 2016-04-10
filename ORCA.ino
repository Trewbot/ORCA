/*
 *  UWRF ORCA Flight Computer
 *  Written by Adam Hendel, Trevor J Hoglund
 *  
 *  References:
 *  [1] Altimeter data sheet: http://cdn.sparkfun.com/datasheets/Sensors/Pressure/MPL3115A2.pdf
 *  [2] Altimeter commands:   https://learn.sparkfun.com/tutorials/mpl3115a2-pressure-sensor-hookup-guide
 *  
 */

//  Include Libraries
    #include <SPI.h>
    #include <SD.h>
    #include <Servo.h>
    #include <math.h>
    #include <SparkFunMPL3115A2.h>
    MPL3115A2 altimeter;
    Servo servoOne;
    File dataFile;

//  Drag system setup
    long    prevApogee  = 1666.9375,    //  Previous apogee in meters
            altitudeErr = 0.3,          //  Error in the altitude reading [1]
            prevAlt     = 0,            //  Set last loop variables
            prevAcc     = 0,
            prevVel     = 0,
            prevTime    = 0,
            crossSecArea = 0.01035,     //  Cross sectional area in meters squared
            gravity     = 0.00000981,   //  Acceleration due to gravity in meteres per second squared
            coeffDrag   = 0.15,         //  Coefficient of drag, estimated???                           TODO
            rho         = 1.225,        //  Density of the medium kg/m2                                 TODO
            mass        = 5.094;        //  Mass of the rocket after fuel is spent in kilograms         TODO
    boolean runDrag     = true,        //  Run drag system this run
            dragOpen    = false,        //  Grag system activated
            hasFired    = false,        //  If engine has been fired
            burnout     = false,        //  If engine has burned out
            apogee      = false,        //  If apogee has been reached
            test        = true;         //  Test variable
    int     tests[][2]  = {             //  Array of tests: {precent to run test at, percent to aim for}
                {50,85},
                {65,75}
            };
    int     activeTest  = 0,            //  Currently running test
            sdPin       = 10,           //  SD Card Pin
            servoPin    = 9,            //  Servo pin
            sweep       = 0,
            sweeps      = 3;            //  Amount of intitial sweeps
    String  filename,
            extension   = ".txt";
    
void setup(){
    //  SD Card Setup
    int    i = 0;
    SD.begin(sdPin);
    do{
        i++;
        filename  = "flight";
        filename += i;
        filename += extension;
    }   while(SD.exists(filename));
    dataFile = SD.open(filename, FILE_WRITE);
    dataFile.println("Time\tChange in time\tAltitude\tVelocity\tAcceleration");
    dataFile.println("ms\tms\tm\tm/s\tm/s*s");
    dataFile.println();
    dataFile.close();
    
    //  Servo Setup
    servoOne.attach(servoPin);
    
    //  Altimeter Setup
    altimeter.begin();
    altimeter.setModeAltimeter();       //  Measures in meters
    altimeter.setOversampleRate(7);     //  Set Oversample to the recommended 128
    altimeter.enableEventFlags();       //  Enable all three pressure and temp event flags
    prevAlt = altimeter.readAltitude();
}

void loop(){
    //  Gather information
    double altitude = altimeter.readAltitude();         //  Get altitude
    unsigned long currTime = millis();                  //  Get time in seconds since run began

    //  Open file
    //  SD.open(filename);
    dataFile = SD.open(filename, FILE_WRITE);
    
    //  Determing velocity and acceleration
    float  deltTime = currTime - prevTime,
           deltAlt  = altitude - prevAlt,
           currVel  = deltAlt  / deltTime,
           deltVel  = currVel  - prevVel,
           currAcc  = deltVel  / deltTime;
    //  Update previous variables
           prevAlt  = altitude;
           prevTime = currTime;
           prevVel  = currVel;
           prevAcc  = currAcc;

    //  Log data
                          dataFile.print(currTime);
    dataFile.print("\t"); dataFile.print(deltTime);
    dataFile.print("\t"); dataFile.print(altitude,5);
    dataFile.print("\t"); dataFile.print(currVel*1000,5);
    dataFile.print("\t"); dataFile.print(currAcc*1000000,5);
    
    //  Check if burnout
    if(!hasFired && currAcc > 0.000006){ //  If accelerating then say engine has fired
        hasFired = true;
        dataFile.print("\tENGINE FIRED");
    }
    if(hasFired && !burnout && currAcc < -0.000009){  //  If decelerating after fired say burnout
        burnout = true;
        dataFile.print("\tBURNOUT");
    }
    if(burnout && !apogee && currVel < -0.0001){
        apogee = true;
        dataFile.println("\tAPOGEE");
        File apogeeFile = SD.open("apogee.txt",FILE_WRITE);
        apogeeFile.println(altitude);
        apogeeFile.close();
    }

    //  Test if should open drag system or update test number
    for(int i = 0; i < (int)(sizeof tests / sizeof tests[0]); i++)
        if(altitude > (prevApogee * (tests[i][0] / 100)) - altitudeErr
        && altitude < (prevApogee * (tests[i][0] / 100)) + altitudeErr){
            if(!dragOpen) openDragSystem();
            activeTest = i;
        }

    //  Test if should close drag system
    if(dragOpen && shouldClose(altitude,currVel,(tests[activeTest][1]/100)*prevApogee)) closeDragSystem();

    //  Initialization sweeps
    if(sweep <= sweeps){
        if(test){
            closeDragSystem();
            sweep++;
        } else openDragSystem();
        test = !test;
    }
    
    //  Finish loop
    dataFile.println();
    dataFile.close(); // Maybe just dataFile.flush();
    delay(600);       // Delay based on data aquisition rate. [1]
}

void openDragSystem(){
    servoOne.write(45);
}

void closeDragSystem(){
    servoOne.write(122.5);
}

boolean shouldClose(long altitude, long velocity, long target){
    long  velTermSq = (2 * gravity * mass) / (coeffDrag * rho * crossSecArea),
          maxAlt    = altitude + ((velTermSq / 2 * gravity) * log((velTermSq + pow(velocity, 2)) / velTermSq));
    return maxAlt <= target * 1.05;
}

