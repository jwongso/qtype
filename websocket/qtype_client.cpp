// qtype_client.cpp - Cross-Platform Console Client with WebSocket
// Compile (MacOS): clang++ qtype_client.cpp -o qtype_client -std=c++17 -framework ApplicationServices -framework CoreFoundation
// Compile (Linux): g++ qtype_client.cpp -o qtype_client -std=c++17 -lX11 -lXtst -lXss
// Compile (Windows): cl qtype_client.cpp /EHsc /std:c++17 /Fe:qtype_client.exe
//            or: g++ qtype_client.cpp -o qtype_client.exe -std=c++17 -static-libgcc -static-libstdc++
// Compile (WSL): See Linux or use xdotool

// Platform-specific includes - Windows first to avoid conflicts
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/keysym.h>
#endif

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>
#include <atomic>

// ============================================================================
// Constants
// ============================================================================

namespace TypingConstants {
    // Math constants
    constexpr double TWO_PI = 6.28318530718;
    
    // Timing bounds
    constexpr int MIN_DELAY_MS = 15;
    constexpr int MAX_DELAY_MS = 8000;
    constexpr int MIN_HOLD_TIME_MS = 40;
    constexpr int MAX_HOLD_TIME_MS = 180;
    
    // Fatigue calculation
    constexpr int CHARS_BEFORE_FATIGUE_UPDATE = 50;
    constexpr int CHARS_FOR_MAX_FATIGUE = 1000;
    constexpr double MAX_FATIGUE_FACTOR = 0.25;
    constexpr double NOISE_LEVEL = 0.15;
    
    // Mouse movement
    constexpr int MIN_MOUSE_MOVE_INTERVAL_CHARS = 20;
    constexpr int MAX_MOUSE_MOVE_INTERVAL_CHARS = 60;
    constexpr int MIN_MOUSE_PIXELS = 3;
    constexpr int MAX_MOUSE_PIXELS = 15;
    constexpr int MIN_MOUSE_PAUSE_MS = 100;
    constexpr int MAX_MOUSE_PAUSE_MS = 300;
    
    // Scroll constants
    constexpr int MIN_SCROLL_AMOUNT = 1;
    constexpr int MAX_SCROLL_AMOUNT = 3;
    constexpr double SCROLL_DOWN_PROBABILITY = 0.8;
}

// Simple WebSocket client using libwebsockets or raw socket
// For simplicity, I'll create a polling HTTP-based version first
// Full WebSocket implementation below

// ============================================================================
// Simplified JSON (if you don't want external dependency)
// ============================================================================

#include <sstream>
#include <map>

class SimpleJSON {
public:
    std::map<std::string, std::string> data;
    
    static SimpleJSON parse(const std::string& json) {
        SimpleJSON obj;
        // Very basic parser - you'd want a real JSON library
        // This is just for demonstration
        return obj;
    }
    
    std::string get(const std::string& key) {
        return data[key];
    }
    
    bool getBool(const std::string& key) {
        return data[key] == "true";
    }
    
    int getInt(const std::string& key) {
        return std::stoi(data[key]);
    }
};

// ============================================================================
// Random Number Generator (same as console version)
// ============================================================================

class RandomGenerator {
public:
    static double gamma(double shape, double scale) {
        if (shape < 1.0) {
            return gamma(1.0 + shape, scale) * std::pow(uniform(), 1.0 / shape);
        }
        
        double d = shape - 1.0 / 3.0;
        double c = 1.0 / std::sqrt(9.0 * d);
        
        while (true) {
            double x, v;
            do {
                x = normal(0.0, 1.0);
                v = 1.0 + c * x;
            } while (v <= 0.0);
            
            v = v * v * v;
            double u = uniform();
            
            if (u < 1.0 - 0.0331 * x * x * x * x) {
                return d * v * scale;
            }
            if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v))) {
                return d * v * scale;
            }
        }
    }
    
    static double normal(double mean, double stddev) {
        thread_local bool hasSpare = false;
        thread_local double spare;
        
        if (hasSpare) {
            hasSpare = false;
            return mean + stddev * spare;
        }
        
        double u, v, s;
        do {
            u = uniform() * 2.0 - 1.0;
            v = uniform() * 2.0 - 1.0;
            s = u * u + v * v;
        } while (s >= 1.0 || s == 0.0);
        
        s = std::sqrt(-2.0 * std::log(s) / s);
        spare = v * s;
        hasSpare = true;
        
        return mean + stddev * u * s;
    }
    
    static int range(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
    
    static double uniform() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }
};

