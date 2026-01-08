// typing_engine.h - Complete typing engine with all implementations
#ifndef TYPING_ENGINE_H
#define TYPING_ENGINE_H

#include <QString>
#include <QChar>
#include <QRandomGenerator>
#include <QThread>
#include <QProcess>
#include <climits>
#include <cmath>
#include <memory>

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
    
    // Word chunking
    constexpr int MAX_CHUNK_LENGTH = 12;
    
    // Thinking pauses
    constexpr int MIN_WORDS_BEFORE_PAUSE = 8;
    constexpr int MAX_WORDS_BEFORE_PAUSE = 15;
    constexpr double THINKING_PAUSE_PROBABILITY = 0.3;
    
    // Backspace timing
    constexpr int BACKSPACE_HOLD_MS = 10;
    constexpr int MIN_BACKSPACE_DELAY_MS = 40;
    constexpr int MAX_BACKSPACE_DELAY_MS = 90;
    constexpr int MIN_CORRECTION_DELAY_MS = 60;
    constexpr int MAX_CORRECTION_DELAY_MS = 160;
    
    // Double key timing
    constexpr int MIN_DOUBLE_KEY_DELAY_MS = 10;
    constexpr int MAX_DOUBLE_KEY_DELAY_MS = 40;
    
    // Platform-specific delays
    constexpr int MAC_SHIFT_DELAY_MS = 10;
    
    // Mouse movement
    constexpr int MIN_MOUSE_MOVE_INTERVAL_CHARS = 20;
    constexpr int MAX_MOUSE_MOVE_INTERVAL_CHARS = 60;
    constexpr int MIN_MOUSE_PIXELS = 3;
    constexpr int MAX_MOUSE_PIXELS = 15;
    constexpr int MIN_MOUSE_PAUSE_MS = 100;
    constexpr int MAX_MOUSE_PAUSE_MS = 300;
    
    // Scroll
    constexpr int MIN_SCROLL_INTERVAL_CHARS = 40;
    constexpr int MAX_SCROLL_INTERVAL_CHARS = 120;
    constexpr int MIN_SCROLL_AMOUNT = 1;
    constexpr int MAX_SCROLL_AMOUNT = 3;
    constexpr int MIN_SCROLL_PAUSE_MS = 150;
    constexpr int MAX_SCROLL_PAUSE_MS = 400;
    constexpr double SCROLL_DOWN_PROBABILITY = 0.8; // 80% scroll down, 20% up
}

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

// Forward declarations
class IKeyboardSimulator;
class IMouseSimulator;

// ============================================================================
// Profile & Settings Structs
// ============================================================================

struct TimingProfile {
    double baseSpeedFactor = 1.0;
    double microStutterProb = 0.1;
    double idlePauseProb = 0.009;
    double burstProb = 0.14;
    int burstMin = 2;
    int burstMax = 6;
    double gammaShape = 2.0;
    double gammaScale = 1.0;
    double noiseLevel = 0.15;
    
    static TimingProfile humanAdvanced();
    static TimingProfile fastHuman();
    static TimingProfile slowTired();
    static TimingProfile professional();
};

struct ImperfectionSettings {
    bool enableTypos = true;
    int typoMin = 300;
    int typoMax = 500;
    
    bool enableDoubleKeys = true;
    int doubleMin = 250;
    int doubleMax = 400;
    
    bool enableAutoCorrection = true;
    int correctionProbability = 15;
};

struct DelayRange {
    int minMs = 80;
    int maxMs = 180;
};

enum class KeyboardLayoutType {
    US_QWERTY,
    UK_QWERTY,
    GERMAN_QWERTZ,
    FRENCH_AZERTY
};

// ============================================================================
// Random Number Generator
// ============================================================================

class RandomGenerator {
public:
    static double gamma(double shape, double scale);
    static double normal(double mean, double stddev);
    static int range(int min, int max);
    static double uniform();
};

// ============================================================================
// Keyboard Layout Logic
// ============================================================================

class KeyboardLayout {
public:
    explicit KeyboardLayout(KeyboardLayoutType type = KeyboardLayoutType::US_QWERTY);
    
    QChar getNeighborKey(QChar c) const;
    bool isLetter(QChar c) const;
    
private:
    KeyboardLayoutType type_;
    QString rows_[3];
    
    void initializeLayout();
};

// ============================================================================
// Typing Dynamics Calculator
// ============================================================================

