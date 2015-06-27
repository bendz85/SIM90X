/***************************************************
  This is a library for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#include <avr/pgmspace.h>
    // next line per http://postwarrior.com/arduino-ethershield-error-prog_char-does-not-name-a-type/
#define prog_char  char PROGMEM

#if (ARDUINO >= 100)
  #include "Arduino.h"
  #ifndef __SAM3X8E__  // Arduino Due doesn't support SoftwareSerial
    #include <SoftwareSerial.h>
  #endif
#else
  #include "WProgram.h"
  #include <NewSoftSerial.h>
#endif

#include "SIM90X.h"

SIM90X::SIM90X(int8_t rst)
{
  _rstpin = rst;
  
  apn = "NomadNT";
  apnusername = 0;
  apnpassword = 0;
  mySerial = 0;
  httpsredirect = false;
  useragent = F("SIM90X");
}

boolean SIM90X::begin(Stream &port) {
  mySerial = &port;

  pinMode(_rstpin, OUTPUT);
  digitalWrite(_rstpin, HIGH);
  delay(10);
  digitalWrite(_rstpin, LOW);
  delay(100);
  digitalWrite(_rstpin, HIGH);

  // give 3 seconds to reboot
  delay(3000);

  while (mySerial->available()) mySerial->read();

  sendCheckReply(F("AT"), F("OK"));
  delay(100);
  sendCheckReply(F("AT"), F("OK"));
  delay(100);
  sendCheckReply(F("AT"), F("OK"));
  delay(100);

  // turn off Echo!
  sendCheckReply(F("ATE0"), F("OK"));
  delay(100);

  if (! sendCheckReply(F("ATE0"), F("OK"))) {
    return false;
  }

  return true;
}


/********* Real Time Clock ********************************************/

boolean SIM90X::readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec) {
  uint16_t v;
  sendParseReply(F("AT+CCLK?"), F("+CCLK: "), &v, '/', 0);
  *year = v;

#ifdef SIM90X_DEBUG
  Serial.println(*year);
#endif
}

boolean SIM90X::enableRTC(uint8_t i) {
  if (! sendCheckReply(F("AT+CLTS="), i, F("OK"))) 
    return false;
  return sendCheckReply(F("AT&W"), F("OK"));
}


/********* BATTERY & ADC ********************************************/

/* returns value in mV (uint16_t) */
boolean SIM90X::getBattVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
}

/* returns the percentage charge of battery as reported by sim800 */
boolean SIM90X::getBattPercent(uint16_t *p) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}

boolean SIM90X::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
}

/********* SIM ***********************************************************/

uint8_t SIM90X::unlockSIM(char *pin)
{
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = NULL;

  return sendCheckReply(sendbuff, "OK");
}

uint8_t SIM90X::getSIMCCID(char *ccid) {
  getReply("AT+CCID");
  // up to 20 chars
  strncpy(ccid, replybuffer, 20);
  ccid[20] = 0;

  readline(); // eat 'OK'

  return strlen(ccid);
}

/********* IMEI **********************************************************/

uint8_t SIM90X::getIMEI(char *imei) {
  getReply("AT+GSN");

  // up to 15 chars
  strncpy(imei, replybuffer, 15);
  imei[15] = 0;

  readline(); // eat 'OK'

  return strlen(imei);
}

/********* NETWORK *******************************************************/

uint8_t SIM90X::getNetworkStatus(void) {
  uint16_t status;

  if (! sendParseReply(F("AT+CREG?"), F("+CREG: "), &status, ',', 1)) return 0;

  return status;
}


uint8_t SIM90X::getRSSI(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply) ) return 0;

  return reply;
}

/********* AUDIO *******************************************************/

boolean SIM90X::setAudio(uint8_t a) {
  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+CHFA="), a, F("OK"));
}

uint8_t SIM90X::getVolume(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CLVL?"), F("+CLVL: "), &reply) ) return 0;

  return reply;
}

boolean SIM90X::setVolume(uint8_t i) {
  return sendCheckReply(F("AT+CLVL="), i, F("OK"));
}


boolean SIM90X::playDTMF(char dtmf) {
  char str[4];
  str[0] = '\"';
  str[1] = dtmf;
  str[2] = '\"';
  str[3] = 0;
  return sendCheckReply(F("AT+CLDTMF=3,"), str, F("OK"));
}

