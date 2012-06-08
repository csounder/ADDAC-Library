//INCLUDE STANDARDCPLUSPLUS LIBRARY//
#include <StandardCplusplus.h>
#include <vector>              
////////////////////////////////////

//include ADDAC MAIN LIBRARY
#include <ADDAC.h>
//include ADDAC COMPARATOR CLASS
#include <ADDAC_Comparator.h>

//initiate ADDAC CLASS's ang name them
ADDAC VCC;
ADDAC_Comparator comp1, comp2;


// DEBUGGING CONSOLE
#define DEBUG 

#define addacMaxResolution 65535  // CONSTANT FOR MAXIMUM RESOLUTION RANGE - NOT RECOMMENDED TO CHANGE - !!


void setup()
{
  VCC.setup();
#ifdef DEBUG
  Serial.begin(115200);
#endif

}

void loop(){
  VCC.update();


  if(VCC.MODE==0){  //  MODE 0 has several test/debug functions for the available functions and modules
    // ---------------------------------------------------------------------------------------------------------------------- SUBMODE 0 -

    //WORKING ON MODE "O" - SUBMODE "0"
    if(VCC.SUBMODE==0){


      //create a boolean variable type that will be true if we move our Manual POT above 0.5
      boolean normalTrigger = comp1.Comparator(VCC.ReadManual(A,0),0.5); 

      //create a boolean variable type that will be true after 5 second if we move our Manual POT above 0.5
      boolean delayedTrigger = comp2.Comparator(VCC.ReadManual(A,1),0.5,5000);

      //if normalTrigger is true print a message in the Serial Monitor
      if(normalTrigger){
        Serial.println("NOW!");
      }

      //if delayedTrigger is true print a message in the Serial Monitor
      if(delayedTrigger){
        Serial.println("5 SECOND AGO!");
      }
    }
  }

#ifdef DEBUG

  Serial.println();
  delay(10);
#endif

}