class TypingDynamics {
public:
    TypingDynamics(const TimingProfile& profile, const DelayRange& delays);
    
    void reset();
    void updateState(QChar currentChar);
    
    int calculateDelay(QChar ch, bool isSentenceEnd, bool isBurst, bool isThinkingPause);
    int generateHoldTime(QChar ch);
    
    bool shouldBurst();
    bool shouldThinkingPause(int wordsSinceBreak);
    
    double digraphFactor(QChar prev, QChar curr);
    
private:
    TimingProfile profile_;
    DelayRange delays_;
    
    QChar previousChar_;
    double rhythmPhase_;
    double fatigueFactor_;
    int burstRemaining_;
    int totalCharsTyped_;
    
    double rhythmicVariation();
};

// ============================================================================
// Imperfection Generator
// ============================================================================

struct ImperfectionResult {
    QChar character;
    bool shouldDouble = false;
    bool shouldCorrect = false;
};

class ImperfectionGenerator {
public:
    ImperfectionGenerator(const ImperfectionSettings& settings, 
                          const KeyboardLayout& layout);
    
    void reset();
    ImperfectionResult processCharacter(QChar original);
    
private:
    ImperfectionSettings settings_;
    const KeyboardLayout& layout_;
    
    int charsTypedTotal_;
    int charsSinceLastTypo_;
    int charsSinceLastDouble_;
    int nextTypoAt_;
    int nextDoubleAt_;
    
    void scheduleNextTypo();
    void scheduleNextDouble();
};

// ============================================================================
// Text Chunker
// ============================================================================

class TextChunker {
public:
    TextChunker(const QString& text);
    
    bool hasMore() const;
    QString nextChunk();
    int currentPosition() const { return currentIndex_; }
    int totalLength() const { return text_.length(); }
    int progressPercent() const;
    
private:
    QString text_;
    int currentIndex_;
};

// ============================================================================
// Keyboard Simulator Interface
// ============================================================================

class IKeyboardSimulator {
public:
    virtual ~IKeyboardSimulator() = default;
    
    virtual void typeCharacter(QChar c, int holdTimeMs) = 0;
    virtual void pressBackspace() = 0;
    virtual void releaseAllKeys() = 0;
};

// ============================================================================
// Mouse Simulator Interface
// ============================================================================

class IMouseSimulator {
public:
    virtual ~IMouseSimulator() = default;
    
    virtual void moveRelative(int deltaX, int deltaY) = 0;
    virtual void scroll(int amount) = 0;  // Positive = down, negative = up
};

// ============================================================================
// Platform-Specific Implementations
// ============================================================================

#ifdef Q_OS_LINUX
class LinuxKeyboardSimulator : public IKeyboardSimulator {
public:
    void typeCharacter(QChar c, int holdTimeMs) override;
    void pressBackspace() override;
    void releaseAllKeys() override;
    
private:
    void sendKey(int keycode, int holdMs);
};

class LinuxMouseSimulator : public IMouseSimulator {
public:
    void moveRelative(int deltaX, int deltaY) override;
    void scroll(int amount) override;
};
#endif

#ifdef Q_OS_MAC
class MacKeyboardSimulator : public IKeyboardSimulator {
public:
    void typeCharacter(QChar c, int holdTimeMs) override;
    void pressBackspace() override;
    void releaseAllKeys() override;
};

class MacMouseSimulator : public IMouseSimulator {
public:
    void moveRelative(int deltaX, int deltaY) override;
    void scroll(int amount) override;
};
#endif

// ============================================================================
// Main Typing Engine
// ============================================================================

class TypingEngine {
public:
    TypingEngine(IKeyboardSimulator* simulator,
                 IMouseSimulator* mouseSimulator,
                 const TimingProfile& profile,
                 const DelayRange& delays,
                 const ImperfectionSettings& imperfections,
                 KeyboardLayoutType layoutType = KeyboardLayoutType::US_QWERTY);
    
    ~TypingEngine();
    
    void setText(const QString& text);
    bool hasMoreToType() const;
    void setMouseMovementEnabled(bool enabled);
    
    int typeNextChunk();
    
    int progressPercent() const;
    void reset();
    
