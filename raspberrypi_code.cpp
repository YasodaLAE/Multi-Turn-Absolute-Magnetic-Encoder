#include <iostream>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>

using namespace std;

#define SPI_CHANNEL 0
const int sampleInterval = 9;                           // degrees
const int samples = int(360 / sampleInterval + 2);
float calibrationTable[samples];                        // for recording magnet angles every "sampleInterval" degrees
float interpolatedAngle;

float numberOfTurns = 0;                                // number of turns
float startAngle = 0;                                   // starting angle
float taredAngle = 0;                                   // tared angle - based on the startup value
float totalAngle = 0;                                   // total absolute angular displacement
float previousTotalAngle = 0;                           // for the display printing
int u;
int v;
int c;
int fin;
int quadrantNumber = 0;                                 // quadrant IDs
int previousQuadrantNumber = 0;                         // these are used for tracking the numberOfTurns


unsigned char transferAndWait(unsigned char what) {
    wiringPiSPIDataRW(SPI_CHANNEL, &what, 1);
    usleep(20);
    unsigned char a;
    wiringPiSPIDataRW(SPI_CHANNEL, &a, 1);
    return a;
}

void checkQuadrant(){
    /*
    //Quadrants:
    4  |  1
    ---|---
    3  |  2
  */
  if (fin >= 0 && fin <= 90) {
    quadrantNumber = 1;
  }

  //Quadrant 2
  if (fin > 90 && fin <= 180) {
    quadrantNumber = 2;
  }

  //Quadrant 3
  if (fin > 180 && fin <= 270) {
    quadrantNumber = 3;
  }

  // ----- Quadrant 4
  if (fin > 270 && fin < 360) {
    quadrantNumber = 4;
  }

  if (quadrantNumber != previousQuadrantNumber)                   // if we changed quadrant
  {
    if (quadrantNumber == 1 && previousQuadrantNumber == 4) {
      numberOfTurns++;                                            // 4 --> 1 transition: CW rotation
    }

    if (quadrantNumber == 4 && previousQuadrantNumber == 1) {
      numberOfTurns--;                                            // 1 --> 4 transition: CCW rotation
    }

    previousQuadrantNumber = quadrantNumber;                      //update to the current quadrant
  }
  totalAngle = (numberOfTurns * 360) + fin;            //number of turns (+/-) plus the actual angle within the 0-360 range
}

int main() {
    if (wiringPiSetup() == -1) {
        cerr << "wiringPi setup failed!" << endl;
        return 1;
    }

    int fd = wiringPiSPISetup(SPI_CHANNEL, 1000000); // 1 MHz clock speed
    if (fd == -1) {
        cerr << "wiringPiSPI setup failed!" << endl;
        return 1;
    }
    

    while (true) {
        // enable Slave Select
        digitalWrite(0, LOW);

        transferAndWait('s');  // add command
        transferAndWait(10);
        unsigned char a = transferAndWait(17);

        // disable Slave Select
        digitalWrite(0, HIGH);

        //cout << "Adding results: " << (int)a << endl;
        delay(1000);

        // enable Slave Select
        digitalWrite(0, LOW);

        transferAndWait('a');  // subtract command
        transferAndWait(10);
        unsigned char b = transferAndWait(17);

        // disable Slave Select
        digitalWrite(0, HIGH);
        u = (int)a;
        v = (int)b;
        c = (v<<8)|u;
        fin = c*360/4096;
        checkQuadrant();
        //cout << "int a: " << u << endl;
        //cout << "int b: " << v << endl;
        //cout << "int c: " << c << endl;
        cout << "Number of Turns: " << numberOfTurns << endl;
        cout << "Angle from 0 to 360: " << fin << endl;

        delay(1000);  // 1 second delay
    }

    return 0;
}
