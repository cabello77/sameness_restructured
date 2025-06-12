#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <cstring>

#include "EventPacket.h"

// Injection declarations
void injectKeyPress(const EventPacket&);
void injectMouseMove(const EventPacket&);

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

int main() {
    try {
        boost::asio::io_context io_context;
        ssl::context ctx(ssl::context::tlsv12_server);
        ctx.set_options(ssl::context::default_workarounds);
        ctx.use_certificate_chain_file("server.crt");
        ctx.use_private_key_file("server.key", ssl::context::pem);

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server listening on port 12345...\n";

        ssl::stream<tcp::socket> socket(io_context, ctx);
        acceptor.accept(socket.next_layer());
        socket.handshake(ssl::stream_base::server);

        std::vector<uint8_t> buffer(4096);
        boost::system::error_code ec;

        while (true) {
            size_t len = socket.read_some(boost::asio::buffer(buffer), ec);
            if (ec == boost::asio::error::eof) {
                std::cout << "Client disconnected.\n";
                break;
            }
            if (ec) throw boost::system::system_error(ec);

            // Deserialize packet
            std::vector<uint8_t> data(buffer.begin(), buffer.begin() + len);
            EventPacket pkt = EventPacket::fromBytes(data);

            // Route to injection
            switch (pkt.type) {
                case EventType::KeyPress:
                    injectKeyPress(pkt);
                    break;
                case EventType::MouseMove:
                    injectMouseMove(pkt);
                    break;
                default:
                    // We’re not injecting button events yet
                    break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
