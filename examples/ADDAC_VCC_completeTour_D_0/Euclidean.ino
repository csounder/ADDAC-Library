#include <EEPROM.h>
#define brightness 8
#define debug 0 // 0 = normal 1 =  (internal clock) 2= SerialDump
#define display_update 2000 // how long active channel display is shown 
int length=50; //pulse length
#define sparepin 17

unsigned long time;
unsigned long last_sync;

int clocks[] = {
  4,8,12,16,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; // possible values for n - to make knob easier by offering presets
int noclocks = 19; // how many possible ns are available?

#define channels 4
unsigned int beat_holder[channels];


/*
Eeprom schema: 
 Channel 1: n = 1 k = 2 o = 7
 Channel 2: n = 3 k = 4 o = 8
 Channel 3: n = 5 k = 6 o = 9 
 */

unsigned int channelbeats[channels][4]={
  {
    3,12,0,0             }
  ,{
    5,15,0, 0             }
  ,{
    2,7,0,0              }
  ,{
    3,9,0,0              }
}; // 0=n, 1=k,2 = position , 3 = offset


int a; 
int changes=0;
boolean sleep=true;
int masterclock=0;
int read_head;
unsigned int  looptracker;

int old_total;//for knobs
int old_pulses;//for knobs

int pulseinput=2;
int newpulse;//for trigger in
int oldpulse=1;//for trigger in

boolean diga_old; // for encoders 
boolean digb_old;
boolean pulses_active = false; // is active while a beat pulse is playing 
boolean lights_active = false;   

int kknob;
int active_channel =1; // which channel is active? zero indexed 
int nknob;
int oknob; 
int maxn = 16; // maximums and minimums for n and k 
int minn = 1;
int mink = 1; 
int nn; 
int kk; 
unsigned long last_read; 
unsigned long last_changed; 
#define read_delay  50 // for debouncing 
int channel_switch;
int reset_button; 
int channel_switch_read;



void EUCLIDEAN() {
  /*
What's in the loop: 
   Update time variable 

   Changes routine - update beat_holder when channelbeats changes - triggered by changes == true
   Trigger routines - on trigget update displays and pulse
   Read encoders 
   Read switches 
   
   */



  time=millis();

  // COPY OVER N & K VARIABLES FOR EASE OF CODE READING 
  nn = channelbeats[active_channel][0];  
  kk = channelbeats[active_channel][1]; 


  // DEBUG PULSE TRIGGER & print out  
  if (debug >0 && time-last_sync > 250){
    Sync();
    if (debug ==2){
      Serial.print ("nn=");
      Serial.print (nn);
      Serial.print (" kk=");
      Serial.print (kk); 
    }   
  };

  // UPDATE BEAT HOLDER WHEN KNOBS ARE MOVED
  if (changes > 0){  
    beat_holder[active_channel] = euclid(nn,kk);
    changes = 0;  
    last_changed = time; 
  }

  // ANALOG PULSE TRIGGER 
  newpulse=map(cvValsA[5],0,1024,0,4); // Pulse input 
  if (newpulse>oldpulse){
    Sync();
  }
  oldpulse = newpulse;


  // READ K KNOB - 
  kknob = cvValsA[0]/1023.0f * 16.0f;//EncodeRead(enc1a,enc1b); 
  Serial.print(" kknob:");
  Serial.print(kknob);
  if (kknob != 0 && time-last_read>read_delay) { 

    /*if (channelbeats[active_channel][1]+kknob > channelbeats[active_channel][0]-1) {
      kknob=0;
    }; // check within limits
    if (channelbeats[active_channel][1]+kknob < mink) {
      kknob=0;
    };*/


    // CHECK AGAIN FOR LOGIC
    if (channelbeats[active_channel][1] > channelbeats[active_channel][0]-1){
      channelbeats[active_channel][1] = channelbeats[active_channel][0]-1;
    };

    channelbeats[active_channel][1] = kknob; // update with encoder reading
    //EEPROM.write((active_channel*2)+2,channelbeats[active_channel][1]); // write settings to 2/4/6 eproms 
    last_read = millis();
    changes = 1; // K change = 1
  }

  // READ N KNOB 
  nknob = cvValsA[1]/1023.0f * 16.0f;//EncodeRead(enc2a,enc2b); 
  Serial.print(" nknob:");
  Serial.print(nknob);
  if (nknob != 0 && time-last_read>read_delay) { 

    // Sense check n encoder reading to prevent crashes 
/*
    if (nn+nknob > maxn) {
      nknob=0;   
    }; // check below maxn
    if (nn+nknob < minn) {
      nknob=0;   
    }; // check above minn
    if (kk > nn+nknob-1 && kk>1){// check if new n is lower than k + reduce K if it is 
      //     nknob=0;   
      channelbeats[active_channel][1] = channelbeats[active_channel][1]+nknob;

    }; */

    channelbeats[active_channel][0] = nknob; // update with encoder reading
    kk = channelbeats[active_channel][1];
    nn = channelbeats[active_channel][0];  // update nn for ease of coding 


    //EEPROM.write((active_channel*2)+1,channelbeats[active_channel][0]); // write settings to 2/4/6 eproms 
    last_read = millis();
    changes = 2; // n change = 2 
  }


  // SELECT ACTIVE CHANNEL 
  channel_switch_read = cvValsA[2]; 
  if (channel_switch_read<120){
    channel_switch = 4;
  };
  if (channel_switch_read>121 && channel_switch_read<240){
    channel_switch = 3;
  };
  if (channel_switch_read>241 && channel_switch_read<400){
    channel_switch = 2;
  };
  if (channel_switch_read>401 && channel_switch_read<622){
    channel_switch = 1;
  };
  if (channel_switch_read>623){
    channel_switch = 0;
  };
  if (channel_switch !=4){
    active_channel = channel_switch;
  }; 
  Serial.print(" Achannel:");
  Serial.print(active_channel);
  // ENABLE RESET BUTTON 
  reset_button = cvValsA[4];
  if (reset_button>500 && channelbeats[0][2]>0){
    for (a=0; a<channels; a++){
      channelbeats[a][2]=0; 

    }
    Serial.print(" RESET ");
  }


  // FINISH ANY PULSES THAT ARE ACTIVE - PULSES LAST 1/4 AS LONG AS LIGHTS 
  if (time-last_sync>(length/4) && pulses_active==true){
    for(a=0;a<channels;a++){  
      VCC.WriteChannel(1+a,0);
      VCC.WriteChannel(5,0);
      VCC.WriteChannel(6+a,addacMaxResolution);
      //digitalWrite(11+a,LOW);
      //digitalWrite(sparepin, LOW);
    }
    pulses_active = false; 
  }



}




// Euclid calculation function 

unsigned int euclid(int n, int k){ // inputs: n=total, k=beats, o = offset
  int pauses = n-k;
  int pulses = k;
  int per_pulse = pauses/k;
  int remainder = pauses%pulses;  
  unsigned int workbeat[n];
  unsigned int outbeat;
  unsigned int working;
  int workbeat_count=n;
  int a; 
  int b; 
  int trim_count;
  for (a=0;a<n;a++){ // Populate workbeat with unsorted pulses and pauses 
    if (a<pulses){
      workbeat[a] = 1;
    }else {
      workbeat [a] = 0;
    }
  }

  if (per_pulse>0 && remainder <2){ // Handle easy cases where there is no or only one remainer  
    for (a=0;a<pulses;a++){
      for (b=workbeat_count-1; b>workbeat_count-per_pulse-1;b--){
        workbeat[a]  = ConcatBin (workbeat[a], workbeat[b]);
      }
      workbeat_count = workbeat_count-per_pulse;
    }

    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count 
    for (a=0;a < workbeat_count;a++){
      outbeat = ConcatBin(outbeat,workbeat[a]);
    }
    return outbeat;
  }else { 
    int groupa = pulses;
    int groupb = pauses; 
    int iteration=0;
    if (groupb<=1){
    }
    while(groupb>1){ //main recursive loop


      if (groupa>groupb){ // more Group A than Group B
        int a_remainder = groupa-groupb; // what will be left of groupa once groupB is interleaved 
        trim_count = 0;
        for (a=0; a<groupa-a_remainder;a++){ //count through the matching sets of A, ignoring remaindered
          workbeat[a]  = ConcatBin (workbeat[a], workbeat[workbeat_count-1-a]);
          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;

        groupa=groupb;
        groupb=a_remainder;
      
      }else if (groupb>groupa){ // More Group B than Group A
        int b_remainder = groupb-groupa; // what will be left of group once group A is interleaved 
        trim_count=0;
        for (a = workbeat_count-1;a>=groupa+b_remainder;a--){ //count from right back through the Bs
          workbeat[workbeat_count-a-1] = ConcatBin (workbeat[workbeat_count-a-1], workbeat[a]);

          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;
        groupb=b_remainder;
      
      }else if (groupa == groupb){ // groupa = groupb 
        trim_count=0;
        for (a=0;a<groupa;a++){
          workbeat[a] = ConcatBin (workbeat[a],workbeat[workbeat_count-1-a]);
          trim_count++;
        }
        workbeat_count = workbeat_count-trim_count;
        groupb=0;
      
      }else {
        //        Serial.println("ERROR");
      }
      iteration++;
    }

    outbeat = 0; // Concatenate workbeat into outbeat - according to workbeat_count 
    for (a=0;a < workbeat_count;a++){
      outbeat = ConcatBin(outbeat,workbeat[a]);
    }
    
    return outbeat;

  }
}



// Function to find the binary length of a number by counting bitwise 
int findlength(unsigned int bnry){
  boolean lengthfound = false;
  int length=1; // no number can have a length of zero - single 0 has a length of one, but no 1s for the sytem to count
  for (int q=32;q>=0;q--){
    int r=bitRead(bnry,q);
    if(r==1 && lengthfound == false){
      length=q+1;
      lengthfound = true;
    }
  }
  return length;
}

// Function to concatenate two binary numbers bitwise 
unsigned int ConcatBin(unsigned int bina, unsigned int binb){
  int binb_len=findlength(binb);
  unsigned int sum=(bina<<binb_len);
  sum = sum | binb;
  return sum;
}


// routine triggered by each beat
void Sync(){ 


  // Cycle through channels 
  for(a=0;a<channels;a++){
    read_head = channelbeats[a][0]-channelbeats[a][2]-1;  
    // turn on pulses on channels where a beat is present  
    if (bitRead (beat_holder[a],read_head)==1){
      VCC.WriteChannel(1+a,addacMaxResolution);
      VCC.WriteChannel(6+a,0);
      //digitalWrite(11+a,HIGH); // pulse out 
      pulses_active = true;   
      lights_active = true;   
    }

    // send off pulses to spare output for the first channel 
    if (bitRead (beat_holder[a],read_head)==0 && a == 0){ // only relates to first channel 
      VCC.WriteChannel(5,addacMaxResolution);
      //digitalWrite(sparepin,HIGH); // pulse out 
      pulses_active = true;   
      lights_active = true;   
    }

    // move counter to next position, ready for next pulse  
    channelbeats[a][2]++;
    if (channelbeats[a][2]>=channelbeats[a][0]){
      channelbeats[a][2] = 0; 
    }
  }

  masterclock++;
  if (masterclock>=16){
    masterclock=0;
  }

  looptracker++;
  length = ((time-last_sync)/5);
  last_sync = time;

}