// ============================================================================
// Cross-Platform Keyboard Simulator
// ============================================================================

class KeyboardSimulator {
public:
#ifdef __APPLE__
    void typeCharacter(unsigned char c, int holdTimeMs) {
        UniChar uc = c;
        CGEventRef down = nullptr;
        CGEventRef up = nullptr;
        
        if (c == '\n') {
            // Send Shift+Enter
            CGEventRef shiftDown = CGEventCreateKeyboardEvent(nullptr, 56, true);
            CGEventPost(kCGHIDEventTap, shiftDown);
            CFRelease(shiftDown);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            down = CGEventCreateKeyboardEvent(nullptr, 0x24, true);
            up = CGEventCreateKeyboardEvent(nullptr, 0x24, false);
            
            CGEventSetFlags(down, kCGEventFlagMaskShift);
            CGEventSetFlags(up, kCGEventFlagMaskShift);
            
            CGEventPost(kCGHIDEventTap, down);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdTimeMs));
            CGEventPost(kCGHIDEventTap, up);
            
            CFRelease(down);
            CFRelease(up);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            CGEventRef shiftUp = CGEventCreateKeyboardEvent(nullptr, 56, false);
            CGEventPost(kCGHIDEventTap, shiftUp);
            CFRelease(shiftUp);
            return;
        } else {
            down = CGEventCreateKeyboardEvent(nullptr, 0, true);
            up = CGEventCreateKeyboardEvent(nullptr, 0, false);
            CGEventKeyboardSetUnicodeString(down, 1, &uc);
            CGEventKeyboardSetUnicodeString(up, 1, &uc);
            
            CGEventPost(kCGHIDEventTap, down);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdTimeMs));
            CGEventPost(kCGHIDEventTap, up);
            
            CFRelease(down);
            CFRelease(up);
        }
    }
    
    void pressBackspace() {
        CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 51, true);  // kVK_Delete
        CGEventRef up = CGEventCreateKeyboardEvent(nullptr, 51, false);
        CGEventPost(kCGHIDEventTap, down);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(down);
        CFRelease(up);
    }
    
    void releaseAllKeys() {
        // Not needed on macOS typically
    }
    
#elif defined(__linux__)
    KeyboardSimulator() {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Error: Cannot open X display. Make sure DISPLAY is set.\n";
            std::cerr << "For WSL, you may need to install and run an X server (VcXsrv, Xming, etc.)\n";
            std::cerr << "Or use: export DISPLAY=:0\n";
        }
    }
    
    ~KeyboardSimulator() {
        if (display) {
            XCloseDisplay(display);
        }
    }
    
    void typeCharacter(unsigned char c, int holdTimeMs) {
        if (!display) return;
        
        if (c == '\n') {
            // Send Shift+Enter
            KeyCode shift = XKeysymToKeycode(display, XK_Shift_L);
            KeyCode enter = XKeysymToKeycode(display, XK_Return);
            
            XTestFakeKeyEvent(display, shift, True, 0);
            XFlush(display);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            XTestFakeKeyEvent(display, enter, True, 0);
            XFlush(display);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdTimeMs));
            XTestFakeKeyEvent(display, enter, False, 0);
            XFlush(display);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            XTestFakeKeyEvent(display, shift, False, 0);
            XFlush(display);
            return;
        }
        
        // Convert character to KeySym
        KeySym keysym = charToKeySym(c);
        if (keysym == NoSymbol) {
            std::cerr << "Warning: Cannot map character '" << c << "' (code: " << (int)c << ")\n";
            return;
        }
        
        KeyCode keycode = XKeysymToKeycode(display, keysym);
        if (keycode == 0) {
            std::cerr << "Warning: No keycode for character '" << c << "'\n";
            return;
        }
        
        // Check if shift is needed
        bool needShift = std::isupper(c) || isShiftChar(c);
        
        if (needShift) {
            KeyCode shift = XKeysymToKeycode(display, XK_Shift_L);
            XTestFakeKeyEvent(display, shift, True, 0);
            XFlush(display);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        XTestFakeKeyEvent(display, keycode, True, 0);
        XFlush(display);
        std::this_thread::sleep_for(std::chrono::milliseconds(holdTimeMs));
        XTestFakeKeyEvent(display, keycode, False, 0);
        XFlush(display);
        
        if (needShift) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            KeyCode shift = XKeysymToKeycode(display, XK_Shift_L);
            XTestFakeKeyEvent(display, shift, False, 0);
            XFlush(display);
        }
    }
    
    void pressBackspace() {
        if (!display) return;
        
        KeyCode backspace = XKeysymToKeycode(display, XK_BackSpace);
        XTestFakeKeyEvent(display, backspace, True, 0);
        XFlush(display);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        XTestFakeKeyEvent(display, backspace, False, 0);
        XFlush(display);
    }
    
    void releaseAllKeys() {
        // Not typically needed for Linux
    }
    