    int getSkippedCharCount() const { return skippedCharCount_; }
    QString getSkippedCharsPreview() const { return skippedCharsPreview_; }
    
private:
    IKeyboardSimulator* simulator_;
    IMouseSimulator* mouseSimulator_;
    TimingProfile profile_;
    DelayRange delays_;
    ImperfectionSettings imperfections_;
    KeyboardLayout layout_;
    
    std::unique_ptr<TextChunker> chunker_;
    std::unique_ptr<TypingDynamics> dynamics_;
    std::unique_ptr<ImperfectionGenerator> imperfectionGen_;
    
    int wordsSinceBreak_;
    bool mouseMovementEnabled_;
    int charsSinceMouseMove_;
    int nextMouseMoveAt_;
    int skippedCharCount_;
    QString skippedCharsPreview_;
    
    void scheduleNextMouseMove();
    bool shouldMoveMouse();
    void performMouseMovement();
    bool isTypeable(QChar c) const;
    void recordSkippedChar(QChar c);
};

// ============================================================================
// IMPLEMENTATIONS
// ============================================================================

// TimingProfile
inline TimingProfile TimingProfile::humanAdvanced() {
    TimingProfile p;
    p.baseSpeedFactor = 1.0;
    p.microStutterProb = 0.1;
    p.idlePauseProb = 0.009;
    p.burstProb = 0.14;
    p.burstMin = 2;
    p.burstMax = 6;
    p.gammaShape = 2.0;
    p.gammaScale = 1.0;
    p.noiseLevel = 0.15;
    return p;
}

inline TimingProfile TimingProfile::fastHuman() {
    TimingProfile p;
    p.baseSpeedFactor = 0.7;
    p.microStutterProb = 0.06;
    p.idlePauseProb = 0.004;
    p.burstProb = 0.2;
    p.burstMin = 3;
    p.burstMax = 8;
    p.gammaShape = 1.8;
    p.gammaScale = 0.9;
    p.noiseLevel = 0.12;
    return p;
}

inline TimingProfile TimingProfile::slowTired() {
    TimingProfile p;
    p.baseSpeedFactor = 1.5;
    p.microStutterProb = 0.15;
    p.idlePauseProb = 0.025;
    p.burstProb = 0.08;
    p.burstMin = 2;
    p.burstMax = 4;
    p.gammaShape = 2.5;
    p.gammaScale = 1.3;
    p.noiseLevel = 0.22;
    return p;
}

inline TimingProfile TimingProfile::professional() {
    TimingProfile p;
    p.baseSpeedFactor = 0.75;
    p.microStutterProb = 0.04;
    p.idlePauseProb = 0.003;
    p.burstProb = 0.25;
    p.burstMin = 4;
    p.burstMax = 10;
    p.gammaShape = 1.6;
    p.gammaScale = 0.85;
    p.noiseLevel = 0.08;
    return p;
}

