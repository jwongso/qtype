// qtype_client.cpp - macOS Console Client with WebSocket
// Compile: clang++ qtype_client.cpp -o qtype_client -std=c++17 -framework ApplicationServices -framework CoreFoundation -lcurl

#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>
#include <atomic>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // Or use simple JSON parsing

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
        static bool hasSpare = false;
        static double spare;
        
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
// macOS Keyboard Simulator
// ============================================================================

class MacKeyboardSimulator {
public:
    void typeCharacter(UniChar c, int holdTimeMs) {
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
            CGEventKeyboardSetUnicodeString(down, 1, &c);
            CGEventKeyboardSetUnicodeString(up, 1, &c);
            
            CGEventPost(kCGHIDEventTap, down);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdTimeMs));
            CGEventPost(kCGHIDEventTap, up);
            
            CFRelease(down);
            CFRelease(up);
        }
    }
    
    void releaseAllKeys() {
        // Not needed on macOS typically
    }
};

// ============================================================================
// Typing Engine
// ============================================================================

class TypingEngine {
public:
    TypingEngine() 
        : rhythmPhase_(RandomGenerator::uniform() * 6.28318530718)
        , fatigueFactor_(1.0)
        , burstRemaining_(0)
        , totalCharsTyped_(0)
        , minDelayMs_(120)
        , maxDelayMs_(2000)
    {}
    
    void setDelayRange(int minMs, int maxMs) {
        minDelayMs_ = minMs;
        maxDelayMs_ = maxMs;
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
            
            UniChar uc = static_cast<unsigned char>(c);
            int holdTime = generateHoldTime(uc);
            simulator_.typeCharacter(uc, holdTime);
            
            int delay = calculateDelay(uc);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            totalCharsTyped_++;
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
    int calculateDelay(UniChar c) {
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
        
        double noise = RandomGenerator::normal(0.0, 0.15);
        delay *= (1.0 + noise);
        
        if (totalCharsTyped_ % 50 == 0) {
            fatigueFactor_ = 1.0 + 0.25 * std::min(1.0, totalCharsTyped_ / 1000.0);
        }
        
        return std::max(15, std::min(int(delay), 8000));
    }
    
    int generateHoldTime(UniChar c) {
        double hold = RandomGenerator::gamma(2.5, 20.0);
        if (std::isupper(c)) hold *= 1.2;
        hold *= (0.9 + RandomGenerator::uniform() * 0.2);
        return std::max(40, std::min(int(hold), 180));
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
    
    MacKeyboardSimulator simulator_;
    double rhythmPhase_;
    double fatigueFactor_;
    int burstRemaining_;
    int totalCharsTyped_;
    int minDelayMs_;
    int maxDelayMs_;
};

// ============================================================================
// Simple WebSocket Client (using raw TCP + WebSocket handshake)
// ============================================================================

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

class WebSocketClient {
public:
    bool connect(const std::string& host, int port) {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            std::cerr << "Error creating socket\n";
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
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
        fcntl(sockfd_, F_SETFL, O_NONBLOCK);
        
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
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }
    
private:
    int sockfd_ = -1;
};

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
    std::atomic<bool> shouldStop(false);
    
    std::cout << "Client ready. Waiting for commands from server...\n";
    std::cout << "Press Ctrl+C to exit\n\n";
    
    while (true) {
        std::string message = ws.receiveMessage();
        
        if (!message.empty()) {
            std::cout << "Received: " << message << "\n";
            
            // Parse JSON (simplified - use real JSON library)
            if (message.find("\"type\":\"start_typing\"") != std::string::npos) {
                // Extract text (very basic parsing)
                size_t textPos = message.find("\"text\":\"");
                if (textPos != std::string::npos) {
                    size_t start = textPos + 8;
                    size_t end = message.find("\"", start);
                    std::string text = message.substr(start, end - start);
                    
                    // Start typing in separate thread
                    std::thread([&engine, text, &shouldStop]() {
                        engine.typeText(text, shouldStop);
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