boolean SIM90X::playToolkitTone(uint8_t t, uint16_t len) {
  return sendCheckReply(F("AT+STTONE=1,"), t, len, F("OK"));
}

boolean SIM90X::setMicVolume(uint8_t a, uint8_t level) {
  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+CMIC="), a, level, F("OK"));
}

/********* FM RADIO *******************************************************/


boolean SIM90X::FMradio(boolean onoff, uint8_t a) {
  if (! onoff) {
    return sendCheckReply(F("AT+FMCLOSE"), F("OK"));
  }

  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+FMOPEN="), a, F("OK"));
}

boolean SIM90X::tuneFMradio(uint16_t station) {
  // Fail if FM station is outside allowed range.
  if ((station < 870) || (station > 1090))
    return false;

  return sendCheckReply(F("AT+FMFREQ="), station, F("OK"));
}

boolean SIM90X::setFMVolume(uint8_t i) {
  // Fail if volume is outside allowed range (0-6).
  if (i > 6) {
    return false;
  }
  // Send FM volume command and verify response.
  return sendCheckReply(F("AT+FMVOLUME="), i, F("OK"));
}

int8_t SIM90X::getFMVolume() {
  uint16_t level;

  if (! sendParseReply(F("AT+FMVOLUME?"), F("+FMVOLUME: "), &level) ) return 0;

  return level;
}

int8_t SIM90X::getFMSignalLevel(uint16_t station) {
  // Fail if FM station is outside allowed range.
  if ((station < 875) || (station > 1080)) {
    return -1;
  }

  // Send FM signal level query command.
  // Note, need to explicitly send timeout so right overload is chosen.
  getReply(F("AT+FMSIGNAL="), station, SIM90X_DEFAULT_TIMEOUT_MS);
  // Check response starts with expected value.
  char *p = strstr_P(replybuffer, PSTR("+FMSIGNAL: "));
  if (p == 0) return -1;
  p+=11;
  // Find second colon to get start of signal quality.
  p = strchr(p, ':');
  if (p == 0) return -1;
  p+=1;
  // Parse signal quality.
  int8_t level = atoi(p);
  readline();  // eat the "OK"
  return level;
}

/********* PWM/BUZZER **************************************************/

boolean SIM90X::setPWM(uint16_t period, uint8_t duty) {
  if (period > 2000) return false;
  if (duty > 100) return false;

  return sendCheckReply(F("AT+SPWM=0,"), period, duty, F("OK"));
}

/********* CALL PHONES **************************************************/
boolean SIM90X::callPhone(char *number) {
  char sendbuff[35] = "ATD";
  strncpy(sendbuff+3, number, min(30, strlen(number)));
  uint8_t x = strlen(sendbuff);
  sendbuff[x] = ';';
  sendbuff[x+1] = 0;
  //Serial.println(sendbuff);

  return sendCheckReply(sendbuff, "OK");
}

boolean SIM90X::hangUp(void) {
  return sendCheckReply(F("ATH0"), F("OK"));
}

boolean SIM90X::pickUp(void) {
  return sendCheckReply(F("ATA"), F("OK"));
}

void SIM90X::onIncomingCall() {
  #ifdef SIM90X_DEBUG
  Serial.print(F("> ")); Serial.println(F("Incoming call..."));
  #endif
  SIM90X::_incomingCall = true;
}

boolean SIM90X::_incomingCall = false;

boolean SIM90X::callerIdNotification(boolean enable, uint8_t interrupt) {
  if(enable){
    attachInterrupt(interrupt, onIncomingCall, FALLING);
    return sendCheckReply(F("AT+CLIP=1"), F("OK"));
  }

  detachInterrupt(interrupt);
  return sendCheckReply(F("AT+CLIP=0"), F("OK"));
}

boolean SIM90X::incomingCallNumber(char* phonenum) {
  //+CLIP: "<incoming phone number>",145,"",0,"",0
  if(!SIM90X::_incomingCall)
    return false;

  readline();
  while(!strcmp_P(replybuffer, (prog_char*)F("RING")) == 0) {
    flushInput();
    readline();
  }

  readline(); //reads incoming phone number line

  parseReply(F("+CLIP: \""), phonenum, '"');

  #ifdef SIM90X_DEBUG
    Serial.print(F("Phone Number: "));
    Serial.println(replybuffer);
  #endif

  SIM90X::_incomingCall = false;
  return true;
}

/********* SMS **********************************************************/

