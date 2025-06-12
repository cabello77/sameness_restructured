#pragma once

// Global flag to track whether an event is from real user input or injection
extern bool isInjectedEvent;

// Function to set the injected event flag
void setInjectedEventFlag(bool value); 