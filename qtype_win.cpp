// qtype_console.cpp - Windows Console Application (No Qt Required)
// Compile: cl qtype_console.cpp /EHsc /std:c++17
// Or with g++: g++ qtype_console.cpp -o qtype.exe -std=c++17

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

// ============================================================================
// Random Number Generator
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
// Timing Profile
// ============================================================================

struct TimingProfile {
    double baseSpeedFactor = 1.0;
    double microStutterProb = 0.1;
    double burstProb = 0.14;
    int burstMin = 2;
    int burstMax = 6;
    double gammaShape = 2.0;
    double gammaScale = 1.0;
    double noiseLevel = 0.15;
    int minDelayMs = 120;
    int maxDelayMs = 2000;

    static TimingProfile humanAdvanced() {
        TimingProfile p;
        p.baseSpeedFactor = 1.0;
        p.microStutterProb = 0.1;
        p.burstProb = 0.14;
        p.gammaShape = 2.0;
        p.gammaScale = 1.0;
        p.noiseLevel = 0.15;
        return p;
    }
};

// ============================================================================
// Keyboard Simulator
// ============================================================================

class KeyboardSimulator {
public:
    void typeCharacter(wchar_t c, int holdTimeMs) {
        if (c == L'\n') {
            sendKeyEvent(VK_SHIFT, 0);
            Sleep(10);
            sendKeyEvent(VK_RETURN, 0);
            Sleep(holdTimeMs);
            sendKeyEvent(VK_RETURN, KEYEVENTF_KEYUP);
            Sleep(10);
            sendKeyEvent(VK_SHIFT, KEYEVENTF_KEYUP);
            return;
        }

        if (c == L'\t') {
            sendKeyEvent(VK_TAB, 0);
            Sleep(holdTimeMs);
            sendKeyEvent(VK_TAB, KEYEVENTF_KEYUP);
            return;
        }

        sendUnicodeChar(c, holdTimeMs);
    }

    void releaseAllKeys() {
        WORD modifiers[] = {
            VK_SHIFT, VK_CONTROL, VK_MENU,
            VK_LSHIFT, VK_RSHIFT,
            VK_LCONTROL, VK_RCONTROL,
            VK_LMENU, VK_RMENU,
            VK_LWIN, VK_RWIN
        };

        for (WORD vk : modifiers) {
            sendKeyEvent(vk, KEYEVENTF_KEYUP);
        }
    }

private:
    void sendKeyEvent(WORD vk, DWORD flags) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = flags;
        SendInput(1, &input, sizeof(INPUT));
    }

    void sendUnicodeChar(wchar_t ch, int holdTimeMs) {
        INPUT inputs[2] = {0};

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = 0;
        inputs[0].ki.wScan = ch;
        inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 0;
        inputs[1].ki.wScan = ch;
        inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        SendInput(1, &inputs[0], sizeof(INPUT));
        Sleep(holdTimeMs);
        SendInput(1, &inputs[1], sizeof(INPUT));
    }
};

// ============================================================================
// Typing Engine
// ============================================================================

class TypingEngine {
public:
    TypingEngine(const TimingProfile& profile)
        : profile_(profile)
        , rhythmPhase_(RandomGenerator::uniform() * 6.28318530718)
        , fatigueFactor_(1.0)
        , burstRemaining_(0)
        , totalCharsTyped_(0)
    {}

    void typeText(const std::wstring& text) {
        std::wcout << L"Starting in 5 seconds... (Switch to target window)\n";
        for (int i = 5; i > 0; --i) {
            std::wcout << i << L"...\n";
            Sleep(1000);
        }
        std::wcout << L"Typing...\n\n";

        size_t total = text.length();
        size_t progress = 0;

        for (wchar_t c : text) {
            int holdTime = generateHoldTime(c);
            simulator_.typeCharacter(c, holdTime);

            int delay = calculateDelay(c);
            Sleep(delay);

            totalCharsTyped_++;
            progress++;

            // Update progress every 50 chars
            if (progress % 50 == 0) {
                int percent = (progress * 100) / total;
                std::wcout << L"\rProgress: " << percent << L"%";
                std::wcout.flush();
            }

            // Check for ESC key to stop
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                std::wcout << L"\n\nStopped by user (ESC pressed)\n";
                simulator_.releaseAllKeys();
                return;
            }
        }