uint8_t SIM90X::getSMSInterrupt(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CFGRI?"), F("+CFGRI: "), &reply) ) return 0;

  return reply;
}

boolean SIM90X::setSMSInterrupt(uint8_t i) {
  return sendCheckReply(F("AT+CFGRI="), i, F("OK"));
}

int8_t SIM90X::getNumSMS(void) {
  uint16_t numsms;

  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return -1;
  // ask how many sms are stored

  if (! sendParseReply(F("AT+CPMS?"), F("+CPMS: \"SM_P\","), &numsms) ) return -1;

  return numsms;
}

// Reading SMS's is a bit involved so we don't use helpers that may cause delays or debug
// printouts!
boolean SIM90X::readSMS(uint8_t i, char *smsbuff, 
			       uint16_t maxlen, uint16_t *readlen) {
  // text mode
  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return false;

  // show all text mode parameters
  if (! sendCheckReply(F("AT+CSDH=1"), F("OK"))) return false;

  // parse out the SMS len
  uint16_t thesmslen = 0;

  //getReply(F("AT+CMGR="), i, 1000);  //  do not print debug!
  mySerial->print(F("AT+CMGR="));
  mySerial->println(i);
  readline(1000); // timeout

  //Serial.print(F("Reply: ")); Serial.println(replybuffer);
  // parse it out...
  if (! parseReply(F("+CMGR:"), &thesmslen, ',', 11)) {
    *readlen = 0;
    return false;
  }

  readRaw(thesmslen);

  flushInput();

  uint16_t thelen = min(maxlen, strlen(replybuffer));
  strncpy(smsbuff, replybuffer, thelen);
  smsbuff[thelen] = 0; // end the string

#ifdef SIM90X_DEBUG
  Serial.println(replybuffer);
#endif
  *readlen = thelen;
  return true;
}

// Retrieve the sender of the specified SMS message and copy it as a string to
// the sender buffer.  Up to senderlen characters of the sender will be copied
// and a null terminator will be added if less than senderlen charactesr are
// copied to the result.  Returns true if a result was successfully retrieved,
// otherwise false.
boolean SIM90X::getSMSSender(uint8_t i, char *sender, int senderlen) {
  // Ensure text mode and all text mode parameters are sent.
  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return false;
  if (! sendCheckReply(F("AT+CSDH=1"), F("OK"))) return false;
  // Send command to retrieve SMS message and parse a line of response.
  mySerial->print(F("AT+CMGR="));
  mySerial->println(i);
  readline(1000);
  // Parse the second field in the response.
  boolean result = parseReplyQuoted(F("+CMGR:"), sender, senderlen, ',', 1);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

boolean SIM90X::sendSMS(char *smsaddr, char *smsmsg) {
  if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;

  char sendcmd[30] = "AT+CMGS=\"";
  strncpy(sendcmd+9, smsaddr, 30-9-2);  // 9 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';

  if (! sendCheckReply(sendcmd, "> ")) return false;
#ifdef SIM90X_DEBUG
  Serial.print(F("> ")); Serial.println(smsmsg);
#endif
  mySerial->println(smsmsg);
  mySerial->println();
  mySerial->write(0x1A);
#ifdef SIM90X_DEBUG
  Serial.println("^Z");
#endif
  readline(10000); // read the +CMGS reply, wait up to 10 seconds!!!
  //Serial.print("* "); Serial.println(replybuffer);
  if (strstr(replybuffer, "+CMGS") == 0) {
    return false;
  }
  readline(1000); // read OK
  //Serial.print("* "); Serial.println(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    return false;
  }

  return true;
}


boolean SIM90X::deleteSMS(uint8_t i) {
    if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  // read an sms
  char sendbuff[12] = "AT+CMGD=000";
  sendbuff[8] = (i / 100) + '0';
  i %= 100;
  sendbuff[9] = (i / 10) + '0';
  i %= 10;
  sendbuff[10] = i + '0';

  return sendCheckReply(sendbuff, "OK", 2000);
}

