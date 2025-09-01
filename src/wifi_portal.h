#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include <WebServer.h>
#include <Preferences.h>
#include <Arduino.h>
extern Preferences preferences;
extern String ssid;
extern String password;
extern bool wifiOk;
extern WebServer server;

void setupWifiPortal();
void handleWifiPortal();

#endif
#pragma once
#include <WebServer.h>
#include <Preferences.h>

extern WebServer server;
extern Preferences preferences;
extern bool wifiOk;
extern String ssid;
extern String password;

// Timer duration variables (defined in main.cpp)
extern unsigned long ledOnDuration;
extern unsigned long ledOffDuration;
extern unsigned long pumpOnDuration;
extern unsigned long pumpOffDuration;

void setupWifiPortal();
void handleWifiPortal();
void setTimerDurations(unsigned long ledOn, unsigned long ledOff, unsigned long pumpOn, unsigned long pumpOff);
