# Sameness

A cross-platform input sharing application that allows you to control multiple computers using a single keyboard and mouse, similar to Synergy. This project enables seamless keyboard and mouse sharing between different machines on the same network.

![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS-lightgrey.svg)

## 📋 Features

- **Cross-platform Support**: Works on Windows and macOS
- **Real-time Input Sharing**: Low-latency keyboard and mouse input sharing
- **Secure Communication**: Encrypted network communication using OpenSSL
- **Modern GUI**: Optional Qt-based user interface for easy configuration
- **Low Latency**: Efficient input capture using libuiohook
- **Modular Design**: Separate client and server components for flexibility
- **Easy Configuration**: Simple setup process with minimal configuration

## 🚀 Quick Start

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- vcpkg for dependency management
- Boost (system component)
- OpenSSL
- Qt6 (optional, for GUI)
- libuiohook

### Installation

1. **Clone the repository**:
```bash
git clone https://github.com/yourusername/sameness.git
cd sameness
```

2. **Install dependencies** using vcpkg:
```bash
# Windows
vcpkg install boost-system:x64-windows
vcpkg install openssl:x64-windows

# macOS
vcpkg install boost-system
vcpkg install openssl
```

3. **Build libuiohook**:
```bash
cd libuiohook
mkdir build && cd build
cmake ..
cmake --build .
cd ../..
```

4. **Build the project**:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## 📁 Project Structure

```
sameness/
├── src/               # Source code files
├── include/          # Header files
├── libuiohook/       # Input capture library
├── third_party/      # Third-party dependencies
├── build/            # Build output directory
└── CMakeLists.txt    # CMake configuration
```

## 🛠️ Components

### Core Library
- Handles event serialization
- Manages basic functionality
- Provides common utilities

### Network Client
- Captures local input events
- Sends events to the server
- Handles connection management

### Network Server
- Receives input events
- Injects events into the local system
- Manages client connections

### GUI (Optional)
- Qt-based configuration interface
- Real-time status monitoring
- Easy setup and configuration

## 💻 Usage

### Server Setup
1. Run the server on the computer that will receive input:
```bash
./network_server
```

### Client Setup
1. Run the client on the computer that will share its input:
```bash
./network_client
```

### GUI Configuration (Optional)
1. Launch the configuration interface:
```bash
./synergy_ui
```

## 🔧 Configuration

The application can be configured through:
- Command-line arguments
- Configuration file
- GUI interface (if Qt is enabled)

### Example Configuration
```json
{
  "server": {
    "port": 24800,
    "encryption": true
  },
  "client": {
    "server_address": "192.168.1.100",
    "auto_reconnect": true
  }
}
```

## 🤝 Contributing

We welcome contributions! Here's how you can help:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow the existing code style
- Add tests for new features
- Update documentation as needed
- Keep commits atomic and well-described

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [libuiohook](https://github.com/kwhat/libuiohook) for input capture
- [Boost](https://www.boost.org/) for networking
- [OpenSSL](https://www.openssl.org/) for secure communication
- [Qt](https://www.qt.io/) for the GUI framework

## 📞 Support

- Create an issue for bug reports
- Submit a pull request for contributions
- Contact the maintainers for questions

## 🔄 Roadmap

- [ ] Add support for Linux
- [ ] Implement clipboard sharing
- [ ] Add file transfer capabilities
- [ ] Improve configuration interface
- [ ] Add more security features