boolean SIM90X::deleteSMSs(uint8_t type){
  if (!sendCheckReply("AT+CMGF=1", "OK")) return -1;
  
  char t[14];
  char buffer[22];

  switch(type){
    case SIM90X_SMS_ALL:
      strcpy_P(t, PSTR("\"DEL ALL\""));
      break;
    case SIM90X_SMS_READ:
      strcpy_P(t, PSTR("\"DEL READ\""));
      break;
    case SIM90X_SMS_UNREAD:
      strcpy_P(t, PSTR("\"DEL UNREAD\""));
      break;
    case SIM90X_SMS_SENT:
      strcpy_P(t, PSTR("\"DEL SENT\""));
      break;
    case SIM90X_SMS_UNSENT:
      strcpy_P(t, PSTR("\"DEL UNSENT\""));
      break;
    case SIM90X_SMS_INBOX:
      strcpy_P(t, PSTR("\"DEL INBOX\""));
      break;
  }

  sprintf_P(buffer, "AT+CMGDA=%s", t);

  return sendCheckReply(buffer, "OK", 2000);
}

uint8_t SIM90X::hasSMS(uint8_t type){
  uint8_t i = 0;
  boolean status;

  mySerial->print(F("AT+CMGL="));
  switch(type){
    case SIM90X_SMS_ALL:
      mySerial->println(F("\"ALL\""));
      break;
    case SIM90X_SMS_UNREAD:
      mySerial->println(F("\"REC UNREAD\""));
      break;
    case SIM90X_SMS_READ:
      mySerial->println(F("\"REC READ\""));
      break;
  }

  readline();
  if(parseReplyQuoted(F("+CMGL:"), replybuffer, 8, ',', 0)){
    i = atoi(replybuffer);
  }

  // Drop any remaining data from the response.
  flushInput();
  return i;
}

/********* PHONEBOOK ****************************************************/

boolean SIM90X::getPhonebook(uint8_t addr, char *number, int number_length, char *name, int name_length) {
  boolean status;
  // Send command to retrieve PHONEBOOK item and parse a line of response.
  mySerial->print(F("AT+CPBR="));
  mySerial->println(addr);
  readline();
  // Parse the second field in the response.
  if((status = parseReplyQuoted(F("+CPBR:"), number, number_length, ',', 1))){
    parseReplyQuoted(F("+CPBR:"), name, name_length, ',', 3);
  }

  // Drop any remaining data from the response.
  flushInput();
  return status;
}

boolean SIM90X::getPhonebookNumber(uint8_t i, char *number, int length) {
  // Send command to retrieve PHONEBOOK item and parse a line of response.
  mySerial->print(F("AT+CPBR="));
  mySerial->println(i);
  readline(1000);
  // Parse the second field in the response.
  boolean result = parseReplyQuoted(F("+CPBR:"), number, length, ',', 1);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

boolean SIM90X::getPhonebookName(uint8_t i, char *name, int length) {
  // Send command to retrieve PHONEBOOK item and parse a line of response.
  mySerial->print(F("AT+CPBR="));
  mySerial->println(i);
  readline(1000);
  // Parse the second field in the response.
  boolean result = parseReplyQuoted(F("+CPBR:"), name, length, ',', 3);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

uint8_t SIM90X::hasPhonebookNumber(char *number) {
  uint8_t addr = 0;
  for(uint8_t i = 0; i < 20; i++){
    char buffer[24];

    mySerial->print(F("AT+CPBR="));
    mySerial->println(i + 1);
    readline(1000);
    if(parseReplyQuoted(F("+CPBR:"), buffer, 24, ',', 1)){
      if(strcmp(number, buffer) == 0) return (i + 1);
    }
    flushInput();
  }
  
  return addr;
}

/********* TIME **********************************************************/

boolean SIM90X::enableNetworkTimeSync(boolean onoff) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CLTS=1"), F("OK")))
      return false;
  } else {
    if (! sendCheckReply(F("AT+CLTS=0"), F("OK")))
      return false;
  }

  flushInput(); // eat any 'Unsolicted Result Code'

  return true;
}

boolean SIM90X::enableNTPTimeSync(boolean onoff, const __FlashStringHelper *ntpserver) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CNTPCID=1"), F("OK")))
      return false;

    mySerial->print(F("AT+CNTP=\""));
    if (ntpserver != 0) {
      mySerial->print(ntpserver);
    } else {
      mySerial->print(F("pool.ntp.org"));
    }
    mySerial->println(F("\",0"));
    readline(SIM90X_DEFAULT_TIMEOUT_MS);
    if (strcmp(replybuffer, "OK") != 0)
      return false;

    if (! sendCheckReply(F("AT+CNTP"), F("OK"), 10000))
      return false;

    uint16_t status;
    readline(10000);
    if (! parseReply(F("+CNTP:"), &status))
      return false;
  } else {
    if (! sendCheckReply(F("AT+CNTPCID=0"), F("OK")))
      return false;
  }

  return true;
}