        std::wcout << L"\rProgress: 100%\n";
        std::wcout << L"\nCompleted!\n";
        simulator_.releaseAllKeys();
    }

private:
    int calculateDelay(wchar_t c) {
        double range = profile_.maxDelayMs - profile_.minDelayMs;
        double gammaValue = RandomGenerator::gamma(profile_.gammaShape, profile_.gammaScale);
        double normalized = std::min(gammaValue / 6.0, 1.0);

        double delay = profile_.minDelayMs + range * normalized;
        delay *= rhythmicVariation();

        if (std::iswdigit(c)) delay *= 1.05;
        if (std::iswspace(c)) delay *= 1.12;
        if (c == L'\n') delay *= 1.5;
        if (c == L'.' || c == L'!' || c == L'?') delay *= 1.4;

        if (RandomGenerator::uniform() < profile_.microStutterProb)
            delay *= 1.3 + RandomGenerator::uniform() * 0.4;

        if (shouldBurst())
            delay *= 0.65;

        delay *= fatigueFactor_;

        double noise = RandomGenerator::normal(0.0, profile_.noiseLevel);
        delay *= (1.0 + noise);

        // Update fatigue
        if (totalCharsTyped_ % 50 == 0) {
            fatigueFactor_ = 1.0 + 0.25 * std::min(1.0, totalCharsTyped_ / 1000.0);
        }

        return std::max(15, std::min(int(delay), 8000));
    }

    int generateHoldTime(wchar_t c) {
        double hold = RandomGenerator::gamma(2.5, 20.0);
        if (std::iswupper(c)) hold *= 1.2;
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
        if (RandomGenerator::uniform() < profile_.burstProb) {
            burstRemaining_ = RandomGenerator::range(profile_.burstMin, profile_.burstMax);
            return true;
        }
        return false;
    }

    KeyboardSimulator simulator_;
    TimingProfile profile_;
    double rhythmPhase_;
    double fatigueFactor_;
    int burstRemaining_;
    int totalCharsTyped_;
};

// ============================================================================
// Main
// ============================================================================

std::wstring readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << filename << "\n";
        return L"";
    }

    // Read file into string
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // Convert to wstring (UTF-8 to UTF-16)
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, content.c_str(),
                                          (int)content.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), (int)content.size(),
                       &wstr[0], size_needed);

    return wstr;
}

void showUsage(const char* progName) {
    std::cout << "qtype - Human-like typing simulator\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " -i <input_file>\n";
    std::cout << "  " << progName << " --input <input_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -i, --input FILE    Path to text file to type\n";
    std::cout << "  -h, --help          Show this help message\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " -i mytext.txt\n\n";
    std::cout << "Press ESC during typing to stop.\n";
}

int main(int argc, char* argv[]) {
    // Enable UTF-8 console output
    SetConsoleOutputCP(CP_UTF8);

    std::string inputFile;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            showUsage(argv[0]);
            return 0;
        }

        if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                inputFile = argv[++i];
            } else {
                std::cerr << "Error: -i requires a filename\n";
                showUsage(argv[0]);
                return 1;
            }
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n\n";
        showUsage(argv[0]);
        return 1;
    }

    // Read input file
    std::wstring text = readFile(inputFile);
    if (text.empty()) {
        std::cerr << "Error: File is empty or could not be read\n";
        return 1;
    }

    std::wcout << L"Loaded " << text.length() << L" characters from "
               << inputFile.c_str() << L"\n\n";

    // Create engine and type
    TimingProfile profile = TimingProfile::humanAdvanced();
    TypingEngine engine(profile);

    try {
        engine.typeText(text);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
