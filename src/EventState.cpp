#include "EventState.h"

// Global flag to track whether an event is from real user input or injection
bool isInjectedEvent = false;

// Function to set the injected event flag
void setInjectedEventFlag(bool value) {
    isInjectedEvent = value;
} 