boolean SIM90X::getTime(char *buff, uint16_t maxlen) {
  getReply(F("AT+CCLK?"), (uint16_t) 10000);
  if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
    return false;

  char *p = replybuffer+7;
  uint16_t lentocopy = min(maxlen-1, strlen(p));
  strncpy(buff, p, lentocopy+1);
  buff[lentocopy] = 0;

  readline(); // eat OK

  return true;
}

/********* GPRS **********************************************************/


boolean SIM90X::enableGPRS(boolean onoff) {

  if (onoff) {
    // disconnect all sockets
    sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000);

    if (! sendCheckReply(F("AT+CGATT=1"), F("OK"), 10000))
      return false;

    // set bearer profile! connection type GPRS
    if (! sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""),
			   F("OK"), 10000))
      return false;

    // set bearer profile access point name
    if (apn) {
      // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the configured APN value.
      if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), apn, F("OK"), 10000))
        return false;

      // set username/password
      if (apnusername) {
        // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the configured APN username.
        if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), apnusername, F("OK"), 10000))
          return false;
      }
      if (apnpassword) {
        // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the configured APN password.
        if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), apnpassword, F("OK"), 10000))
          return false;
      }
    }

    // open GPRS context
    if (! sendCheckReply(F("AT+SAPBR=1,1"), F("OK"), 10000))
      return false;
  } else {
    // disconnect all sockets
    if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000))
      return false;

    // close GPRS context
    if (! sendCheckReply(F("AT+SAPBR=0,1"), F("OK"), 10000))
      return false;

    if (! sendCheckReply(F("AT+CGATT=0"), F("OK"), 10000))
      return false;

  }
  return true;
}

uint8_t SIM90X::GPRSstate(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state) )
    return -1;

  return state;
}

void SIM90X::setGPRSNetworkSettings(const __FlashStringHelper *apn,
              const __FlashStringHelper *username, const __FlashStringHelper *password) {
  strcpy_P(this->apn, (const PROGMEM char *) apn);
  strcpy_P(this->apnusername, (const PROGMEM char *) apnusername);
  strcpy_P(this->apnpassword, (const PROGMEM char *) apnpassword);
}

void SIM90X::setGPRSNetworkSettings(char *apn, char *username, char *password) {
  this->apn = apn;
  this->apnusername = username;
  this->apnpassword = password;
}

boolean SIM90X::getGSMLoc(uint16_t *errorcode, char *buff, uint16_t maxlen) {

  getReply(F("AT+CIPGSMLOC=1,1"), (uint16_t)10000);

  if (! parseReply(F("+CIPGSMLOC: "), errorcode))
    return false;

  char *p = replybuffer+14;
  uint16_t lentocopy = min(maxlen-1, strlen(p));
  strncpy(buff, p, lentocopy+1);

  readline(); // eat OK

  return true;
}

/********* TCP FUNCTIONS  ************************************/


boolean SIM90X::TCPconnect(char *server, uint16_t port) {
  flushInput();

  // close all old connections
  if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000) ) return false;

  // single connection at a time
  if (! sendCheckReply(F("AT+CIPMUX=0"), F("OK")) ) return false;

  // manually read data
  if (! sendCheckReply(F("AT+CIPRXGET=1"), F("OK")) ) return false;

#ifdef SIM90X_DEBUG
  Serial.print(F("AT+CIPSTART=\"TCP\",\""));
  Serial.print(server);
  Serial.print(F("\",\""));
  Serial.print(port);
  Serial.println(F("\""));
#endif

  mySerial->print(F("AT+CIPSTART=\"TCP\",\""));
  mySerial->print(server);
  mySerial->print(F("\",\""));
  mySerial->print(port);
  mySerial->println(F("\""));

  if (! expectReply(F("OK"))) return false;
  if (! expectReply(F("CONNECT OK"))) return false;
}

boolean SIM90X::TCPclose(void) {
  return sendCheckReply(F("AT+CIPCLOSE"), F("OK"));
}

boolean SIM90X::TCPconnected(void) {
  if (! sendCheckReply(F("AT+CIPSTATUS"), F("OK"), 100) ) return false;
  readline(100);
#ifdef SIM90X_DEBUG
  Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return (strcmp(replybuffer, "STATE: CONNECT OK") == 0);
}

