#include "ScreenEdgeSwitcher.h"
#include <iostream>

ScreenEdgeSwitcher::ScreenEdgeSwitcher(int hostWidth, int hostHeight) 
    : hostWidth_(hostWidth)
    , hostHeight_(hostHeight)
    , edgeThreshold_(20)  // 20 pixel threshold for edge detection
    , state_(ControlState::HOST) {
    std::cout << "ScreenEdgeSwitcher initialized with dimensions: " << hostWidth << "x" << hostHeight << std::endl;
}

ControlState ScreenEdgeSwitcher::update(int x, int y) {
    // Add hysteresis to prevent rapid switching
    static const int HYSTERESIS = 5;
    static int lastX = 0;
    
    // Log the current position
    std::cout << "Mouse position: (" << x << ", " << y << ")" << std::endl;
    
    // Check if we're near the edge with hysteresis
    bool nearEdge = false;
    
    // Right edge of host screen
    if (x >= hostWidth_ - edgeThreshold_ && x <= hostWidth_ + HYSTERESIS) {
        nearEdge = true;
        std::cout << "Near right edge of host screen" << std::endl;
    }
    // Left edge of client screen
    else if (x <= edgeThreshold_ && x >= -HYSTERESIS) {
        nearEdge = true;
        std::cout << "Near left edge of client screen" << std::endl;
    }
    
    // Update state based on edge detection
    if (nearEdge) {
        if (x > hostWidth_ / 2) {
            state_ = ControlState::CLIENT;
            std::cout << "Switching to client control" << std::endl;
        } else {
            state_ = ControlState::HOST;
            std::cout << "Switching to host control" << std::endl;
        }
    }
    
    lastX = x;
    return state_;
}

bool ScreenEdgeSwitcher::isClientControlled() const {
    return state_ == ControlState::CLIENT;
}

void ScreenEdgeSwitcher::setEdgeThreshold(int threshold) {
    edgeThreshold_ = threshold;
    std::cout << "Edge threshold set to: " << threshold << " pixels" << std::endl;
}
