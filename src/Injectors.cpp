#include "EventPacket.h"
#include <cstring>  // for memcpy
#include "EventState.h"
#include <uiohook.h>
#include <stdexcept>
#include <memory>

// Platform-specific implementations
#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>

namespace {
    class MacOSEventInjector {
    public:
        static void injectKeyPress(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint32_t)) {
                throw std::runtime_error("Invalid key press payload size");
            }
            
            uint32_t code;
            std::memcpy(&code, pkt.payload.data(), sizeof(code));
            
            std::unique_ptr<CGEventRef, decltype(&CFRelease)> eDown(
                CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, true),
                CFRelease
            );
            std::unique_ptr<CGEventRef, decltype(&CFRelease)> eUp(
                CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, false),
                CFRelease
            );
            
            if (!eDown || !eUp) {
                throw std::runtime_error("Failed to create keyboard events");
            }
            
            CGEventPost(kCGHIDEventTap, eDown.get());
            CGEventPost(kCGHIDEventTap, eUp.get());
        }

        static void injectMouseMove(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(int32_t) * 2) {
                throw std::runtime_error("Invalid mouse move payload size");
            }
            
            int32_t coords[2];
            std::memcpy(coords, pkt.payload.data(), sizeof(coords));
            
            std::unique_ptr<CGEventRef, decltype(&CFRelease)> e(
                CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, 
                    CGPointMake(coords[0], coords[1]), kCGMouseButtonLeft),
                CFRelease
            );
            
            if (!e) {
                throw std::runtime_error("Failed to create mouse event");
            }
            
            CGEventPost(kCGHIDEventTap, e.get());
        }
    };
    
    using PlatformInjector = MacOSEventInjector;
}

#elif defined(_WIN32)
#include <Windows.h>

namespace {
    class WindowsEventInjector {
    public:
        static void injectKeyPress(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint32_t)) {
                throw std::runtime_error("Invalid key press payload size");
            }
            
            uint32_t code;
            std::memcpy(&code, pkt.payload.data(), sizeof(code));
            
            INPUT inputs[2] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = static_cast<WORD>(code);
            inputs[0].ki.dwFlags = 0;  // key down
            
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = static_cast<WORD>(code);
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;  // key up
            
            if (SendInput(2, inputs, sizeof(INPUT)) != 2) {
                throw std::runtime_error("Failed to send keyboard input");
            }
        }

        static void injectMouseMove(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(int32_t) * 2) {
                throw std::runtime_error("Invalid mouse move payload size");
            }
            
            int32_t coords[2];
            std::memcpy(coords, pkt.payload.data(), sizeof(coords));
            
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            
            // Convert to absolute [0,65535] range
            input.mi.dx = static_cast<LONG>(coords[0] * (65535.0 / GetSystemMetrics(SM_CXSCREEN)));
            input.mi.dy = static_cast<LONG>(coords[1] * (65535.0 / GetSystemMetrics(SM_CYSCREEN)));
            
            if (SendInput(1, &input, sizeof(input)) != 1) {
                throw std::runtime_error("Failed to send mouse input");
            }
        }
    };
    
    using PlatformInjector = WindowsEventInjector;
}
#endif

// Common implementation using uiohook
void injectKeyRelease(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t)) {
        throw std::runtime_error("Invalid key release payload size");
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
    
    if (hook_post_event(&event) != UIOHOOK_SUCCESS) {
        setInjectedEventFlag(false);
        throw std::runtime_error("Failed to post key release event");
    }
    setInjectedEventFlag(false);
}

void injectMouseButtonPress(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
        throw std::runtime_error("Invalid mouse button press payload size");
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
                .button = button,
                .clicks = 1,
                .x = x,
                .y = y
            }
        }
    };
    
    if (hook_post_event(&event) != UIOHOOK_SUCCESS) {
        setInjectedEventFlag(false);
        throw std::runtime_error("Failed to post mouse button press event");
    }
    setInjectedEventFlag(false);
}

void injectMouseButtonRelease(const EventPacket& pkt) {
    if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
        throw std::runtime_error("Invalid mouse button release payload size");
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
                .button = button,
                .clicks = 1,
                .x = x,
                .y = y
            }
        }
    };
    
    if (hook_post_event(&event) != UIOHOOK_SUCCESS) {
        setInjectedEventFlag(false);
        throw std::runtime_error("Failed to post mouse button release event");
    }
    setInjectedEventFlag(false);
}

// Platform-specific event injection wrappers
void injectKeyPress(const EventPacket& pkt) {
    PlatformInjector::injectKeyPress(pkt);
}

void injectMouseMove(const EventPacket& pkt) {
    PlatformInjector::injectMouseMove(pkt);
}