boolean SIM90X::TCPsend(char *packet, uint8_t len) {

#ifdef SIM90X_DEBUG
  Serial.print(F("AT+CIPSEND="));
  Serial.println(len);

  for (uint16_t i=0; i<len; i++) {
    Serial.print(" 0x");
    Serial.print(packet[i], HEX);
  }
  Serial.println();
#endif


  mySerial->print(F("AT+CIPSEND="));
  mySerial->println(len);
  readline();
#ifdef SIM90X_DEBUG
  Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  if (replybuffer[0] != '>') return false;

  mySerial->write(packet, len);
  readline(3000); // wait up to 3 seconds to send the data
#ifdef SIM90X_DEBUG
  Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
    
  return (strcmp(replybuffer, "SEND OK") == 0);
}

uint16_t SIM90X::TCPavailable(void) {
  uint16_t avail;

  if (! sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0) ) return false;

#ifdef SIM90X_DEBUG
  Serial.print (avail); Serial.println(F(" bytes available"));
#endif
    
  return avail;
}


uint16_t SIM90X::TCPread(uint8_t *buff, uint8_t len) {
  uint16_t avail;

  mySerial->print(F("AT+CIPRXGET=2,"));
  mySerial->println(len);
  readline();
  if (! parseReply(F("+CIPRXGET: 2,"), &avail, ',', 0)) return false;

  readRaw(avail);

#ifdef SIM90X_DEBUG
  Serial.print (avail); Serial.println(F(" bytes read"));
  for (uint8_t i=0;i<avail;i++) {
    Serial.print(" 0x"); Serial.print(replybuffer[i], HEX);
  }
  Serial.println(); 
#endif

  memcpy(buff, replybuffer, avail);
    
  return avail;
}



/********* HTTP LOW LEVEL FUNCTIONS  ************************************/

boolean SIM90X::HTTP_init() {
  return sendCheckReply(F("AT+HTTPINIT"), F("OK"));
}

boolean SIM90X::HTTP_term() {
  return sendCheckReply(F("AT+HTTPTERM"), F("OK"));
}

void SIM90X::HTTP_para_start(const __FlashStringHelper *parameter,
                                    boolean quoted) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); 
  Serial.print(F("AT+HTTPPARA=\""));
  Serial.print(parameter);
  Serial.println('"');
#endif

  mySerial->print(F("AT+HTTPPARA=\""));
  mySerial->print(parameter);
  if (quoted)
    mySerial->print(F("\",\""));
  else
    mySerial->print(F("\","));
}

boolean SIM90X::HTTP_para_end(boolean quoted) {
  if (quoted)
    mySerial->println('"');
  else
    mySerial->println();

  return expectReply(F("OK"));
}

boolean SIM90X::HTTP_para(const __FlashStringHelper *parameter, 
                                 const char *value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

boolean SIM90X::HTTP_para(const __FlashStringHelper *parameter, 
                                 const __FlashStringHelper *value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

boolean SIM90X::HTTP_para(const __FlashStringHelper *parameter, 
                                 int32_t value) {
  HTTP_para_start(parameter, false);
  mySerial->print(value);
  return HTTP_para_end(false);
}

boolean SIM90X::HTTP_data(uint32_t size, uint32_t maxTime) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); 
  Serial.print(F("AT+HTTPDATA="));
  Serial.print(size);
  Serial.print(",");
  Serial.println(maxTime);
#endif

  mySerial->print(F("AT+HTTPDATA="));
  mySerial->print(size);
  mySerial->print(",");
  mySerial->println(maxTime);

  return expectReply(F("DOWNLOAD"));
}

boolean SIM90X::HTTP_action(uint8_t method, uint16_t *status, 
                                   uint16_t *datalen, int32_t timeout) {
  // Send request.
  if (! sendCheckReply(F("AT+HTTPACTION="), method, F("OK")))
    return false;

  // Parse response status and size.
  readline(timeout);
  if (! parseReply(F("+HTTPACTION:"), status, ',', 1))
    return false;
  if (! parseReply(F("+HTTPACTION:"), datalen, ',', 2))
    return false;

  return true;
}

boolean SIM90X::HTTP_readall(uint16_t *datalen) {
  getReply(F("AT+HTTPREAD"));
  if (! parseReply(F("+HTTPREAD:"), datalen, ',', 0))
    return false;

  return true;
}

