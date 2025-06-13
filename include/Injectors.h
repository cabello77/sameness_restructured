#pragma once
#include "EventPacket.h"

void injectKeyPress(const EventPacket&);
void injectKeyRelease(const EventPacket&);
void injectMouseMove(const EventPacket&);
void injectMouseButtonPress(const EventPacket&);
void injectMouseButtonRelease(const EventPacket&); 