#include "EventPacket.h"
#include <cstring>  // for memcpy
#include "EventState.h"
#include <uiohook.h>

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

void injectKeyRelease(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t)) {
        return;  // Invalid payload size
    }
    
    uint16_t keycode;
    std::memcpy(&keycode, pkt.payload.data(), sizeof(keycode));
    
    setInjectedEventFlag(true);
    uiohook_event event = {
        .type = EVENT_KEY_RELEASED,
        .time = pkt.timestamp,
        .mask = 0,
        .data = {
            .keyboard = {
                .keycode = keycode,
                .rawcode = keycode,
                .keychar = 0
            }
        }
    };
    hook_post_event(&event);
    setInjectedEventFlag(false);
}

void injectMouseButtonPress(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
        return;  // Invalid payload size
    }
    
    uint16_t button;
    int16_t x, y;
    size_t offset = 0;
    
    std::memcpy(&button, pkt.payload.data() + offset, sizeof(button));
    offset += sizeof(button);
    std::memcpy(&x, pkt.payload.data() + offset, sizeof(x));
    offset += sizeof(x);
    std::memcpy(&y, pkt.payload.data() + offset, sizeof(y));
    
    setInjectedEventFlag(true);
    uiohook_event event = {
        .type = EVENT_MOUSE_PRESSED,
        .time = pkt.timestamp,
        .mask = 0,
        .data = {
            .mouse = {
                .x = x,
                .y = y,
                .clicks = 1,
                .button = button
            }
        }
    };
    hook_post_event(&event);
    setInjectedEventFlag(false);
}

void injectMouseButtonRelease(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
        return;  // Invalid payload size
    }
    
    uint16_t button;
    int16_t x, y;
    size_t offset = 0;
    
    std::memcpy(&button, pkt.payload.data() + offset, sizeof(button));
    offset += sizeof(button);
    std::memcpy(&x, pkt.payload.data() + offset, sizeof(x));
    offset += sizeof(x);
    std::memcpy(&y, pkt.payload.data() + offset, sizeof(y));
    
    setInjectedEventFlag(true);
    uiohook_event event = {
        .type = EVENT_MOUSE_RELEASED,
        .time = pkt.timestamp,
        .mask = 0,
        .data = {
            .mouse = {
                .x = x,
                .y = y,
                .clicks = 1,
                .button = button
            }
        }
    };
    hook_post_event(&event);
    setInjectedEventFlag(false);
}
