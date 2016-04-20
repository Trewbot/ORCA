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
            gravity     = 9.81,         //  Acceleration due to gravity in meteres per second squared
            coeffDrag   = 0.15,         //  Coefficient of drag, estimated???                           TODO
            rho         = 1.225,        //  Density of the medium kg/m2                                 TODO
            mass        = 5.094,        //  Mass of the rocket after fuel is spent in kilograms         TODO
            delayStart  = 99999999;
    boolean runDrag     = true,         //  Run drag system this run
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
            sweeps      = 3,            //  Amount of intitial sweeps
            target      = 1172;         //  Target altitude (2nd launch)
    String  filename,
            extension   = ".txt";
    double  ground      = 0,
            A           = 5.12,         //constants for curve fit A-D
            B           = 0.34,
            C           = 14.15,
            D           = 1.57;
    
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
    dataFile.println("s\ts\tm\tm/s\tm/s*s");
    dataFile.println();
    dataFile.close();
    
    //  Servo Setup
    servoOne.attach(servoPin);
    
    //  Altimeter Setup
    altimeter.begin();
    altimeter.setModeAltimeter();       //  Measures in meters
    altimeter.setOversampleRate(0);     //  Set Oversample to 0
    altimeter.enableEventFlags();       //  Enable all three pressure and temp event flags
    prevAlt = altimeter.readAltitude(); //  Required for setup
    delay(100);
    ground = altimeter.readAltitude();

    Serial.begin(9600);

    //  Initialization sweeps
  for(sweeps;sweep<sweeps; sweep++){
       openDragSystem();
       delay(500);               //wait for arms to extend
       closeDragSystem();
       delay(500);              //wait for arms to retract
       }
}//end setup

void loop(){
    //  Gather information
    double altitude = altimeter.readAltitude() - ground;         //  Get altitude, subtract ground alt
    Serial.println(altitude);
    unsigned long currTime = millis();                  //  Get time in ms since run began

    //  Open file
    //  SD.open(filename);
    dataFile = SD.open(filename, FILE_WRITE);
    
    //  Determing velocity and acceleration
    long   deltTime = (currTime - prevTime) / 1000.0,
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
    dataFile.print("\t"); dataFile.print(deltTime,5);
    dataFile.print("\t"); dataFile.print(altitude,5);
    dataFile.print("\t"); dataFile.print(currVel,5);
    dataFile.print("\t"); dataFile.print(currAcc,5);
    
    //  Check if burnout
    if(!hasFired && (altitude > 30)){ // engine fired if change in alt >30
        hasFired = true;
        Serial.print("Fired");
        delayStart = millis();
        dataFile.print("\tENGINE FIRED");
    }
    if(hasFired && !burnout && millis() >= (delayStart + 4100)){  //  Burnout if 4.1 sec past ignition
        burnout = true;
        dataFile.print("\tBURNOUT");
        openDragSystem();
        ORCA(currVel);
    }

    //****BEGIN FAILSAFE*****
    if(!hasFired && (currVel>100) && (altitude>1220))           //TODO: set parameters
      openDragSystem();
    if(!burnout && (currVel<30) && (altitude>(target-5)))                                 
      closeDragSystem();
    //****END FAILSAFE****
    
    //  Finish loop
    dataFile.println();
    dataFile.close(); // Maybe just dataFile.flush();
}

void openDragSystem(){
    servoOne.write(45);
}

void closeDragSystem(){
    servoOne.write(122.5);
}

bool ORCA(long velocity){ //returns true if drag system should retract
  double expectedApogee = A + B*pow(abs(velocity-C),D);
  if(target>expectedApogee)
  return false;
}

