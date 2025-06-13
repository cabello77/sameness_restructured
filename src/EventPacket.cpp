#include "../include/EventPacket.h"
#include <cstring>  // for memcpy

std::vector<uint8_t> EventPacket::toBytes() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1 + 8 + 4 + payloadSize);
    // 1. type
    buffer.push_back(static_cast<uint8_t>(type));
    // 2. timestamp (big-endian)
    for (int i = 7; i >= 0; --i) {
        buffer.push_back(static_cast<uint8_t>((timestamp >> (i*8)) & 0xFF));
    }
    // 3. payloadSize
    for (int i = 3; i >= 0; --i) {
        buffer.push_back(static_cast<uint8_t>((payloadSize >> (i*8)) & 0xFF));
    }
    // 4. payload bytes
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    return buffer;
}

EventPacket EventPacket::fromBytes(const std::vector<uint8_t>& buffer) {
    EventPacket pkt;
    size_t offset = 0;
    // 1. type
    pkt.type = static_cast<SamenessEventType>(buffer[offset++]);
    // 2. timestamp
    pkt.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        pkt.timestamp = (pkt.timestamp << 8) | buffer[offset++];
    }
    // 3. payloadSize
    pkt.payloadSize = 0;
    for (int i = 0; i < 4; ++i) {
        pkt.payloadSize = (pkt.payloadSize << 8) | buffer[offset++];
    }
    // 4. payload
    pkt.payload.assign(buffer.begin() + offset,
                       buffer.begin() + offset + pkt.payloadSize);
    return pkt;
}
