#include "EventPacket.h"
#include <cstring>  // for memcpy

#if defined(__APPLE__)
  #include <ApplicationServices/ApplicationServices.h>

  void injectKeyPress(const EventPacket& pkt) {
      uint32_t code;
      std::memcpy(&code, pkt.payload.data(), sizeof(code));
      CGEventRef eDown = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, true);
      CGEventRef eUp   = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, false);
      CGEventPost(kCGHIDEventTap, eDown);
      CGEventPost(kCGHIDEventTap, eUp);
      CFRelease(eDown);
      CFRelease(eUp);
  }

  void injectMouseMove(const EventPacket& pkt) {
      int32_t coords[2];
      std::memcpy(coords, pkt.payload.data(), sizeof(coords));
      CGPoint pt = CGPointMake(coords[0], coords[1]);
      CGEventRef e = CGEventCreateMouseEvent(
          NULL, kCGEventMouseMoved, pt, kCGMouseButtonLeft);
      CGEventPost(kCGHIDEventTap, e);
      CFRelease(e);
  }

#elif defined(_WIN32)
  #include <Windows.h>

  void injectKeyPress(const EventPacket& pkt) {
      uint32_t code;
      std::memcpy(&code, pkt.payload.data(), sizeof(code));
      INPUT inputs[2] = {};
      inputs[0].type            = INPUT_KEYBOARD;
      inputs[0].ki.wVk          = (WORD)code;
      inputs[0].ki.dwFlags      = 0;           // key down
      inputs[1].type            = INPUT_KEYBOARD;
      inputs[1].ki.wVk          = (WORD)code;
      inputs[1].ki.dwFlags      = KEYEVENTF_KEYUP;  // key up
      SendInput(2, inputs, sizeof(INPUT));
  }

  void injectMouseMove(const EventPacket& pkt) {
      int32_t coords[2];
      std::memcpy(coords, pkt.payload.data(), sizeof(coords));
      // Convert to absolute [0,65535] range
      INPUT input = {};
      input.type                = INPUT_MOUSE;
      input.mi.dwFlags          = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
      input.mi.dx               = coords[0] * (65535 / GetSystemMetrics(SM_CXSCREEN));
      input.mi.dy               = coords[1] * (65535 / GetSystemMetrics(SM_CYSCREEN));
      SendInput(1, &input, sizeof(input));
  }

#endif