private:
    Display* display = nullptr;
    
    KeySym charToKeySym(unsigned char c) {
        // Handle special characters
        switch (c) {
            case ' ': return XK_space;
            case '!': return XK_exclam;
            case '"': return XK_quotedbl;
            case '#': return XK_numbersign;
            case '$': return XK_dollar;
            case '%': return XK_percent;
            case '&': return XK_ampersand;
            case '\'': return XK_apostrophe;
            case '(': return XK_parenleft;
            case ')': return XK_parenright;
            case '*': return XK_asterisk;
            case '+': return XK_plus;
            case ',': return XK_comma;
            case '-': return XK_minus;
            case '.': return XK_period;
            case '/': return XK_slash;
            case ':': return XK_colon;
            case ';': return XK_semicolon;
            case '<': return XK_less;
            case '=': return XK_equal;
            case '>': return XK_greater;
            case '?': return XK_question;
            case '@': return XK_at;
            case '[': return XK_bracketleft;
            case '\\': return XK_backslash;
            case ']': return XK_bracketright;
            case '^': return XK_asciicircum;
            case '_': return XK_underscore;
            case '`': return XK_grave;
            case '{': return XK_braceleft;
            case '|': return XK_bar;
            case '}': return XK_braceright;
            case '~': return XK_asciitilde;
            case '\t': return XK_Tab;
            case '\r': return XK_Return;
        }
        
        // Handle alphanumeric
        if (c >= 'a' && c <= 'z') return XK_a + (c - 'a');
        if (c >= 'A' && c <= 'Z') return XK_a + (c - 'A');
        if (c >= '0' && c <= '9') return XK_0 + (c - '0');
        
        return NoSymbol;
    }
    
    bool isShiftChar(unsigned char c) {
        return (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
                c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
                c == ':' || c == '"' || c == '<' || c == '>' || c == '?' ||
                c == '~');
    }
#elif defined(_WIN32) || defined(_WIN64)
    // Windows implementation using SendInput
    void typeCharacter(unsigned char c, int holdTimeMs) {
        if (c == '\n') {
            // Send Shift+Enter on Windows
            INPUT shiftDown = {0};
            shiftDown.type = INPUT_KEYBOARD;
            shiftDown.ki.wVk = VK_SHIFT;
            SendInput(1, &shiftDown, sizeof(INPUT));
            
            Sleep(10);
            
            INPUT enterDown = {0};
            enterDown.type = INPUT_KEYBOARD;
            enterDown.ki.wVk = VK_RETURN;
            SendInput(1, &enterDown, sizeof(INPUT));
            
            Sleep(holdTimeMs);
            
            INPUT enterUp = {0};
            enterUp.type = INPUT_KEYBOARD;
            enterUp.ki.wVk = VK_RETURN;
            enterUp.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &enterUp, sizeof(INPUT));
            
            Sleep(10);
            
            INPUT shiftUp = {0};
            shiftUp.type = INPUT_KEYBOARD;
            shiftUp.ki.wVk = VK_SHIFT;
            shiftUp.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &shiftUp, sizeof(INPUT));
            return;
        }
        
        // Use Unicode input for all characters
        wchar_t wc = static_cast<unsigned char>(c);
        
        // Key down
        INPUT down = {0};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = wc;
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        SendInput(1, &down, sizeof(INPUT));
        
        Sleep(holdTimeMs);
        
        // Key up
        INPUT up = {0};
        up.type = INPUT_KEYBOARD;
        up.ki.wScan = wc;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(1, &up, sizeof(INPUT));
    }
    
    void pressBackspace() {
        INPUT down = {0};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_BACK;
        SendInput(1, &down, sizeof(INPUT));
        
        Sleep(10);
        
        INPUT up = {0};
        up.type = INPUT_KEYBOARD;
        up.ki.wVk = VK_BACK;
        up.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &up, sizeof(INPUT));
    }
    
    void releaseAllKeys() {
        // Release common modifier keys on Windows
        WORD modifiers[] = {VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN};
        for (WORD vk : modifiers) {
            INPUT up = {0};
            up.type = INPUT_KEYBOARD;
            up.ki.wVk = vk;
            up.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &up, sizeof(INPUT));
        }
    }
