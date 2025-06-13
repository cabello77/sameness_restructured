#pragma once 
#include <cstdint>
#include <vector>

enum class SamenessEventType : uint8_t {
    KeyPress            =1,
    KeyRelease          =2,
    MouseMove           =3,
    MouseButtonPress    =4,
    MouseButtonRelease  =5,
}; 

struct EventPacket {
    SamenessEventType type;
    uint64_t timestamp;
    uint32_t payloadSize;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> toBytes() const;
    static EventPacket fromBytes(const std::vector<uint8_t>& buffer);
};


