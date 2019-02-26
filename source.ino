#include <Nextion.h>
#include <NextionPage.h>
#include <NextionText.h>
#include <NextionButton.h>
#include <NextionSlider.h>
#include <SoftwareSerial.h>
#include "SIM900.h"
#include "GSM.h"
#include "call.h"
#include "sms.h"

// Software Serial Objects 
SoftwareSerial SIM900(7, 8); // RX, TX
SoftwareSerial nextionSerial(10, 11); // RX, TX

// NeoNextion objects used to control the LCD
Nextion nex(nextionSerial);
NextionButton caller(nex, 1, 13, "b1");
NextionButton callerHangUp(nex, 4, 1, "b0");
NextionButton sendSmsPg2(nex, 2, 44, "b42");
NextionButton sendSmsPg3(nex, 3, 42, "b42");
NextionButton answer(nex, 5, 1, "b0");
NextionText t0Page2(nex, 2, 43, "t0");
NextionText t1Page2(nex, 2, 45, "t1");
NextionText t2Page2(nex, 2, 52, "t2");
NextionText t3Page2(nex, 2, 54, "t3");
NextionText t4Page2(nex, 2, 55, "t4");
NextionText t5Page2(nex, 2, 57, "t5");
NextionText t0Page3(nex, 3, 41, "t0");
NextionText t1Page3(nex, 3, 43, "t1");
NextionText t2Page3(nex, 3, 48, "t2");
NextionText t3Page3(nex, 3, 51, "t3");
NextionText t4Page3(nex, 3, 49, "t4");
NextionText t5Page3(nex, 3, 50, "t5");
NextionText text(nex, 4, 2, "t0");
NextionSlider brightness(nex, 6, 3, "h0");

// Other variables used during the cycle of the program
CallGSM call;
SMSGSM sms;
long long start = millis();
int value = 0;
int valueOld = 1;
bool flagAttachedCallerHangUp = false;
bool flagAttachedSmsCallback2 = false;
bool flagAttachedSmsCallback3 = false;
bool flagAttachedAnswer = false;
int smsPos = 0;
int numdata;
bool started=false;
char smsbuffer[160];
char n[20];
char incomingChar=0;
char smsPostion;
char phoneNumber[20];
char smsText[100];
int i;

void setup() {
  // For GSM900 Shield
  if (gsm.begin(9600))
    Serial.println("\nstatus=READY");
  else 
    Serial.println("\nstatus=IDLE");
  // For serial monitor
  Serial.begin(9600); 
  // For Nextion Display
  nextionSerial.begin(9600);
  nex.init();
}

void loop() {
  // For debugging to see which page Nextion 
  // display send to Arduino Uno
  uint8_t currentPage = nex.getCurrentPage();
  Serial.println(currentPage);
  
  // If the Nextion display is on page 4 it means
  // that the user wants to call someone
  if (currentPage == 4) {
    calling();
  }
  
  // If the Nextion display is on page 2 or page 3
  // it means that the user wants to receive or 
  // send a sms
  if (currentPage == 2) {
    if (!flagAttachedSmsCallback2) {
      flagAttachedSmsCallback2 = true;
      sendSmsPg2.attachCallback(&callbackSmsSend2);
    }
    if (millis() - start >= 180000) {
      readSMS();
      start = millis();
    }
  }
  if (currentPage == 3) {
    readSMS();
    if (!flagAttachedSmsCallback3) {
      flagAttachedSmsCallback3 = true;
      sendSmsPg3.attachCallback(&callbackSmsSend3);
    }
  }

  // If the Nextion display is on page 6 it means
  // that the user want to change the brightness 
  // of the LCD
  if (currentPage == 6) {
    brightness.setValue(nex.getBrightness());
    uint16_t valBright = brightness.getValue();
    if (valBright > 20) {
      nex.setBrightness(valBright, true);
    }
  }

  // Polls for new messages and touch events from 
  // Nextion display
  nex.poll();
}

/*
 * Function calling is used to call a pearson if 
 * the Nextion display is on page 4. The function
 * is using the SIM900 Library
 */
void calling() {
  if (!flagAttachedCallerHangUp) {
      flagAttachedCallerHangUp = true;
      callerHangUp.attachCallback(&callbackHangUp);
    }
    if (value != valueOld) {
      char *phoneNumber = new char[20];
      phoneNumber[0] = NULL;
      strcat(phoneNumber, "+4");
      text.getText(phoneNumber + 2, 20);
      Serial.println(phoneNumber);
      if (call.CallStatus() != CALL_ACTIVE_VOICE) {
        call.Call(phoneNumber);
        delay(1000);
      }
      else {
        Serial.println("Active call");
        if (value == 0) {
          call.SendDTMF("0", 1);
          delay(5000);
        }
        else {
          call.SendDTMF("0, 0", 1);
          delay(5000);
        }
      }
      
      delete[] phoneNumber; 
    }
    
    
  if (call.CallStatus() != CALL_ACTIVE_VOICE) {
    Serial.println("Update...");
    valueOld = value;
    delay(1000);
  }
}

