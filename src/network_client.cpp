#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>      // memcpy
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <uiohook.h>

#include "EventPacket.h"

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

// Global socket pointer for hook callback
ssl::stream<tcp::socket>* global_socket = nullptr;

// Last‐sent coordinates (init to impossible values)
static int32_t lastSentX = INT32_MIN, lastSentY = INT32_MIN;

// Helper: current time in microseconds
uint64_t currentMicroseconds() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

void hook_callback(uiohook_event * const event) {
    if (!global_socket) return;

    // --- Mouse moved ---
    if (event->type == EVENT_MOUSE_MOVED) {
        int x = event->data.mouse.x;
        int y = event->data.mouse.y;

        // Only forward if truly moved
        if (x == lastSentX && y == lastSentY) {
            return;
        }
        lastSentX = x;
        lastSentY = y;

        // Build packet
        EventPacket pkt;
        pkt.type        = EventType::MouseMove;
        pkt.timestamp   = currentMicroseconds();
        int32_t coords[2] = { x, y };
        pkt.payloadSize = sizeof(coords);
        pkt.payload.resize(pkt.payloadSize);
        std::memcpy(pkt.payload.data(), coords, pkt.payloadSize);

        // Send
        auto bytes = pkt.toBytes();
        boost::asio::write(*global_socket, boost::asio::buffer(bytes));
        std::cout << "[SEND] MouseMove (" << x << "," << y << ")\n";
    }
    // --- Key pressed ---
    else if (event->type == EVENT_KEY_PRESSED) {
        uint32_t code = event->data.keyboard.keycode;

        EventPacket pkt;
        pkt.type        = EventType::KeyPress;
        pkt.timestamp   = currentMicroseconds();
        pkt.payloadSize = sizeof(code);
        pkt.payload.resize(pkt.payloadSize);
        std::memcpy(pkt.payload.data(), &code, sizeof(code));

        auto bytes = pkt.toBytes();
        boost::asio::write(*global_socket, boost::asio::buffer(bytes));
        std::cout << "[SEND] KeyPress code=" << code << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>\n";
        return 1;
    }
    std::string host = argv[1];

    try {
        boost::asio::io_context io_context;
        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_verify_mode(ssl::verify_none);

        ssl::stream<tcp::socket> ssl_socket(io_context, ctx);
        global_socket = &ssl_socket;

        // Connect
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, "12345");
        boost::asio::connect(ssl_socket.next_layer(), endpoints);
        ssl_socket.handshake(ssl::stream_base::client);

        std::cout << "Connected to server. Forwarding all input...\n";

        // Start capturing
        hook_set_logger_proc(nullptr);
        hook_set_dispatch_proc(hook_callback);
        if (hook_run() != UIOHOOK_SUCCESS) {
            std::cerr << "Failed to start libuiohook.\n";
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