#else
    void typeCharacter(unsigned char c, int holdTimeMs) {
        std::cerr << "Error: Keyboard simulation not implemented for this platform\n";
    }
    
    void pressBackspace() {
        std::cerr << "Error: Backspace not implemented for this platform\n";
    }
    
    void releaseAllKeys() {
    }
#endif
};

// ============================================================================
// Cross-Platform Mouse Simulator
// ============================================================================

class MouseSimulator {
public:
#ifdef __APPLE__
    void moveRelative(int deltaX, int deltaY) {
        CGEventRef event = CGEventCreate(nullptr);
        CGPoint currentPos = CGEventGetLocation(event);
        CFRelease(event);
        
        CGPoint newPos;
        newPos.x = currentPos.x + deltaX;
        newPos.y = currentPos.y + deltaY;
        
        CGEventRef move = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, newPos, kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, move);
        CFRelease(move);
    }
    
    void scroll(int amount) {
        // Create a scroll wheel event
        // Positive amount = scroll down, negative = scroll up
        CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(nullptr, 
                                                               kCGScrollEventUnitLine,
                                                               1,  // Number of wheels (1 for vertical)
                                                               amount);
        CGEventPost(kCGHIDEventTap, scrollEvent);
        CFRelease(scrollEvent);
    }
#elif defined(__linux__)
    MouseSimulator() {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Warning: Cannot open X display for mouse\n";
        }
    }
    
    ~MouseSimulator() {
        if (display) {
            XCloseDisplay(display);
        }
    }
    
    void moveRelative(int deltaX, int deltaY) {
        if (!display) return;
        XTestFakeRelativeMotionEvent(display, deltaX, deltaY, CurrentTime);
        XFlush(display);
    }
    
    void scroll(int amount) {
        if (!display) return;
        // X11 scroll simulation using button 4 (scroll up) and button 5 (scroll down)
        unsigned int button = (amount > 0) ? 5 : 4;  // 5=down, 4=up
        int absAmount = std::abs(amount);
        
        for (int i = 0; i < absAmount; i++) {
            XTestFakeButtonEvent(display, button, True, CurrentTime);
            XTestFakeButtonEvent(display, button, False, CurrentTime);
        }
        XFlush(display);
    }
    
private:
    Display* display = nullptr;
#elif defined(_WIN32) || defined(_WIN64)
    void moveRelative(int deltaX, int deltaY) {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(pt.x + deltaX, pt.y + deltaY);
    }
    
    void scroll(int amount) {
        // Windows scroll using mouse_event with MOUSEEVENTF_WHEEL
        // Positive amount = scroll down (negative wheel delta)
        // Negative amount = scroll up (positive wheel delta)
        // Each WHEEL_DELTA unit is 120
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = -amount * 120;  // Negative for down scroll
        SendInput(1, &input, sizeof(INPUT));
    }
#else
    void moveRelative(int deltaX, int deltaY) {
        // No-op for unsupported platforms
    }
    
    void scroll(int amount) {
        // No-op for unsupported platforms
    }
#endif
};

// ============================================================================
// Idle Detection (Platform-Specific)
// ============================================================================

