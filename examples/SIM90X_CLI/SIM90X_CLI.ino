/*************************************************************************
  This is an example for our SIM90X Arduino library

  These displays use TTL Serial to communicate, 2 pins are required to 
  interface
  
  This library is based on Adafruit FONA library written by Limor 
  Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ************************************************************************/

/*************************************************************************
 THIS CODE IS STILL IN PROGRESS!

 Open up the serial console on the Arduino at 115200 baud to interact 
 with SIM90X

 Note that if you need to set a GPRS APN, username, and password scroll 
 down to the commented section below at the end of the setup() function.
 ************************************************************************/

#include <SoftwareSerial.h>
#include <SIM90X.h>

// #define SIM90X_RST 4

// this is a large buffer for replies
char replybuffer[255];

// or comment this out & use a hardware serial port like Serial1 (see below)
SoftwareSerial simSS = SoftwareSerial(7, 8);

//SIM90X sim = SIM90X(SIM90X_RST);
SIM90X sim = SIM90X();

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup() {
  while(!Serial);

  Serial.begin(115200);
  Serial.println(F("SIM90X CLI"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // make it slow so its easy to read!
  simSS.begin(9600); // if you're using software serial

  // See if the SIM90X is responding
  if(!sim.begin(simSS)){           // can also try sim.begin(Serial1) 
    Serial.println(F("Couldn't find SIM90X"));
    while(1);
  }
  Serial.println(F("SIM90X is OK"));

  // Print SIM card IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = sim.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  // Optionally configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  sim.setGPRSNetworkSettings(F("mobile.vodafone.it"));

  // Optionally configure HTTP gets to follow redirects over SSL.
  // Default is not to follow SSL redirects, however if you uncomment
  // the following line then redirects over SSL will be followed.
  //sim.setHTTPSRedirect(true);

  printMenu();
}

void printMenu(void){
  Serial.println(F("+-------------------------------------------------------------------------+"));
  Serial.println(F("| SIM90X CLI                                                              |"));
  Serial.println(F("|                                                                         |"));
  Serial.println(F("| print this menu...............[?]    read the ADC (2.8V max)........[a] |"));
  Serial.println(F("| read Battery V and % charged..[b]    read the SIM CCID..............[C] |"));
  Serial.println(F("| Unlock SIM with PIN code......[U]    read RSSI......................[i] |"));
  Serial.println(F("| get Network status............[n]    set audio Volume...............[v] |"));
  Serial.println(F("| get Volume....................[V]    set Headphone audio............[H] |"));
  Serial.println(F("| set External audio ...........[e]    play audio Tone................[T] |"));
  Serial.println(F("| make phone Call ..............[c]    Hang up phone..................[h] |"));
  Serial.println(F("| Pick up phone.................[p]    get Number of SMSs.............[N] |"));
  Serial.println(F("| Read SMS......................[r]    Read All SMS...................[R] |"));
  Serial.println(F("| Delete SMS....................[d]    Send SMS.......................[s] |"));
  Serial.println(F("| Enable network time sync......[y]    Get network time...............[t] |"));
  Serial.println(F("| Enable GPRS...................[G]    Disable GPRS...................[g] |"));
  Serial.println(F("| Get GPRS status...............[I]    Read webpage (GPRS)............[w] |"));
  Serial.println(F("| Post to website (GPRS)........[W]    create Serial passthru tunnel..[S] |"));
  Serial.println(F("|                                                                         |"));
  Serial.println(F("+-------------------------------------------------------------------------+"));
}

void loop() {
  Serial.print(F("SIM90X> "));
  while (! Serial.available() );
  
  char command = Serial.read();
  Serial.println(command);
  
  switch (command) {
    case '?': {
      printMenu();
      break;
    }
    
    case 'a': {
      // read the ADC
      uint16_t adc;
      if (! sim.getADCVoltage(&adc)) {
        Serial.println(F("Failed to read ADC"));
      } else {
        Serial.print(F("ADC = ")); Serial.print(adc); Serial.println(F(" mV"));
      }
      break;
    }
    
    case 'b': {
        // read the battery voltage and percentage
        uint16_t vbat;
        if (! sim.getBattVoltage(&vbat)) {
          Serial.println(F("Failed to read Batt"));
        } else {
          Serial.print(F("VBat = ")); Serial.print(vbat); Serial.println(F(" mV"));
        }
 

        if (! sim.getBattPercent(&vbat)) {
          Serial.println(F("Failed to read Batt"));
        } else {
          Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
        }
 
        break;
    }

    case 'U': {
        // Unlock the SIM with a PIN code
        char PIN[5];
        flushSerial();
        Serial.println(F("Enter 4-digit PIN"));
        readline(PIN, 3);
        Serial.println(PIN);
        Serial.print(F("Unlocking SIM card: "));
        if (! sim.unlockSIM(PIN)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }        
        break;
    }

    case 'C': {
        // read the CCID
        sim.getSIMCCID(replybuffer);  // make sure replybuffer is at least 21 bytes!
        Serial.print(F("SIM CCID = ")); Serial.println(replybuffer);
        break;
    }

    case 'i': {
        // read the RSSI
        uint8_t n = sim.getRSSI();
        int8_t r;
        
        Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(": ");
        if (n == 0) r = -115;
        if (n == 1) r = -111;
        if (n == 31) r = -52;
        if ((n >= 2) && (n <= 30)) {
          r = map(n, 2, 30, -110, -54);
        }
        Serial.print(r); Serial.println(F(" dBm"));
       
        break;
    }
    
    case 'n': {
        // read the network/cellular status
        uint8_t n = sim.getNetworkStatus();
        Serial.print(F("Network status ")); 
        Serial.print(n);
        Serial.print(F(": "));
        if (n == 0) Serial.println(F("Not registered"));
        if (n == 1) Serial.println(F("Registered (home)"));
        if (n == 2) Serial.println(F("Not registered (searching)"));
        if (n == 3) Serial.println(F("Denied"));
        if (n == 4) Serial.println(F("Unknown"));
        if (n == 5) Serial.println(F("Registered roaming"));
        break;
    }
    
    /*** Audio ***/
    case 'v': {
      // set volume
      flushSerial();
      Serial.print(F("Set Vol %"));
      uint8_t vol = readnumber();
      Serial.println();
      if (! sim.setVolume(vol)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      break;
    }

    case 'V': {
      uint8_t v = sim.getVolume();
      Serial.print(v); Serial.println("%");
    
      break; 
    }
    
    case 'H': {
      // Set Headphone output
      if (! sim.setAudio(SIM90X_HEADSETAUDIO)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      sim.setMicVolume(SIM90X_HEADSETAUDIO, 15);
      break;
    }
    case 'e': {
      // Set External output
      if (! sim.setAudio(SIM90X_EXTAUDIO)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }

      sim.setMicVolume(SIM90X_EXTAUDIO, 10);
      break;
    }

    case 'T': {
      // play tone
      flushSerial();
      Serial.print(F("Play tone #"));
      uint8_t kittone = readnumber();
      Serial.println();
      // play for 1 second (1000 ms)
      if (! sim.playToolkitTone(kittone, 1000)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      break;
    }

    /*** Call ***/
    case 'c': {      
      // call a phone!
      char number[30];
      flushSerial();
      Serial.print(F("Call #"));
      readline(number, 30);
      Serial.println();
      Serial.print(F("Calling ")); Serial.println(number);
      if (!sim.callPhone(number)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("Sent!"));
      }
      
      break;
    }
    case 'h': {
       // hang up! 
      if (! sim.hangUp()) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      break;     
    }

    case 'p': {
       // pick up! 
      if (! sim.pickUp()) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      break;     
    }
    
    /*** SMS ***/
    
    case 'N': {
        // read the number of SMS's!
        int8_t smsnum = sim.getNumSMS();
        if (smsnum < 0) {
          Serial.println(F("Could not read # SMS"));
        } else {
          Serial.print(smsnum); 
          Serial.println(F(" SMS's on SIM card!"));
        }
        break;
    }
    case 'r': {
      // read an SMS
      flushSerial();
      Serial.print(F("Read #"));
      uint8_t smsn = readnumber();
      Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);

      // Retrieve SMS sender address/phone number.
      if (! sim.getSMSSender(smsn, replybuffer, 250)) {
        Serial.println("Failed!");
        break;
      }
      Serial.print(F("FROM: ")); Serial.println(replybuffer);

      // Retrieve SMS value.
      uint16_t smslen;
      if (! sim.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
        Serial.println("Failed!");
        break;
      }
      Serial.print(F("***** SMS #")); Serial.print(smsn); 
      Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
      Serial.println(replybuffer);
      Serial.println(F("*****"));
      
      break;
    }
    case 'R': {
      // read all SMS
      int8_t smsnum = sim.getNumSMS();
      uint16_t smslen;
      for (int8_t smsn=1; smsn<=smsnum; smsn++) {
        Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
        if (!sim.readSMS(smsn, replybuffer, 250, &smslen)) {  // pass in buffer and max len!
           Serial.println(F("Failed!"));
           break;
        }
        // if the length is zero, its a special case where the index number is higher
        // so increase the max we'll look at!
        if (smslen == 0) {
          Serial.println(F("[empty slot]"));
          smsnum++;
          continue;
        }
        
        Serial.print(F("***** SMS #")); Serial.print(smsn); 
        Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
        Serial.println(replybuffer);
        Serial.println(F("*****"));
      }
      break;
    }

    case 'd': {
      // delete an SMS
      flushSerial();
      Serial.print(F("Delete #"));
      uint8_t smsn = readnumber();
      
      Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
      if (sim.deleteSMS(smsn)) {
        Serial.println(F("OK!"));
      } else {
        Serial.println(F("Couldn't delete"));
      }
      break;
    }
    
    case 's': {
      // send an SMS!
      char sendto[21], message[141];
      flushSerial();
      Serial.print(F("Send to #"));
      readline(sendto, 20);
      Serial.println(sendto);
      Serial.print(F("Type out one-line message (140 char): "));
      readline(message, 140);
      Serial.println(message);
      if (!sim.sendSMS(sendto, message)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("Sent!"));
      }
      
      break;
    }

    /*** Time ***/

    case 'y': {
      // enable network time sync
      if (!sim.enableNetworkTimeSync(true))
        Serial.println(F("Failed to enable"));
      break;
    }

    case 't': {
        // read the time
        char buffer[23];

        sim.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
        Serial.print(F("Time = ")); Serial.println(buffer);
        break;
    }

    /*********************************** GPRS */
    
    case 'g': {
       // turn GPRS off
       if (!sim.enableGPRS(false))  
         Serial.println(F("Failed to turn off"));
       break;
    }
    case 'G': {
       // turn GPRS on
       if (!sim.enableGPRS(true))  
         Serial.println(F("Failed to turn on"));
       break;
    }
    case 'I': {
      // Get GPRS status
      Serial.print(F("GPRS State: "));
      Serial.println(sim.GPRSstate());
      break;
    }
    case 'w': {
      // read website URL
      uint16_t statuscode;
      int16_t length;
      char url[80];
      
      flushSerial();
      Serial.println(F("NOTE: in beta! Use small webpages to read!"));
      Serial.println(F("URL to read (e.g. www.adafruit.com/testwifi/index.html):"));
      Serial.print(F("http://")); readline(url, 79);
      Serial.println(url);
      
       Serial.println(F("****"));
       if (!sim.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
         Serial.println("Failed!");
         break;
       }
       while (length > 0) {
         while (sim.available()) {
           char c = sim.read();
           
           // Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
           loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
           UDR0 = c;
#else
           Serial.write(c);
#endif
           length--;
           if (! length) break;
         }
       }
       Serial.println(F("\n****"));
       sim.HTTP_GET_end();
       break;
    }
    
    case 'W': {
      // Post data to website
      uint16_t statuscode;
      int16_t length;
      char url[80];
      char data[80];
      
      flushSerial();
      Serial.println(F("NOTE: in beta! Use simple websites to post!"));
      Serial.println(F("URL to post (e.g. httpbin.org/post):"));
      Serial.print(F("http://")); readline(url, 79);
      Serial.println(url);
      Serial.println(F("Data to post (e.g. \"foo\" or \"{\"simple\":\"json\"}\"):"));
      readline(data, 79);
      Serial.println(data);
      
       Serial.println(F("****"));
       if (!sim.HTTP_POST_start(url, F("application/x-www-form-urlencoded"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
         Serial.println("Failed!");
         break;
       }
       while (length > 0) {
         while (sim.available()) {
           char c = sim.read();
           
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
           loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
           UDR0 = c;
#else
           Serial.write(c);
#endif
           
           length--;
           if (! length) break;
         }
       }
       Serial.println(F("\n****"));
       sim.HTTP_POST_end();
       break;
    }
    /*****************************************/
      
    case 'S': {
      Serial.println(F("Creating SERIAL TUBE"));
      while (1) {
        while (Serial.available()) {
          sim.write(Serial.read());
        }
        if (sim.available()) {
          Serial.write(sim.read());
        }
      }
      break;
    }
    
    default: {
      Serial.println(F("Unknown command"));
      printMenu();
      break;
    }
  }
  // flush input
  flushSerial();
  while (sim.available()) {
    Serial.write(sim.read());
  }

}

void flushSerial() {
    while (Serial.available()) 
    Serial.read();
}

char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}
uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}
  
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;
  
  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while(Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;
        
        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }
    
    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}