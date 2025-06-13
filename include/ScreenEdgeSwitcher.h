#pragma once

// Control state: events are either handled locally on the Host,
// or forwarded to the peer Client.
enum class ControlState {
    HOST,
    CLIENT
};

class ScreenEdgeSwitcher {
public:
    // hostWidth: width in pixels of the host’s screen (e.g. 1920)
    // hostHeight: height in pixels of the host’s screen (not used here, but for future extension)
    ScreenEdgeSwitcher(int hostWidth, int hostHeight);

    // Call on every mouse-move event.
    // Returns the new control state.
    ControlState update(int x, int y);

    // Helper: are we currently forwarding to client?
    bool isClientControlled() const;

private:
    int hostWidth_;
    // int hostHeight_; // reserved for future multi-monitor logic
    ControlState state_ = ControlState::HOST;
};