class IdleDetector {
public:
    // Returns milliseconds since last user input (keyboard or mouse)
    static int64_t getIdleTimeMs() {
#ifdef __APPLE__
        // macOS: Use CGEventSource to get seconds since last event
        CFTimeInterval idleSeconds = CGEventSourceSecondsSinceLastEventType(
            kCGEventSourceStateHIDSystemState,
            kCGAnyInputEventType);
        return static_cast<int64_t>(idleSeconds * 1000);
        
#elif defined(_WIN32) || defined(_WIN64)
        // Windows: Use GetLastInputInfo
        LASTINPUTINFO lii;
        lii.cbSize = sizeof(LASTINPUTINFO);
        if (GetLastInputInfo(&lii)) {
            DWORD currentTime = GetTickCount();
            return static_cast<int64_t>(currentTime - lii.dwTime);
        }
        return 0;
        
#elif defined(__linux__)
        // Linux: Use XScreenSaver extension for accurate idle time
        static Display* display = nullptr;
        if (!display) {
            display = XOpenDisplay(nullptr);
            if (!display) {
                return 0;  // Can't open display
            }
        }
        
        XScreenSaverInfo* info = XScreenSaverAllocInfo();
        if (!info) {
            return 0;
        }
        
        if (XScreenSaverQueryInfo(display, DefaultRootWindow(display), info)) {
            unsigned long idleMs = info->idle;
            XFree(info);
            return static_cast<int64_t>(idleMs);
        }
        
        XFree(info);
        return 0;
#else
        return 0;
#endif
    }
};

// ============================================================================
// Typing Engine
// ============================================================================

class TypingEngine {
public:
    TypingEngine() 
        : rhythmPhase_(RandomGenerator::uniform() * TypingConstants::TWO_PI)
        , fatigueFactor_(1.0)
        , burstRemaining_(0)
        , totalCharsTyped_(0)
        , minDelayMs_(120)
        , maxDelayMs_(2000)
        , mouseMovementEnabled_(false)
        , charsSinceMouseMove_(0)
        , nextMouseMoveAt_(0)
    {
        scheduleNextMouseMove();
    }
    
    void setDelayRange(int minMs, int maxMs) {
        minDelayMs_ = minMs;
        maxDelayMs_ = maxMs;
    }
    
    void setMouseMovementEnabled(bool enabled) {
        mouseMovementEnabled_ = enabled;
    }
    
    void typeText(const std::string& text, std::atomic<bool>& shouldStop) {
        std::cout << "Starting in 5 seconds...\n";
        for (int i = 5; i > 0 && !shouldStop; --i) {
            std::cout << i << "...\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (shouldStop) return;
        
        std::cout << "Typing...\n";
        
        size_t total = text.length();
        size_t progress = 0;
        
        for (char c : text) {
            if (shouldStop) break;
            
            // Check if we should move mouse
            if (shouldMoveMouse()) {
                performMouseMovement();
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    RandomGenerator::range(TypingConstants::MIN_MOUSE_PAUSE_MS, 
                                          TypingConstants::MAX_MOUSE_PAUSE_MS)));
            }
            
            unsigned char uc = static_cast<unsigned char>(c);
            int holdTime = generateHoldTime(uc);
            simulator_.typeCharacter(uc, holdTime);
            
            int delay = calculateDelay(uc);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            totalCharsTyped_++;
            charsSinceMouseMove_++;
            progress++;
            
            if (progress % 50 == 0) {
                int percent = (progress * 100) / total;
                std::cout << "\rProgress: " << percent << "%";
                std::cout.flush();
            }
        }
        
        std::cout << "\rProgress: 100%\n";
        std::cout << "Completed!\n";
    }
    
