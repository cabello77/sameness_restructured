#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

enum class EventType : uint8_t {
    KEYBOARD_DOWN,
    KEYBOARD_UP,
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE,
    MOUSE_WHEEL
};

struct Event {
    EventType type;
    std::chrono::system_clock::time_point timestamp;
    uint16_t keycode;  // For keyboard events
    uint16_t modifiers; // For keyboard modifier flags
    int16_t x;         // For mouse events
    int16_t y;         // For mouse events
    int16_t wheel_delta; // For mouse wheel events

    // Convert event to byte stream
    std::vector<uint8_t> toBytes() const {
        std::vector<uint8_t> bytes;
        bytes.reserve(32); // Pre-allocate space for efficiency

        // Type (1 byte)
        bytes.push_back(static_cast<uint8_t>(type));

        // Timestamp (8 bytes)
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        for (int i = 0; i < 8; i++) {
            bytes.push_back((ts >> (i * 8)) & 0xFF);
        }

        // Keycode (2 bytes)
        bytes.push_back(keycode & 0xFF);
        bytes.push_back((keycode >> 8) & 0xFF);

        // Modifiers (2 bytes)
        bytes.push_back(modifiers & 0xFF);
        bytes.push_back((modifiers >> 8) & 0xFF);

        // Mouse coordinates (4 bytes)
        bytes.push_back(x & 0xFF);
        bytes.push_back((x >> 8) & 0xFF);
        bytes.push_back(y & 0xFF);
        bytes.push_back((y >> 8) & 0xFF);

        // Wheel delta (2 bytes)
        bytes.push_back(wheel_delta & 0xFF);
        bytes.push_back((wheel_delta >> 8) & 0xFF);

        return bytes;
    }

    // Create event from byte stream
    static Event fromBytes(const std::vector<uint8_t>& bytes) {
        if (bytes.size() < 19) { // Updated minimum size check
            throw std::runtime_error("Invalid event data size");
        }

        Event event;
        size_t pos = 0;

        // Type
        event.type = static_cast<EventType>(bytes[pos++]);

        // Timestamp
        uint64_t ts = 0;
        for (int i = 0; i < 8; i++) {
            ts |= static_cast<uint64_t>(bytes[pos++]) << (i * 8);
        }
        event.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(ts));

        // Keycode
        event.keycode = bytes[pos] | (bytes[pos + 1] << 8);
        pos += 2;

        // Modifiers
        event.modifiers = bytes[pos] | (bytes[pos + 1] << 8);
        pos += 2;

        // Mouse coordinates
        event.x = bytes[pos] | (bytes[pos + 1] << 8);
        pos += 2;
        event.y = bytes[pos] | (bytes[pos + 1] << 8);
        pos += 2;

        // Wheel delta
        event.wheel_delta = bytes[pos] | (bytes[pos + 1] << 8);

        return event;
    }
}; 