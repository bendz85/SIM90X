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

#if (ARDUINO >= 100)
	#include "Arduino.h"
	#include <SoftwareSerial.h>
#else
	#include "WProgram.h"
	#include <NewSoftSerial.h>
#endif

// #define SIM90X_DEBUG

#define FONA_HEADSETAUDIO 0
#define FONA_EXTAUDIO 1

#define FONA_STTONE_DIALTONE 1
#define FONA_STTONE_BUSY 2
#define FONA_STTONE_CONGESTION 3
#define FONA_STTONE_PATHACK 4
#define FONA_STTONE_DROPPED 5
#define FONA_STTONE_ERROR 6
#define FONA_STTONE_CALLWAIT 7
#define FONA_STTONE_RINGING 8
#define FONA_STTONE_BEEP 16
#define FONA_STTONE_POSTONE 17
#define FONA_STTONE_ERRTONE 18
#define FONA_STTONE_INDIANDIALTONE 19
#define FONA_STTONE_USADIALTONE 20

#define FONA_DEFAULT_TIMEOUT_MS 500

#define FONA_SMS_ALL 0
#define FONA_SMS_READ 1
#define FONA_SMS_UNREAD 2
#define FONA_SMS_SENT 3
#define FONA_SMS_UNSENT 4
#define FONA_SMS_INBOX 5

class SIM90X : public Stream {
 public:
  SIM90X();
  SIM90X(int8_t r);
  boolean begin(Stream &port);

  // Stream
  int available(void);
  size_t write(uint8_t x);
  int read(void);
  int peek(void);
  void flush();

  // Raw
  void AT(char *cmd, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

  // RTC
  boolean enableRTC(uint8_t i);
  boolean readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec);

  // Battery and ADC
  boolean getADCVoltage(uint16_t *v);
  boolean getBattPercent(uint16_t *p);
  boolean getBattVoltage(uint16_t *v);

  // SIM query
  uint8_t unlockSIM(char *pin);
  uint8_t getSIMCCID(char *ccid);
  uint8_t getNetworkStatus(void);
  uint8_t getRSSI(void);

  // IMEI
  uint8_t getIMEI(char *imei);

  // Phone Book
  boolean getPhonebook(uint8_t addr, char *number, int number_length, char *name, int name_length);
  boolean getPhonebookNumber(uint8_t i, char *number, int length);
  boolean getPhonebookName(uint8_t i, char *name, int length);
  uint8_t hasPhonebookNumber(char *number);

  // set Audio output
  boolean setAudio(uint8_t a);
  boolean setVolume(uint8_t i);
  uint8_t getVolume(void);
  boolean playToolkitTone(uint8_t t, uint16_t len);
  boolean setMicVolume(uint8_t a, uint8_t level);

  // SMS handling
  boolean setSMSInterrupt(uint8_t i);
  uint8_t getSMSInterrupt(void);
  int8_t getNumSMS(void);
  boolean readSMS(uint8_t i, char *smsbuff, uint16_t max, uint16_t *readsize);
  boolean sendSMS(char *smsaddr, char *smsmsg);
  boolean deleteSMS(uint8_t i);
  boolean deleteSMSs(uint8_t type);
  boolean getSMSSender(uint8_t i, char *sender, int senderlen);
  uint8_t hasSMS(uint8_t type);

  // Time
  boolean enableNetworkTimeSync(boolean onoff);
  boolean getTime(char *buff, uint16_t maxlen);

  // GPRS handling
  boolean enableGPRS(boolean onoff);
  uint8_t GPRSstate(void);
  void setGPRSNetworkSettings(char *apn, char *username = 0, char *password = 0);
  void setGPRSNetworkSettings(const __FlashStringHelper *apn, const __FlashStringHelper *username=0, const __FlashStringHelper *password=0);

  // HTTP 
  boolean HTTP_GET_start(char *url, uint16_t *status, uint16_t *datalen);
  void HTTP_GET_end(void);
  boolean HTTP_POST_start(char *url, const __FlashStringHelper *contenttype, const uint8_t *postdata, uint16_t postdatalen,  uint16_t *status, uint16_t *datalen);
  void HTTP_POST_end(void);
  void setUserAgent(const __FlashStringHelper *useragent);

  // HTTPS
  void setHTTPSRedirect(boolean onoff);

  // Phone calls
  boolean callPhone(char *phonenum);
  boolean hangUp(void);
  boolean pickUp(void);
  boolean callerIdNotification(boolean enable, uint8_t interrupt = 0);
  boolean incomingCallNumber(char* phonenum);

 private:
  int8_t _rstpin;

  char replybuffer[255];
  //const __FlashStringHelper *apn;
  //const __FlashStringHelper *apnusername;
  //const __FlashStringHelper *apnpassword;
  char *apn;
  char *apnusername;
  char *apnpassword;

  boolean httpsredirect;
  const __FlashStringHelper *useragent;

  // HTTP helpers
  boolean HTTP_initialize(char *url);
  boolean HTTP_response(uint16_t *status, uint16_t *datalen);
  void HTTP_terminate(void);

  void flushInput();
  uint16_t readRaw(uint16_t b);
  uint8_t readline(uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS, boolean multiline = false);
  uint8_t getReply(char *send, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(const __FlashStringHelper *send, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(const __FlashStringHelper *prefix, char *suffix, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(const __FlashStringHelper *prefix, int32_t suffix, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(const __FlashStringHelper *prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout); // Don't set default value or else function call is ambiguous.
  uint8_t getReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReplyQuoted(const __FlashStringHelper *prefix, const char *suffix, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

  boolean sendCheckReply(char *send, char *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(const __FlashStringHelper *send, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(const __FlashStringHelper *prefix, char *suffix, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix, int32_t suffix2, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReplyQuoted(const __FlashStringHelper *prefix, const char *suffix, const __FlashStringHelper *reply, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

  boolean parseReply(const __FlashStringHelper *toreply, uint16_t *v, char divider  = ',', uint8_t index=0);
  boolean parseReply(const __FlashStringHelper *toreply, char *v, char divider  = ',', uint8_t index=0);
  boolean parseReplyQuoted(const __FlashStringHelper *toreply, char *v, int maxlen, char divider, uint8_t index);

  boolean sendParseReply(const __FlashStringHelper *tosend, const __FlashStringHelper *toreply, uint16_t *v, char divider = ',', uint8_t index=0);

  static boolean _incomingCall;
  static void onIncomingCall();

  Stream *mySerial;
};