private:
    int calculateDelay(unsigned char c) {
        double range = maxDelayMs_ - minDelayMs_;
        double gammaValue = RandomGenerator::gamma(2.0, 1.0);
        double normalized = std::min(gammaValue / 6.0, 1.0);
        
        double delay = minDelayMs_ + range * normalized;
        delay *= rhythmicVariation();
        
        if (std::isdigit(c)) delay *= 1.05;
        if (std::isspace(c)) delay *= 1.12;
        if (c == '\n') delay *= 1.5;
        if (c == '.' || c == '!' || c == '?') delay *= 1.4;
        
        if (RandomGenerator::uniform() < 0.1)
            delay *= 1.3 + RandomGenerator::uniform() * 0.4;
        
        if (shouldBurst())
            delay *= 0.65;
        
        delay *= fatigueFactor_;
        
        double noise = RandomGenerator::normal(0.0, TypingConstants::NOISE_LEVEL);
        delay *= (1.0 + noise);
        
        if (totalCharsTyped_ % TypingConstants::CHARS_BEFORE_FATIGUE_UPDATE == 0) {
            fatigueFactor_ = 1.0 + TypingConstants::MAX_FATIGUE_FACTOR * 
                             std::min(1.0, totalCharsTyped_ / static_cast<double>(TypingConstants::CHARS_FOR_MAX_FATIGUE));
        }
        
        return std::max(TypingConstants::MIN_DELAY_MS, std::min(int(delay), TypingConstants::MAX_DELAY_MS));
    }
    
    int generateHoldTime(unsigned char c) {
        double hold = RandomGenerator::gamma(2.5, 20.0);
        if (std::isupper(c)) hold *= 1.2;
        hold *= (0.9 + RandomGenerator::uniform() * 0.2);
        return std::max(TypingConstants::MIN_HOLD_TIME_MS, std::min(int(hold), TypingConstants::MAX_HOLD_TIME_MS));
    }
    
    double rhythmicVariation() {
        rhythmPhase_ += 0.03;
        double rhythm = std::sin(rhythmPhase_) * 0.5 + 0.5;
        return 0.85 + rhythm * 0.3;
    }
    
    bool shouldBurst() {
        if (burstRemaining_ > 0) {
            burstRemaining_--;
            return true;
        }
        if (RandomGenerator::uniform() < 0.14) {
            burstRemaining_ = RandomGenerator::range(2, 6);
            return true;
        }
        return false;
    }
    
    void scheduleNextMouseMove() {
        nextMouseMoveAt_ = RandomGenerator::range(TypingConstants::MIN_MOUSE_MOVE_INTERVAL_CHARS,
                                                  TypingConstants::MAX_MOUSE_MOVE_INTERVAL_CHARS);
    }
    
    bool shouldMoveMouse() {
        return mouseMovementEnabled_ && charsSinceMouseMove_ >= nextMouseMoveAt_;
    }
    
    void performMouseMovement() {
        int deltaX = RandomGenerator::range(-TypingConstants::MAX_MOUSE_PIXELS, 
                                            TypingConstants::MAX_MOUSE_PIXELS);
        int deltaY = RandomGenerator::range(-TypingConstants::MAX_MOUSE_PIXELS, 
                                            TypingConstants::MAX_MOUSE_PIXELS);
        
        if (deltaX == 0 && deltaY == 0) {
            deltaX = RandomGenerator::range(TypingConstants::MIN_MOUSE_PIXELS, 
                                            TypingConstants::MAX_MOUSE_PIXELS);
        }
        
        mouseSim_.moveRelative(deltaX, deltaY);
        charsSinceMouseMove_ = 0;
        scheduleNextMouseMove();
    }
    
    KeyboardSimulator simulator_;
    MouseSimulator mouseSim_;
    double rhythmPhase_;
    double fatigueFactor_;
    int burstRemaining_;
    int totalCharsTyped_;
    int minDelayMs_;
    int maxDelayMs_;
    bool mouseMovementEnabled_;
    int charsSinceMouseMove_;
    int nextMouseMoveAt_;
};

// ============================================================================
// Simple WebSocket Client (using raw TCP + WebSocket handshake)
// ============================================================================

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

class WebSocketClient {
public:
#if defined(_WIN32) || defined(_WIN64)
    using SocketType = SOCKET;
    static constexpr SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    using SocketType = int;
    static constexpr SocketType INVALID_SOCKET_VALUE = -1;
#endif

