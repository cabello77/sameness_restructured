#include "EventPacket.h"
#include "ScreenEdgeSwitcher.h"
#include "input_helper.h"    // uiohook event types
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <cstring>           // memcpy
#include <stdexcept>
#include <system_error>
#include <uiohook.h>
#include <chrono>
#include <openssl/x509.h>
#include "Injectors.h"

// Adjust these to your host's actual screen resolution:
static constexpr int HOST_SCREEN_WIDTH  = 1920;
static constexpr int HOST_SCREEN_HEIGHT = 1080;

// Global switcher instance:
static ScreenEdgeSwitcher edgeSwitcher(HOST_SCREEN_WIDTH, HOST_SCREEN_HEIGHT);

// Global pointer for SSL socket
static boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* g_ssl_socket_ptr = nullptr;

// Forward declaration for hook_callback
void hook_callback(uiohook_event * const event, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_socket);

// Static dispatch function for uiohook
static void dispatch_hook(uiohook_event* const event) {
    if (g_ssl_socket_ptr) {
        hook_callback(event, *g_ssl_socket_ptr);
    }
}

// Network error handling
class NetworkError : public std::runtime_error {
public:
    explicit NetworkError(const std::string& message) 
        : std::runtime_error(message) {}
};

// Safe network write with error handling
void safeWrite(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_socket, 
               const std::vector<uint8_t>& data) {
    try {
        boost::asio::write(ssl_socket, boost::asio::buffer(data));
    } catch (const boost::system::system_error& e) {
        throw NetworkError("Network write failed: " + std::string(e.what()));
    }
}

// Utility to get current time in microseconds
inline uint64_t currentMicroseconds() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

void hook_callback(uiohook_event * const event, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_socket) {
    if (!event) {
        throw std::runtime_error("Null event received in hook_callback");
    }

    std::cout << "Received event type: " << event->type << std::endl;

    EventPacket pkt;
    pkt.timestamp = currentMicroseconds();

    // Handle mouse movement with edge switching
    if (event->type == EVENT_MOUSE_MOVED) {
        int x = event->data.mouse.x;
        int y = event->data.mouse.y;
        
        std::cout << "Mouse moved to: (" << x << ", " << y << ")" << std::endl;
        
        // Validate coordinates
        if (x < 0 || y < 0) {
            throw std::runtime_error("Invalid mouse coordinates");
        }
        
        ControlState newState = edgeSwitcher.update(x, y);
        std::cout << "Control state: " << (newState == ControlState::HOST ? "HOST" : "CLIENT") << std::endl;

        if (newState == ControlState::HOST) {
            // Host-controlled: do NOT forward mouse moves
            return;
        }
        
        // CLIENT-controlled: forward this mouse move to peer
        pkt.type = SamenessEventType::MouseMove;
        pkt.payloadSize = sizeof(int32_t) * 2;
        pkt.payload.resize(pkt.payloadSize);
        
        // Convert to client-relative coordinates
        int32_t coords[2] = { x - HOST_SCREEN_WIDTH, y }; 
        std::memcpy(pkt.payload.data(), coords, pkt.payloadSize);

    } else {
        // Other events: only forward if in CLIENT state
        if (!edgeSwitcher.isClientControlled()) {
            return;
        }

        switch (event->type) {
            case EVENT_KEY_PRESSED:
                pkt.type = SamenessEventType::KeyPress;
                {
                    uint32_t code = event->data.keyboard.keycode;
                    if (code == 0) {
                        throw std::runtime_error("Invalid key code");
                    }
                    pkt.payloadSize = sizeof(code);
                    pkt.payload.resize(pkt.payloadSize);
                    std::memcpy(pkt.payload.data(), &code, pkt.payloadSize);
                }
                break;

            case EVENT_KEY_RELEASED:
                pkt.type = SamenessEventType::KeyRelease;
                {
                    uint32_t code = event->data.keyboard.keycode;
                    if (code == 0) {
                        throw std::runtime_error("Invalid key code");
                    }
                    pkt.payloadSize = sizeof(code);
                    pkt.payload.resize(pkt.payloadSize);
                    std::memcpy(pkt.payload.data(), &code, pkt.payloadSize);
                }
                break;

            case EVENT_MOUSE_PRESSED:
                pkt.type = SamenessEventType::MouseButtonPress;
                {
                    uint8_t button = static_cast<uint8_t>(event->data.mouse.button);
                    if (button == 0) {
                        throw std::runtime_error("Invalid mouse button");
                    }
                    int32_t x = event->data.mouse.x - HOST_SCREEN_WIDTH;
                    int32_t y = event->data.mouse.y;
                    
                    // Validate coordinates
                    if (x < -HOST_SCREEN_WIDTH || y < 0) {
                        throw std::runtime_error("Invalid mouse coordinates");
                    }
                    
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
                pkt.type = SamenessEventType::MouseButtonRelease;
                {
                    uint8_t button = static_cast<uint8_t>(event->data.mouse.button);
                    if (button == 0) {
                        throw std::runtime_error("Invalid mouse button");
                    }
                    int32_t x = event->data.mouse.x - HOST_SCREEN_WIDTH;
                    int32_t y = event->data.mouse.y;
                    
                    // Validate coordinates
                    if (x < -HOST_SCREEN_WIDTH || y < 0) {
                        throw std::runtime_error("Invalid mouse coordinates");
                    }
                    
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

    try {
        // Serialize and send
        auto bytes = pkt.toBytes();
        safeWrite(ssl_socket, bytes);
    } catch (const std::exception& e) {
        std::cerr << "Error sending event: " << e.what() << std::endl;
        // You might want to handle this error differently, e.g., by reconnecting
        throw;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <server_address>" << std::endl;
            return 1;
        }

        // Set up Boost ASIO context and SSL context
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_client);
        
        // Configure SSL context
        ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
        // Only use default verification for now
        // ssl_context.set_verify_callback(...); // Not needed
        
        // Load CA certificate for server verification
        ssl_context.load_verify_file("ca.crt");
        
        // Set up SSL socket
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket(io_context, ssl_context);
        
        // Resolve the server address
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argv[1], "12345");
        
        // Connect to the server
        boost::asio::connect(ssl_socket.lowest_layer(), endpoints);
        ssl_socket.lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
        
        // Perform SSL handshake
        try {
            ssl_socket.handshake(boost::asio::ssl::stream_base::client);
            std::cout << "SSL handshake successful" << std::endl;
        } catch (const boost::system::system_error& e) {
            std::cerr << "SSL handshake failed: " << e.what() << std::endl;
            return 1;
        }

        // Set global pointer for dispatch
        g_ssl_socket_ptr = &ssl_socket;
        hook_set_dispatch_proc(dispatch_hook);

        // Start the hook event loop
        if (hook_run() != UIOHOOK_SUCCESS) {
            std::cerr << "Failed to start uiohook event loop" << std::endl;
            return 1;
        }

        // Run the IO context
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