// RandomGenerator
inline double RandomGenerator::gamma(double shape, double scale) {
    if (shape < 1.0) {
        return gamma(1.0 + shape, scale) * 
               std::pow(uniform(), 1.0 / shape);
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

inline double RandomGenerator::normal(double mean, double stddev) {
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

inline int RandomGenerator::range(int min, int max) {
    if (min > max) std::swap(min, max);
    return QRandomGenerator::global()->bounded(min, max + 1);
}

inline double RandomGenerator::uniform() {
    return QRandomGenerator::global()->generateDouble();
}

// KeyboardLayout
inline KeyboardLayout::KeyboardLayout(KeyboardLayoutType type)
    : type_(type)
{
    initializeLayout();
}

inline void KeyboardLayout::initializeLayout() {
    switch (type_) {
        case KeyboardLayoutType::US_QWERTY:
            rows_[0] = "qwertyuiop";
            rows_[1] = "asdfghjkl";
            rows_[2] = "zxcvbnm";
            break;
        case KeyboardLayoutType::UK_QWERTY:
            rows_[0] = "qwertyuiop";
            rows_[1] = "asdfghjkl";
            rows_[2] = "zxcvbnm";
            break;
        case KeyboardLayoutType::GERMAN_QWERTZ:
            rows_[0] = "qwertzuiop";
            rows_[1] = "asdfghjkl";
            rows_[2] = "yxcvbnm";
            break;
        case KeyboardLayoutType::FRENCH_AZERTY:
            rows_[0] = "azertyuiop";
            rows_[1] = "qsdfghjklm";
            rows_[2] = "wxcvbn";
            break;
    }
}

inline QChar KeyboardLayout::getNeighborKey(QChar c) const {
    bool upper = c.isUpper();
    QChar lower = c.toLower();
    
    int rowIndex = -1;
    int colIndex = -1;
    
    for (int r = 0; r < 3; ++r) {
        int idx = rows_[r].indexOf(lower);
        if (idx != -1) {
            rowIndex = r;
            colIndex = idx;
            break;
        }
    }
    
    if (rowIndex == -1) return c;
    
    QList<QChar> candidates;
    
    auto addIfValid = [&](int r, int col) {
        if (r < 0 || r >= 3) return;
        const QString &row = rows_[r];
        if (col < 0 || col >= row.size()) return;
        QChar ch = row[col];
        if (!candidates.contains(ch))
            candidates.append(ch);
    };
    
    addIfValid(rowIndex, colIndex - 1);
    addIfValid(rowIndex, colIndex + 1);
    addIfValid(rowIndex - 1, colIndex);
    addIfValid(rowIndex + 1, colIndex);
    addIfValid(rowIndex - 1, colIndex - 1);
    addIfValid(rowIndex - 1, colIndex + 1);
    addIfValid(rowIndex + 1, colIndex - 1);
    addIfValid(rowIndex + 1, colIndex + 1);
    
    if (candidates.isEmpty()) return c;
    
    QChar out = candidates[RandomGenerator::range(0, candidates.size() - 1)];
    return upper ? out.toUpper() : out;
}

inline bool KeyboardLayout::isLetter(QChar c) const {
    return c.isLetter();
}

// TypingDynamics
inline TypingDynamics::TypingDynamics(const TimingProfile& profile, const DelayRange& delays)
    : profile_(profile)
    , delays_(delays)
    , previousChar_()
    , rhythmPhase_(RandomGenerator::uniform() * TypingConstants::TWO_PI)
    , fatigueFactor_(1.0)
    , burstRemaining_(0)
    , totalCharsTyped_(0)
{}

inline void TypingDynamics::reset() {
    previousChar_ = QChar();
    rhythmPhase_ = RandomGenerator::uniform() * TypingConstants::TWO_PI;
    fatigueFactor_ = 1.0;
    burstRemaining_ = 0;
    totalCharsTyped_ = 0;
}

inline void TypingDynamics::updateState(QChar currentChar) {
    previousChar_ = currentChar;
    totalCharsTyped_++;
    
    if (totalCharsTyped_ % TypingConstants::CHARS_BEFORE_FATIGUE_UPDATE == 0) {
        fatigueFactor_ = 1.0 + TypingConstants::MAX_FATIGUE_FACTOR * 
                         std::min(1.0, totalCharsTyped_ / static_cast<double>(TypingConstants::CHARS_FOR_MAX_FATIGUE));
    }
}

inline bool TypingDynamics::shouldBurst() {
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

inline bool TypingDynamics::shouldThinkingPause(int wordsSinceBreak) {
    return wordsSinceBreak > RandomGenerator::range(TypingConstants::MIN_WORDS_BEFORE_PAUSE, 
                                                     TypingConstants::MAX_WORDS_BEFORE_PAUSE) &&
           RandomGenerator::uniform() < TypingConstants::THINKING_PAUSE_PROBABILITY;
}

inline double TypingDynamics::rhythmicVariation() {
    rhythmPhase_ += 0.03;
    double rhythm = std::sin(rhythmPhase_) * 0.5 + 0.5;
    return 0.85 + rhythm * 0.3;
}

inline double TypingDynamics::digraphFactor(QChar prev, QChar curr) {
    QString digraph = QString(prev) + QString(curr);
    static const QStringList fast = {"th","he","in","er","an","re","on","at","en","nd"};
    
    if (fast.contains(digraph.toLower())) {
        return 0.75;
    }
    
    if ((prev == 'q' && curr == 'z') ||
        (prev == 'z' && curr == 'q') ||
        (prev == 'p' && curr == 'q')) {
        return 1.4;
    }
    
    static const QString leftHand = "qwertasdfgzxcvb";
    static const QString rightHand = "yuiophjklnm";
    
    bool bothLeft = leftHand.contains(prev.toLower()) && leftHand.contains(curr.toLower());
    bool bothRight = rightHand.contains(prev.toLower()) && rightHand.contains(curr.toLower());
    
    if (bothLeft || bothRight) {
        return 1.08;
    }
    
    return 1.0;
}

inline int TypingDynamics::calculateDelay(QChar ch, bool isSentenceEnd, bool isBurst, bool isThinkingPause) {
    double range = delays_.maxMs - delays_.minMs;
    double gammaValue = RandomGenerator::gamma(profile_.gammaShape, profile_.gammaScale);
    double normalized = std::min(gammaValue / 6.0, 1.0);
    
    double delay = delays_.minMs + range * normalized;
    delay *= rhythmicVariation();
    
    if (ch.isDigit()) delay *= 1.05;
    if (ch.isSpace()) delay *= 1.12;
    if (ch == '\n') delay *= 1.5;
    if (ch == '.' || ch == '!' || ch == '?') delay *= 1.4;
    
    if (previousChar_ != QChar()) {
        delay *= digraphFactor(previousChar_, ch);
    }
    
    if (isSentenceEnd)
        delay += RandomGenerator::gamma(2.0, 150);
    
    if (isThinkingPause)
        delay += RandomGenerator::gamma(3.0, 800);
    
    if (RandomGenerator::uniform() < profile_.microStutterProb)
        delay *= 1.3 + RandomGenerator::uniform() * 0.4;
    
    if (isBurst)
        delay *= 0.65;
    
    delay *= fatigueFactor_;
    
    double noise = RandomGenerator::normal(0.0, profile_.noiseLevel);
    delay *= (1.0 + noise);
    
    return qBound(TypingConstants::MIN_DELAY_MS, int(delay), TypingConstants::MAX_DELAY_MS);
}

inline int TypingDynamics::generateHoldTime(QChar ch) {
    double hold = RandomGenerator::gamma(2.5, 20.0);
    
    if (ch.isUpper()) {
        hold *= 1.2;
    }
    
    hold *= (0.9 + RandomGenerator::uniform() * 0.2);
    
    return qBound(TypingConstants::MIN_HOLD_TIME_MS, int(hold), TypingConstants::MAX_HOLD_TIME_MS);
}

// ImperfectionGenerator
inline ImperfectionGenerator::ImperfectionGenerator(const ImperfectionSettings& settings,
                                                     const KeyboardLayout& layout)
    : settings_(settings)
    , layout_(layout)
    , charsTypedTotal_(0)
    , charsSinceLastTypo_(0)
    , charsSinceLastDouble_(0)
    , nextTypoAt_(INT_MAX)
    , nextDoubleAt_(INT_MAX)
{
    reset();
}

inline void ImperfectionGenerator::reset() {
    charsTypedTotal_ = 0;
    charsSinceLastTypo_ = 0;
    charsSinceLastDouble_ = 0;
    scheduleNextTypo();
    scheduleNextDouble();
}

inline void ImperfectionGenerator::scheduleNextTypo() {
    if (settings_.enableTypos) {
        nextTypoAt_ = RandomGenerator::range(settings_.typoMin, settings_.typoMax);
    } else {
        nextTypoAt_ = INT_MAX;
    }
}

inline void ImperfectionGenerator::scheduleNextDouble() {
    if (settings_.enableDoubleKeys) {
        nextDoubleAt_ = RandomGenerator::range(settings_.doubleMin, settings_.doubleMax);
    } else {
        nextDoubleAt_ = INT_MAX;
    }
}

inline ImperfectionResult ImperfectionGenerator::processCharacter(QChar original) {
    ImperfectionResult result;
    result.character = original;
    
    charsTypedTotal_++;
    charsSinceLastTypo_++;
    charsSinceLastDouble_++;
    
    if (charsSinceLastTypo_ >= nextTypoAt_ && layout_.isLetter(original)) {
        result.character = layout_.getNeighborKey(original);
        charsSinceLastTypo_ = 0;
        scheduleNextTypo();
        
        if (settings_.enableAutoCorrection &&
            RandomGenerator::range(0, 99) < settings_.correctionProbability) {
            result.shouldCorrect = true;
        }
    }
    
    if (charsSinceLastDouble_ >= nextDoubleAt_ && !original.isSpace()) {
        result.shouldDouble = true;
        charsSinceLastDouble_ = 0;
        scheduleNextDouble();
    }
    
    return result;
}

// TextChunker
inline TextChunker::TextChunker(const QString& text)
    : text_(text)
    , currentIndex_(0)
{}

inline bool TextChunker::hasMore() const {
    return currentIndex_ < text_.length();
}

inline QString TextChunker::nextChunk() {
    if (!hasMore()) return QString();
    
    QChar ch = text_[currentIndex_];
    
    if (ch == '\n' || ch == '\t') {
        currentIndex_++;
        return QString(ch);
    }
    
    static const QString punct = "*-#`_[](){}<>!~+|\"'.,:;/?\\";
    if (punct.contains(ch)) {
        currentIndex_++;
        return QString(ch);
    }
    
    if (ch.isSpace()) {
        currentIndex_++;
        return QString(ch);
    }
    
    QString chunk;
    int limit = TypingConstants::MAX_CHUNK_LENGTH;
    while (currentIndex_ < text_.length() && limit--) {
        ch = text_[currentIndex_];
        if (ch == '\n' || ch == '\t') break;
        if (punct.contains(ch)) break;
        if (ch.isSpace()) break;
        chunk += ch;
        currentIndex_++;
    }
    
    return chunk;
}

inline int TextChunker::progressPercent() const {
    if (text_.length() == 0) return 100;
    return (currentIndex_ * 100) / text_.length();
}

// Platform Implementations
#ifdef Q_OS_LINUX
inline void LinuxKeyboardSimulator::typeCharacter(QChar c, int holdTimeMs) {
    if (c == '\n') {
        // Send Shift+Enter as a single ydotool command to avoid timing issues
        // Format: "42:1 28:1 28:0 42:0" means Shift down, Enter down, Enter up, Shift up
        QProcess::execute("ydotool", {"key", "42:1", "28:1", "28:0", "42:0"});
        QThread::msleep(holdTimeMs);
        return;
    } else if (c == '\t') {
        sendKey(43, holdTimeMs);
        return;
    }
    
    QProcess::execute("ydotool", {"type", "--", QString(c)});
    QThread::msleep(holdTimeMs);
}

inline void LinuxKeyboardSimulator::pressBackspace() {
    QProcess::execute("ydotool", {"key", "14:1"});
    QThread::msleep(TypingConstants::BACKSPACE_HOLD_MS);
    QProcess::execute("ydotool", {"key", "14:0"});
}

inline void LinuxKeyboardSimulator::sendKey(int keycode, int holdMs) {
    QProcess::execute("ydotool", {"key", QString::number(keycode) + ":1"});
    QThread::msleep(holdMs);
    QProcess::execute("ydotool", {"key", QString::number(keycode) + ":0"});
}

inline void LinuxKeyboardSimulator::releaseAllKeys() {
    QList<int> mods = {28, 42, 29, 56, 125, 97, 100, 102};
    for (int keycode : mods) {
        QProcess::execute("ydotool", {"key", QString::number(keycode) + ":0"});
    }
}

inline void LinuxMouseSimulator::moveRelative(int deltaX, int deltaY) {
    QProcess::execute("ydotool", {"mousemove", "--", 
                                   QString::number(deltaX), 
                                   QString::number(deltaY)});
}

inline void LinuxMouseSimulator::scroll(int amount) {
    // ydotool scroll: positive Y = scroll down, negative Y = scroll up
    // Each unit is roughly one "notch" of the scroll wheel
    QProcess::execute("ydotool", {"scroll", "--", "0", QString::number(amount)});
}
#endif

#ifdef Q_OS_MAC
inline void MacKeyboardSimulator::typeCharacter(QChar c, int holdTimeMs) {
    CGEventRef down = nullptr;
    CGEventRef up = nullptr;
    
    if (c == '\n') {
        // Send Shift+Enter to avoid triggering submit buttons
        CGEventRef shiftDown = CGEventCreateKeyboardEvent(nullptr, 56, true);  // Shift
        CGEventPost(kCGHIDEventTap, shiftDown);
        CFRelease(shiftDown);
        
        QThread::msleep(TypingConstants::MAC_SHIFT_DELAY_MS);
        
        // Send Enter while Shift is held
        down = CGEventCreateKeyboardEvent(nullptr, 0x24, true);  // Enter down
        up = CGEventCreateKeyboardEvent(nullptr, 0x24, false);   // Enter up
        
        CGEventSetFlags(down, kCGEventFlagMaskShift);  // Mark as Shift+Enter
        CGEventSetFlags(up, kCGEventFlagMaskShift);
        
        CGEventPost(kCGHIDEventTap, down);
        QThread::msleep(holdTimeMs);
        CGEventPost(kCGHIDEventTap, up);
        
        CFRelease(down);
        CFRelease(up);
        
        QThread::msleep(TypingConstants::MAC_SHIFT_DELAY_MS);
        
        // Release Shift
        CGEventRef shiftUp = CGEventCreateKeyboardEvent(nullptr, 56, false);
        CGEventPost(kCGHIDEventTap, shiftUp);
        CFRelease(shiftUp);
        return;
    } else {
        UniChar uc = c.unicode();
        down = CGEventCreateKeyboardEvent(nullptr, 0, true);
        up = CGEventCreateKeyboardEvent(nullptr, 0, false);
        CGEventKeyboardSetUnicodeString(down, 1, &uc);
        CGEventKeyboardSetUnicodeString(up, 1, &uc);
        
        CGEventPost(kCGHIDEventTap, down);
        QThread::msleep(holdTimeMs);
        CGEventPost(kCGHIDEventTap, up);
        
        CFRelease(down);
        CFRelease(up);
    }
}

inline void MacKeyboardSimulator::pressBackspace() {
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 51, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, 51, false);
    CGEventPost(kCGHIDEventTap, down);
    QThread::msleep(TypingConstants::BACKSPACE_HOLD_MS);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

inline void MacKeyboardSimulator::releaseAllKeys() {
    // macOS doesn't typically need this
}

inline void MacMouseSimulator::moveRelative(int deltaX, int deltaY) {
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

inline void MacMouseSimulator::scroll(int amount) {
    // Create a scroll wheel event
    // Positive amount = scroll down, negative = scroll up
    CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(nullptr, 
                                                           kCGScrollEventUnitLine,
                                                           1,  // Number of wheels (1 for vertical)
                                                           amount);
    CGEventPost(kCGHIDEventTap, scrollEvent);
    CFRelease(scrollEvent);
}
#endif

// TypingEngine
inline TypingEngine::TypingEngine(IKeyboardSimulator* simulator,
                           IMouseSimulator* mouseSimulator,
                           const TimingProfile& profile,
                           const DelayRange& delays,
                           const ImperfectionSettings& imperfections,
                           KeyboardLayoutType layoutType)
    : simulator_(simulator)
    , mouseSimulator_(mouseSimulator)
    , profile_(profile)
    , delays_(delays)
    , imperfections_(imperfections)
    , layout_(layoutType)
    , chunker_(nullptr)
    , dynamics_(nullptr)
    , imperfectionGen_(nullptr)
    , wordsSinceBreak_(0)
    , mouseMovementEnabled_(false)
    , charsSinceMouseMove_(0)
    , nextMouseMoveAt_(0)
    , skippedCharCount_(0)
    , skippedCharsPreview_()
{}

inline TypingEngine::~TypingEngine() {
    // unique_ptr handles cleanup automatically
}

inline void TypingEngine::setText(const QString& text) {
    chunker_ = std::make_unique<TextChunker>(text);
    dynamics_ = std::make_unique<TypingDynamics>(profile_, delays_);
    imperfectionGen_ = std::make_unique<ImperfectionGenerator>(imperfections_, layout_);
    wordsSinceBreak_ = 0;
    charsSinceMouseMove_ = 0;
    skippedCharCount_ = 0;
    skippedCharsPreview_.clear();
    scheduleNextMouseMove();
}

inline void TypingEngine::setMouseMovementEnabled(bool enabled) {
    mouseMovementEnabled_ = enabled;
    if (enabled) {
        scheduleNextMouseMove();
    }
}

inline void TypingEngine::scheduleNextMouseMove() {
    nextMouseMoveAt_ = RandomGenerator::range(TypingConstants::MIN_MOUSE_MOVE_INTERVAL_CHARS,
                                              TypingConstants::MAX_MOUSE_MOVE_INTERVAL_CHARS);
}

inline bool TypingEngine::shouldMoveMouse() {
    return mouseMovementEnabled_ && mouseSimulator_ && 
           charsSinceMouseMove_ >= nextMouseMoveAt_;
}

inline void TypingEngine::performMouseMovement() {
    if (!mouseSimulator_) return;
    
    // Generate small random movement
    int deltaX = RandomGenerator::range(-TypingConstants::MAX_MOUSE_PIXELS, 
                                        TypingConstants::MAX_MOUSE_PIXELS);
    int deltaY = RandomGenerator::range(-TypingConstants::MAX_MOUSE_PIXELS, 
                                        TypingConstants::MAX_MOUSE_PIXELS);
    
    // Avoid zero movement
    if (deltaX == 0 && deltaY == 0) {
        deltaX = RandomGenerator::range(TypingConstants::MIN_MOUSE_PIXELS, 
                                        TypingConstants::MAX_MOUSE_PIXELS);
    }
    
    mouseSimulator_->moveRelative(deltaX, deltaY);
    
    charsSinceMouseMove_ = 0;
    scheduleNextMouseMove();
}

inline bool TypingEngine::isTypeable(QChar c) const {
    // Basic ASCII is always safe
    if (c.unicode() < 128) return true;
    
    // Common extended ASCII might work
    // But ydotool/CGEvent may not handle all Unicode reliably
    return false;
}

inline void TypingEngine::recordSkippedChar(QChar c) {
    skippedCharCount_++;
    if (skippedCharsPreview_.length() < 20) {
        if (!skippedCharsPreview_.contains(c)) {
            if (!skippedCharsPreview_.isEmpty()) {
                skippedCharsPreview_ += ", ";
            }
            skippedCharsPreview_ += c;
        }
    }
}

inline bool TypingEngine::hasMoreToType() const {
    return chunker_ && chunker_->hasMore();
}

inline int TypingEngine::typeNextChunk() {
    if (!hasMoreToType()) return 0;
    
    // Check if we should move mouse before typing this chunk
    if (shouldMoveMouse()) {
        performMouseMovement();
        // Return a pause delay - typing stops during mouse movement
        return RandomGenerator::range(TypingConstants::MIN_MOUSE_PAUSE_MS,
                                      TypingConstants::MAX_MOUSE_PAUSE_MS);
    }
    
    QString chunk = chunker_->nextChunk();
    if (chunk.isEmpty()) return 0;
    
    for (QChar originalChar : chunk) {
        charsSinceMouseMove_++;
        
        // Check if character can be typed
        if (!isTypeable(originalChar)) {
            recordSkippedChar(originalChar);
            continue; // Skip this character
        }
        
        ImperfectionResult result = imperfectionGen_->processCharacter(originalChar);
        
        int holdTime = dynamics_->generateHoldTime(result.character);
        simulator_->typeCharacter(result.character, holdTime);
        
        if (result.shouldDouble) {
            int secondHold = dynamics_->generateHoldTime(result.character);
            QThread::msleep(RandomGenerator::range(TypingConstants::MIN_DOUBLE_KEY_DELAY_MS, 
                                                   TypingConstants::MAX_DOUBLE_KEY_DELAY_MS));
            simulator_->typeCharacter(result.character, secondHold);
        }
        
        if (result.shouldCorrect) {
            QThread::msleep(RandomGenerator::range(TypingConstants::MIN_CORRECTION_DELAY_MS, 
                                                   TypingConstants::MAX_CORRECTION_DELAY_MS));
            simulator_->pressBackspace();
            int corrHold = dynamics_->generateHoldTime(originalChar);
            QThread::msleep(RandomGenerator::range(TypingConstants::MIN_BACKSPACE_DELAY_MS, 
                                                   TypingConstants::MAX_BACKSPACE_DELAY_MS));
            simulator_->typeCharacter(originalChar, corrHold);
        }
        
        if (originalChar.isSpace()) wordsSinceBreak_++;
        
        dynamics_->updateState(originalChar);
    }
    
    QChar lastChar = chunk.back();
    bool isSentenceEnd = (lastChar == '.' || lastChar == '!' || lastChar == '?');
    bool isBurst = dynamics_->shouldBurst();
    bool isThinkingPause = dynamics_->shouldThinkingPause(wordsSinceBreak_);
    
    if (isThinkingPause) wordsSinceBreak_ = 0;
    
    return dynamics_->calculateDelay(lastChar, isSentenceEnd, isBurst, isThinkingPause);
}

inline int TypingEngine::progressPercent() const {
    return chunker_ ? chunker_->progressPercent() : 0;
}

inline void TypingEngine::reset() {
    if (dynamics_) dynamics_->reset();
    if (imperfectionGen_) imperfectionGen_->reset();
    wordsSinceBreak_ = 0;
}

#endif // TYPING_ENGINE_H