boolean SIM90X::HTTP_ssl(boolean onoff) {
  return sendCheckReply(F("AT+HTTPSSL="), onoff ? 1 : 0, F("OK"));
}

/********* HTTP HIGH LEVEL FUNCTIONS ***************************/

boolean SIM90X::HTTP_GET_start(char *url,
              uint16_t *status, uint16_t *datalen){
  if (! HTTP_setup(url))
    return false;

  // HTTP GET
  if (! HTTP_action(SIM90X_HTTP_GET, status, datalen))
    return false;

#ifdef SIM90X_DEBUG
  Serial.print("Status: "); Serial.println(*status);
  Serial.print("Len: "); Serial.println(*datalen);
#endif

  // HTTP response data
  if (! HTTP_readall(datalen))
    return false;

  return true;
}

void SIM90X::HTTP_GET_end(void) {
  HTTP_term();
}

boolean SIM90X::HTTP_POST_start(char *url,
              const __FlashStringHelper *contenttype,
              const uint8_t *postdata, uint16_t postdatalen,
              uint16_t *status, uint16_t *datalen){
  if (! HTTP_setup(url))
    return false;

  if (! HTTP_para(F("CONTENT"), contenttype)) {
    return false;
  }

  // HTTP POST data
  if (! HTTP_data(postdatalen, 10000))
    return false;
  mySerial->write(postdata, postdatalen);
  if (! expectReply(F("OK")))
    return false;

  // HTTP POST
  if (! HTTP_action(SIM90X_HTTP_POST, status, datalen))
    return false;

#ifdef SIM90X_DEBUG
  Serial.print("Status: "); Serial.println(*status);
  Serial.print("Len: "); Serial.println(*datalen);
#endif

  // HTTP response data
  if (! HTTP_readall(datalen))
    return false;

  return true;
}

void SIM90X::HTTP_POST_end(void) {
  HTTP_term();
}

void SIM90X::setUserAgent(const __FlashStringHelper *useragent) {
  this->useragent = useragent;
}

void SIM90X::setHTTPSRedirect(boolean onoff) {
  httpsredirect = onoff;
}

/********* HTTP HELPERS ****************************************/

boolean SIM90X::HTTP_setup(char *url) {
  // Handle any pending
  HTTP_term();

  // Initialize and set parameters
  if (! HTTP_init())
    return false;
  if (! HTTP_para(F("CID"), 1))
    return false;
  if (! HTTP_para(F("UA"), useragent))
    return false;
  if (! HTTP_para(F("URL"), url))
    return false;

  // HTTPS redirect
  if (httpsredirect) {
    if (! HTTP_para(F("REDIR"),1))
      return false;

    if (! HTTP_ssl(true))
      return false;
  }

  return true;
}

/********* HELPERS *********************************************/

boolean SIM90X::expectReply(const __FlashStringHelper *reply,
                                   uint16_t timeout) {
  readline(timeout);
#ifdef SIM90X_DEBUG
  Serial.print(F("\t<--- ")); Serial.println(replybuffer);
#endif
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

/********* LOW LEVEL *******************************************/

inline int SIM90X::available(void) {
  return mySerial->available();
}

inline size_t SIM90X::write(uint8_t x) {
  return mySerial->write(x);
}

inline int SIM90X::read(void) {
  return mySerial->read();
}

inline int SIM90X::peek(void) {
  return mySerial->peek();
}

inline void SIM90X::flush() {
  mySerial->flush();
}

void SIM90X::flushInput() {
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40) {
        while(available()) {
            read();
            timeoutloop = 0;  // If char was received reset the timer
        }
        delay(1);
    }
}

uint16_t SIM90X::readRaw(uint16_t b) {
  uint16_t idx = 0;

  while (b && (idx < sizeof(replybuffer)-1)) {
    if (mySerial->available()) {
      replybuffer[idx] = mySerial->read();
      idx++;
      b--;
    }
  }
  replybuffer[idx] = 0;

  return idx;
}

