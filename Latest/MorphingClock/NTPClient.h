#ifndef __NTP_CLIENT_H__
#define __NTP_CLIENT_H__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PxMatrix.h>

extern char timezone[5]; 
extern bool military;    

class NTPClient {
  public:
    NTPClient();
    
    /**
     * @brief Instantiates the network layer, connects via WiFiManager, loads FS preferences.
     */
    void Setup(PxMATRIX* d);

    /**
     * @brief Checks structural sync intervals and handles live Unix clock counters.
     * @param force Explicit bypass parameter to re-query the cloud servers immediately.
     */
    unsigned long GetCurrentTime(bool force);
    
    unsigned long sendNTPpacket(IPAddress& address);
    byte GetHours();
    byte GetMinutes();
    byte GetSeconds();
    void PrintTime();
    
  private:
    PxMATRIX* _display;
    unsigned long ReadCurrentEpoch();
    void AskCurrentEpoch();
};

#endif // __NTP_CLIENT_H__