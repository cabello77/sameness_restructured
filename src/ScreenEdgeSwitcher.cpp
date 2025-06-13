#include "ScreenEdgeSwitcher.h"

ScreenEdgeSwitcher::ScreenEdgeSwitcher(int hostWidth, int /*hostHeight*/) : hostWidth_(hostWidth) {}

ControlState ScreenEdgeSwitcher::update(int x, int /*y*/) {
    if (x >= hostWidth_) {
        state_ = ControlState::CLIENT;
    } else {
        state_ = ControlState::HOST;
    }
    return state_;
}

bool ScreenEdgeSwitcher::isClientControlled() const {
    return state_ == ControlState::CLIENT;
}
