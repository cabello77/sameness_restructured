#include <iostream>
#include <uiohook.h>

void hook_callback(uiohook_event * const event) { 
    if (event->type == EVENT_KEY_PRESSED) {
        std::cout << "Key pressed: " << event->data.keyboard.keycode << std::endl;
        hook_stop();
    }
}

int main() {
    hook_set_logger_proc(nullptr);
    hook_set_dispatch_proc(hook_callback);
    if (hook_run() != UIOHOOK_SUCCESS) {
        std::cerr << "Failed to start lubuiohook." << std::endl;
        return 1;
    }
    return 0;
}