uint8_t SIM90X::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      //Serial.println(F("SPACE"));
      break;
    }

    while(mySerial->available()) {
      char c =  mySerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }

    if (timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

uint8_t SIM90X::getReply(char *send, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
    Serial.print("\t---> "); Serial.println(send);
#endif

  mySerial->println(send);

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

uint8_t SIM90X::getReply(const __FlashStringHelper *send, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.println(send);
#endif

  mySerial->println(send);

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t SIM90X::getReply(const __FlashStringHelper *prefix, char *suffix, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.print(prefix); Serial.println(suffix);
#endif

  mySerial->print(prefix);
  mySerial->println(suffix);

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t SIM90X::getReply(const __FlashStringHelper *prefix, int32_t suffix, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.print(prefix); Serial.println(suffix, DEC);
#endif

  mySerial->print(prefix);
  mySerial->println(suffix, DEC);

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t SIM90X::getReply(const __FlashStringHelper *prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.print(prefix);
  Serial.print(suffix1, DEC); Serial.print(","); Serial.println(suffix2, DEC);
#endif

  mySerial->print(prefix);
  mySerial->print(suffix1);
  mySerial->print(',');
  mySerial->println(suffix2, DEC);

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set replybuffer with response).
uint8_t SIM90X::getReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.print(prefix);
  Serial.print('"'); Serial.print(suffix); Serial.println('"');
#endif

  mySerial->print(prefix);
  mySerial->print('"');
  mySerial->print(suffix);
  mySerial->println('"');

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}

uint8_t SIM90X::getReplyQuoted(const __FlashStringHelper *prefix, const char *suffix, uint16_t timeout) {
  flushInput();

#ifdef SIM90X_DEBUG
  Serial.print("\t---> "); Serial.print(prefix);
  Serial.print('"'); Serial.print(suffix); Serial.println('"');
#endif

  mySerial->print(prefix);
  mySerial->print('"');
  mySerial->print(suffix);
  mySerial->println('"');

  uint8_t l = readline(timeout);
#ifdef SIM90X_DEBUG
    Serial.print (F("\t<--- ")); Serial.println(replybuffer);
#endif
  return l;
}


boolean SIM90X::sendCheckReply(char *send, char *reply, uint16_t timeout) {
  getReply(send, timeout);

/*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
    Serial.print(replybuffer[i], HEX); Serial.print(" ");
  }
  Serial.println();
  for (uint8_t i=0; i<strlen(reply); i++) {
    Serial.print(reply[i], HEX); Serial.print(" ");
  }
  Serial.println();
  */
  return (strcmp(replybuffer, reply) == 0);
}

boolean SIM90X::sendCheckReply(const __FlashStringHelper *send, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(send, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
boolean SIM90X::sendCheckReply(const __FlashStringHelper *prefix, char *suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
boolean SIM90X::sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, suffix2, and newline.  Verify FONA response matches reply parameter.
boolean SIM90X::sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix1, int32_t suffix2, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix1, suffix2, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify FONA response matches reply parameter.
boolean SIM90X::sendCheckReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

boolean SIM90X::sendCheckReplyQuoted(const __FlashStringHelper *prefix, const char *suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

boolean SIM90X::parseReply(const __FlashStringHelper *toreply,
          uint16_t *v, char divider, uint8_t index) {
  char *p = strstr_P(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);
  //Serial.println(p);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
    //Serial.println(p);

  }
  *v = atoi(p);

  return true;
}

boolean SIM90X::parseReply(const __FlashStringHelper *toreply,
          char *v, char divider, uint8_t index) {
  uint8_t i=0;
  char *p = strstr_P(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);

  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  for(i=0; i<strlen(p);i++) {
    if(p[i] == divider)
      break;
    v[i] = p[i];
  }

  v[i] = '\0';

  return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean SIM90X::parseReplyQuoted(const __FlashStringHelper *toreply,
          char *v, int maxlen, char divider, uint8_t index) {
  uint8_t i=0, j;
  // Verify response starts with toreply.
  char *p = strstr_P(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);

  // Find location of desired response field.
  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  // Copy characters from response field into result string.
  for(i=0, j=0; j<maxlen && i<strlen(p); ++i) {
    // Stop if a divier is found.
    if(p[i] == divider)
      break;
    // Skip any quotation marks.
    else if(p[i] == '"')
      continue;
    v[j++] = p[i];
  }

  // Add a null terminator if result string buffer was not filled.
  if (j < maxlen)
    v[j] = '\0';

  return true;
}

boolean SIM90X::sendParseReply(const __FlashStringHelper *tosend,
				      const __FlashStringHelper *toreply,
				      uint16_t *v, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, v, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}