    WebSocketClient() {
#if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    bool connect(const std::string& host, int port) {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
#if defined(_WIN32) || defined(_WIN64)
        if (sockfd_ == INVALID_SOCKET) {
#else
        if (sockfd_ < 0) {
#endif
            std::cerr << "Error creating socket\n";
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
#if defined(_WIN32) || defined(_WIN64)
        server_addr.sin_addr.s_addr = inet_addr(host.c_str());
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
#else
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
#endif
            std::cerr << "Invalid address\n";
            return false;
        }
        
        if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed\n";
            return false;
        }
        
        // Send WebSocket handshake
        std::string handshake = 
            "GET / HTTP/1.1\r\n"
            "Host: " + host + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
        
        send(sockfd_, handshake.c_str(), handshake.length(), 0);
        
        // Read response (simplified - should parse properly)
        char buffer[1024];
        recv(sockfd_, buffer, sizeof(buffer), 0);
        
        // Set non-blocking
#if defined(_WIN32) || defined(_WIN64)
        u_long mode = 1;
        ioctlsocket(sockfd_, FIONBIO, &mode);
#else
        fcntl(sockfd_, F_SETFL, O_NONBLOCK);
#endif
        
        std::cout << "Connected to server\n";
        return true;
    }
    
    void sendMessage(const std::string& message) {
        // Simplified WebSocket frame (text frame)
        std::string frame;
        frame += (char)0x81; // FIN + text frame
        
        size_t len = message.length();
        if (len < 126) {
            frame += (char)(0x80 | len); // Masked + length
        } else {
            frame += (char)(0x80 | 126);
            frame += (char)((len >> 8) & 0xFF);
            frame += (char)(len & 0xFF);
        }
        
        // Masking key (simple)
        char mask[4] = {0x12, 0x34, 0x56, 0x78};
        frame.append(mask, 4);
        
        // Masked payload
        for (size_t i = 0; i < len; ++i) {
            frame += message[i] ^ mask[i % 4];
        }
        
        send(sockfd_, frame.c_str(), frame.length(), 0);
    }
    
    std::string receiveMessage() {
        char buffer[4096];
        ssize_t n = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
        
        if (n <= 0) return "";
        
        // Parse WebSocket frame (simplified)
        if (n < 2) return "";
        
        unsigned char frame_type = buffer[0];
        unsigned char payload_len = buffer[1] & 0x7F;
        
        size_t offset = 2;
        if (payload_len == 126) {
            offset = 4;
        } else if (payload_len == 127) {
            offset = 10;
        }
        
        std::string message(buffer + offset, n - offset);
        return message;
    }
    
    ~WebSocketClient() {
#if defined(_WIN32) || defined(_WIN64)
        if (sockfd_ != INVALID_SOCKET) {
            closesocket(sockfd_);
            WSACleanup();
        }
#else
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
#endif
    }
    
private:
    SocketType sockfd_ = INVALID_SOCKET_VALUE;
};

// ============================================================================
// JSON Utilities
// ============================================================================