/*
 * Function readSMS is used to receive a SMS from 
 * the GSM operator and read the stack of SMS from
 * SimCard
 */
void readSMS() {
  if (gsm.begin(9600))
    Serial.println("\nstatus=READY");
  else 
    Serial.println("\nstatus=IDLE");
  
  for (int i = 0; i < 10; ++i) {
    smsPostion = sms.IsSMSPresent(SMS_UNREAD);
    if (smsPostion)  {
      // read new SMS
      Serial.print("SMS postion:");
      Serial.println(smsPostion,DEC);
      sms.GetSMS(smsPostion, phoneNumber, smsText, 100);
      // now we have phone number string in phoneNUM
      Serial.println(phoneNumber);
      // and SMS text in smsText
      Serial.println(smsText);
      char _tmp[250];
      _tmp[0] = NULL;
      strcat(_tmp, phoneNumber);
      strcat(_tmp, "  ");
      strcat(_tmp, smsText);
      
      if (smsPos == 0) {
        nex.sendCommand("vis t2,1");
        nex.sendCommand("vis p0,1");
        t2Page2.setText(_tmp);
        smsPos++;
      }
      else if (smsPos == 1) {
        nex.sendCommand("vis t5,1");
        nex.sendCommand("vis p1,1");
        t5Page2.setText(_tmp);
        smsPos++;
      }
      else if (smsPos == 2) {
        _tmp[0] = NULL;
        t5Page2.getText(_tmp, 200);
        t2Page2.setText(_tmp);
        _tmp[0] = NULL;
        strcat(_tmp, phoneNumber);
        strcat(_tmp, "  ");
        strcat(_tmp, smsText);
        t5Page2.setText(_tmp);
      }
    }   
    else {
      Serial.println("NO NEW SMS,WAITTING");
    }     
    delay(1000);
  }
  nextionSerial.begin(9600);
}

/*
 * Function callbackHangUp is used by Nextion display
 * to respond to an event when an user wants to end a 
 * conversation. The principle is like Node.JS.
 */
void callbackHangUp(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    Serial.println("Hang Up");
    call.HangUp();
    delay(1000);
    valueOld = 1;
  }
}

/*
 * Functions callbackSmsSend2 and callbackSmsSend3 are used by Nextion
 * display to respond to an event when an user wants to send a message. 
 */
void callbackSmsSend2(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    char *phoneNumber = new char[21];
    char *smsText = new char[121];
    phoneNumber[0] = NULL;
    smsText[0] = NULL;
    strcat(phoneNumber, "+4");
    t0Page2.getText(smsText, 120);
    t1Page2.getText(phoneNumber + 2, 20);
    Serial.println(phoneNumber);
    Serial.println(smsText);
    sendSMS(phoneNumber, smsText);
    
    delay(5000);
    delete[] phoneNumber;
    delete[] smsText;
  }
}

void callbackSmsSend3(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    char *phoneNumber = new char[21];
    char *smsText = new char[121];
    phoneNumber[0] = NULL;
    smsText[0] = NULL;
    strcat(phoneNumber, "+4");
    t0Page3.getText(smsText, 121);
    t1Page3.getText(phoneNumber + 2, 20);
    Serial.println(smsText);
    Serial.println(phoneNumber);
    sendSMS(phoneNumber, smsText);
    delay(5000);
    delete[] phoneNumber;
    delete[] smsText;
  }
}

/*
 * A helpful function used by other function to connect to GSM operator
 * and send an message.
 */
void sendSMS(char *phoneNumber, char *smsText) {
  if (gsm.begin(9600))
    Serial.println("\nstatus=READY");
  else 
    Serial.println("\nstatus=IDLE");
  if (sms.SendSMS(phoneNumber, smsText))
    Serial.println("\nSMS sent OK");
  nextionSerial.begin(9600);
}

/*
 * Function answerCallback is not used in the current version 
 * of this project. The scope was to make the phone to answer
 * when it receives a call.
 */
void answerCallback(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    SIM900.print("ATA\r");
    nex.sendCommand("page 4");
    nextionSerial.begin(9600);
  }
}
