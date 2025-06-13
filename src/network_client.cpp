#include "EventPacket.h"
#include "ScreenEdgeSwitcher.h"
#include "input_helper.h"    // uiohook event types
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <cstring>           // memcpy

// Adjust these to your host’s actual screen resolution:
static constexpr int HOST_SCREEN_WIDTH  = 1920;
static constexpr int HOST_SCREEN_HEIGHT = 1080;

// Global switcher instance:
static ScreenEdgeSwitcher edgeSwitcher(HOST_SCREEN_WIDTH, HOST_SCREEN_HEIGHT);

void hook_callback(uiohook_event * const event) {
    EventPacket pkt;
    pkt.timestamp = currentMicroseconds();

    // Handle mouse movement with edge switching
    if (event->type == EVENT_MOUSE_MOVED) {
        int x = event->data.mouse.x;
        int y = event->data.mouse.y;
        ControlState newState = edgeSwitcher.update(x, y);

        if (newState == ControlState::HOST) {
            // Host-controlled: do NOT forward mouse moves
            return;
        }
        // CLIENT-controlled: forward this mouse move to peer
        pkt.type = EventType::MouseMove;
        // 2 * 4 bytes (int32_t x, int32_t y)
        pkt.payloadSize = sizeof(int32_t) * 2;
        pkt.payload.resize(pkt.payloadSize);
        int32_t coords[2] = { x - HOST_SCREEN_WIDTH, y }; 
        // subtract HOST_SCREEN_WIDTH to get client-relative X
        std::memcpy(pkt.payload.data(), coords, pkt.payloadSize);

    } else {
        // Other events: only forward if in CLIENT state
        if (!edgeSwitcher.isClientControlled()) {
            return;
        }

        switch (event->type) {
            case EVENT_KEY_PRESSED:
                pkt.type = EventType::KeyPress;
                {
                    uint32_t code = event->data.keyboard.keycode;
                    pkt.payloadSize = sizeof(code);
                    pkt.payload.resize(pkt.payloadSize);
                    std::memcpy(pkt.payload.data(), &code, pkt.payloadSize);
                }
                break;

            case EVENT_KEY_RELEASED:
                pkt.type = EventType::KeyRelease;
                {
                    uint32_t code = event->data.keyboard.keycode;
                    pkt.payloadSize = sizeof(code);
                    pkt.payload.resize(pkt.payloadSize);
                    std::memcpy(pkt.payload.data(), &code, pkt.payloadSize);
                }
                break;

            case EVENT_MOUSE_PRESSED:
                pkt.type = EventType::MouseButtonPress;
                {
                    uint8_t button = static_cast<uint8_t>(event->data.mouse.button);
                    int32_t x = event->data.mouse.x - HOST_SCREEN_WIDTH;
                    int32_t y = event->data.mouse.y;
                    pkt.payloadSize = sizeof(button) + sizeof(int32_t)*2;
                    pkt.payload.resize(pkt.payloadSize);
                    size_t off = 0;
                    std::memcpy(pkt.payload.data() + off, &button, sizeof(button));
                    off += sizeof(button);
                    std::memcpy(pkt.payload.data() + off, &x, sizeof(x));
                    off += sizeof(x);
                    std::memcpy(pkt.payload.data() + off, &y, sizeof(y));
                }
                break;

            case EVENT_MOUSE_RELEASED:
                pkt.type = EventType::MouseButtonRelease;
                {
                    uint8_t button = static_cast<uint8_t>(event->data.mouse.button);
                    int32_t x = event->data.mouse.x - HOST_SCREEN_WIDTH;
                    int32_t y = event->data.mouse.y;
                    pkt.payloadSize = sizeof(button) + sizeof(int32_t)*2;
                    pkt.payload.resize(pkt.payloadSize);
                    size_t off = 0;
                    std::memcpy(pkt.payload.data() + off, &button, sizeof(button));
                    off += sizeof(button);
                    std::memcpy(pkt.payload.data() + off, &x, sizeof(x));
                    off += sizeof(x);
                    std::memcpy(pkt.payload.data() + off, &y, sizeof(y));
                }
                break;

            default:
                return; // ignore other events
        }
    }

    // Serialize and send
    auto bytes = pkt.toBytes();
    boost::asio::write(ssl_socket, boost::asio::buffer(bytes));
}

// … rest of your main() and setup code …