std::string unescapeJsonString(const std::string& str) {
    std::string result;
    result.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case 'n': result += '\n'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '"': result += '"'; ++i; break;
                case '/': result += '/'; ++i; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }

    return result;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <server_ip>\n";
        std::cout << "Example: " << argv[0] << " 192.168.1.100\n";
        return 1;
    }
    
    std::string server_ip = argv[1];
    int server_port = 9999;
    
    WebSocketClient ws;
    if (!ws.connect(server_ip, server_port)) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }
    
    // Send ready message
    ws.sendMessage(R"({"type":"ready"})");
    
    TypingEngine engine;
    MouseSimulator mouseSim;
    std::atomic<bool> shouldStop(false);
    std::atomic<bool> isBusy(false);
    std::atomic<bool> scrollEnabled(false);  // Enable via command or startup
    
    // Idle scroll thread - runs independently
    std::thread idleScrollThread([&mouseSim, &scrollEnabled]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (!scrollEnabled.load()) continue;
            
            int64_t idleMs = IdleDetector::getIdleTimeMs();
            
            // If idle for more than 30 seconds, scroll
            if (idleMs >= 30000) {
                int amount = RandomGenerator::range(TypingConstants::MIN_SCROLL_AMOUNT,
                                                    TypingConstants::MAX_SCROLL_AMOUNT);
                
                // 80% chance to scroll down, 20% to scroll up
                if (RandomGenerator::uniform() > TypingConstants::SCROLL_DOWN_PROBABILITY) {
                    amount = -amount;
                }
                
                mouseSim.scroll(amount);
            }
        }
    });
    idleScrollThread.detach();

    std::cout << "Client ready. Waiting for commands from server...\n";
    std::cout << "Press Ctrl+C to exit\n\n";
    
    while (true) {
        std::string message = ws.receiveMessage();
        
        if (!message.empty()) {
            std::cout << "Received: " << message << "\n";
            
            // Parse JSON (simplified - use real JSON library)
            if (message.find("\"type\":\"start_typing\"") != std::string::npos) {
                // Check if already busy
                if (isBusy) {
                    std::cout << "Client is busy, ignoring command\n";
                    continue;
                }

                // Extract text (very basic parsing)
                size_t textPos = message.find("\"text\":\"");
                if (textPos != std::string::npos) {
                    size_t start = textPos + 8;
                    size_t end = message.find("\"", start);

                    // Handle escaped quotes within the text
                    while (end != std::string::npos && end > 0 && message[end - 1] == '\\') {
                        end = message.find("\"", end + 1);
                    }

                    std::string rawText = message.substr(start, end - start);
                    std::string text = unescapeJsonString(rawText);

                    std::cout << "Text to type: " << text.length() << " characters\n";

                    // Extract settings (minDelay and maxDelay)
                    int minDelay = 120;  // defaults
                    int maxDelay = 2000;
                    
                    size_t minDelayPos = message.find("\"minDelay\":");
                    if (minDelayPos != std::string::npos) {
                        size_t numStart = minDelayPos + 11;
                        size_t numEnd = message.find_first_of(",}", numStart);
                        if (numEnd != std::string::npos) {
                            std::string minDelayStr = message.substr(numStart, numEnd - numStart);
                            try {
                                minDelay = std::stoi(minDelayStr);
                            } catch (...) {}
                        }
                    }
                    
                    size_t maxDelayPos = message.find("\"maxDelay\":");
                    if (maxDelayPos != std::string::npos) {
                        size_t numStart = maxDelayPos + 11;
                        size_t numEnd = message.find_first_of(",}", numStart);
                        if (numEnd != std::string::npos) {
                            std::string maxDelayStr = message.substr(numStart, numEnd - numStart);
                            try {
                                maxDelay = std::stoi(maxDelayStr);
                            } catch (...) {}
                        }
                    }
                    
                    std::cout << "Using delay range: " << minDelay << "ms - " << maxDelay << "ms\n";
                    engine.setDelayRange(minDelay, maxDelay);

                    // Extract mouse movement setting
                    bool mouseMovement = false;
                    size_t mousePos = message.find("\"mouseMovement\":");
                    if (mousePos != std::string::npos) {
                        size_t boolStart = mousePos + 16;
                        if (message.substr(boolStart, 4) == "true") {
                            mouseMovement = true;
                        }
                    }
                    engine.setMouseMovementEnabled(mouseMovement);
                    std::cout << "Mouse movement: " << (mouseMovement ? "enabled" : "disabled") << "\n";
                    
                    // Extract idle scroll setting
                    size_t scrollPos = message.find("\"idleScroll\":");
                    if (scrollPos != std::string::npos) {
                        size_t boolStart = scrollPos + 13;
                        if (message.substr(boolStart, 4) == "true") {
                            scrollEnabled.store(true);
                            std::cout << "Idle scrolling: enabled (30s delay)\n";
                        } else {
                            scrollEnabled.store(false);
                            std::cout << "Idle scrolling: disabled\n";
                        }
                    }

                    // Reset stop flag and set busy state
                    shouldStop = false;
                    isBusy = true;
                    ws.sendMessage(R"({"type":"status","status":"busy"})");

                    // Start typing in separate thread
                    std::thread([&engine, text, &shouldStop, &ws, &isBusy]() {
                        engine.typeText(text, shouldStop);
                        // Mark as free and notify server
                        isBusy = false;
                        ws.sendMessage(R"({"type":"status","status":"free"})");
                    }).detach();
                }
            }
            else if (message.find("\"type\":\"stop_typing\"") != std::string::npos) {
                shouldStop = true;
                std::cout << "Stop command received\n";
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}
