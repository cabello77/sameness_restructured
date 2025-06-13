#include "../include/EventPacket.h"
#include <cstring>  // for memcpy
#include "EventState.h"
#include <uiohook.h>
#include <stdexcept>
#include <memory>
#include <type_traits>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>

namespace {
    class MacOSEventInjector {
    public:
        static void injectKeyPress(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint32_t)) {
                throw std::runtime_error("Invalid key press payload size");
            }
            
            uint32_t code;
            std::memcpy(&code, pkt.payload.data(), sizeof(code));
            
            using CGEventPtr = std::unique_ptr<std::remove_pointer_t<CGEventRef>, decltype(&CFRelease)>;
            CGEventPtr eDown(
                CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, true),
                CFRelease
            );
            CGEventPtr eUp(
                CGEventCreateKeyboardEvent(NULL, (CGKeyCode)code, false),
                CFRelease
            );
            
            if (!eDown || !eUp) {
                throw std::runtime_error("Failed to create keyboard events");
            }
            
            CGEventPost(kCGHIDEventTap, eDown.get());
            CGEventPost(kCGHIDEventTap, eUp.get());
        }

        static void injectKeyRelease(const EventPacket&) {
            // Not implemented for Mac
        }

        static void injectMouseMove(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(int32_t) * 2) {
                throw std::runtime_error("Invalid mouse move payload size");
            }
            
            int32_t coords[2];
            std::memcpy(coords, pkt.payload.data(), sizeof(coords));
            
            using CGEventPtr = std::unique_ptr<std::remove_pointer_t<CGEventRef>, decltype(&CFRelease)>;
            CGEventPtr e(
                CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, 
                    CGPointMake(coords[0], coords[1]), kCGMouseButtonLeft),
                CFRelease
            );
            
            if (!e) {
                throw std::runtime_error("Failed to create mouse event");
            }
            
            CGEventPost(kCGHIDEventTap, e.get());
        }

        static void injectMouseButtonPress(const EventPacket&) {
            // Not implemented for Mac
        }

        static void injectMouseButtonRelease(const EventPacket&) {
            // Not implemented for Mac
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
#else
// Fallback to uiohook for other platforms
namespace {
    class UiohookEventInjector {
    public:
        static void injectKeyPress(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint16_t)) {
                return;
            }
            uint16_t keycode;
            std::memcpy(&keycode, pkt.payload.data(), sizeof(keycode));
            setInjectedEventFlag(true);
            uiohook_event event = {
                .type = EVENT_KEY_PRESSED,
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
        static void injectKeyRelease(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint16_t)) {
                return;
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
        static void injectMouseMove(const EventPacket& pkt) {
            if (pkt.payload.size() < 2 * sizeof(int32_t)) {
                return;
            }
            int32_t coords[2];
            std::memcpy(coords, pkt.payload.data(), sizeof(coords));
            setInjectedEventFlag(true);
            uiohook_event event = {
                .type = EVENT_MOUSE_MOVED,
                .time = pkt.timestamp,
                .mask = 0,
                .data = {
                    .mouse = {
                        .button = 0,
                        .clicks = 0,
                        .x = static_cast<int16_t>(coords[0]),
                        .y = static_cast<int16_t>(coords[1])
                    }
                }
            };
            hook_post_event(&event);
            setInjectedEventFlag(false);
        }
        static void injectMouseButtonPress(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
                return;
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
            hook_post_event(&event);
            setInjectedEventFlag(false);
        }
        static void injectMouseButtonRelease(const EventPacket& pkt) {
            if (pkt.payload.size() < sizeof(uint16_t) + 2 * sizeof(int16_t)) {
                return;
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
            hook_post_event(&event);
            setInjectedEventFlag(false);
        }
    };
    using PlatformInjector = UiohookEventInjector;
}
#endif

// Global injection functions that use the platform-specific injector
void injectKeyPress(const EventPacket& pkt) {
    PlatformInjector::injectKeyPress(pkt);
}

void injectKeyRelease(const EventPacket& pkt) {
    PlatformInjector::injectKeyRelease(pkt);
}

void injectMouseMove(const EventPacket& pkt) {
    PlatformInjector::injectMouseMove(pkt);
}

void injectMouseButtonPress(const EventPacket& pkt) {
    PlatformInjector::injectMouseButtonPress(pkt);
}

void injectMouseButtonRelease(const EventPacket& pkt) {
    PlatformInjector::injectMouseButtonRelease(pkt);
}
