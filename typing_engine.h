// ============================================================================
// typing_engine.h
// ============================================================================
#ifndef TYPING_ENGINE_H
#define TYPING_ENGINE_H

#include <QString>
#include <QChar>
#include <QRandomGenerator>
#include <cmath>

// Forward declarations
class IKeyboardSimulator;

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
    static QChar getNeighborKey(QChar c);
    static bool isLetter(QChar c);
    
private:
    static const QString rows[3];
};

// ============================================================================
// Typing Dynamics Calculator
// ============================================================================

class TypingDynamics {
public:
    TypingDynamics(const TimingProfile& profile, const DelayRange& delays);
    
    void reset();
    void updateState(QChar currentChar, bool isNewWord);
    
    int calculateDelay(QChar ch, bool isSentenceEnd, bool isBurst, bool isThinkingPause);
    int generateHoldTime(QChar ch);
    
    bool shouldBurst();
    bool shouldThinkingPause(int wordsSinceBreak);
    
    double digraphFactor(QChar prev, QChar curr);
    
private:
    TimingProfile profile_;
    DelayRange delays_;
    
    // State
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
    ImperfectionGenerator(const ImperfectionSettings& settings);
    
    void reset();
    ImperfectionResult processCharacter(QChar original);
    
private:
    ImperfectionSettings settings_;
    
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
#endif

#ifdef Q_OS_MAC
class MacKeyboardSimulator : public IKeyboardSimulator {
public:
    void typeCharacter(QChar c, int holdTimeMs) override;
    void pressBackspace() override;
    void releaseAllKeys() override;
};
#endif

// ============================================================================
// Main Typing Engine
// ============================================================================

class TypingEngine {
public:
    TypingEngine(IKeyboardSimulator* simulator,
                 const TimingProfile& profile,
                 const DelayRange& delays,
                 const ImperfectionSettings& imperfections);
    
    void setText(const QString& text);
    bool hasMoreToType() const;
    
    // Returns delay in ms until next character should be typed
    int typeNextChunk();
    
    int progressPercent() const;
    void reset();
    
private:
    IKeyboardSimulator* simulator_;
    TimingProfile profile_;
    DelayRange delays_;
    ImperfectionSettings imperfections_;
    
    TextChunker* chunker_;
    TypingDynamics* dynamics_;
    ImperfectionGenerator* imperfectionGen_;
    
    int wordsSinceBreak_;
};

#endif // TYPING_ENGINE_H

// ============================================================================
// typing_engine.cpp
// ============================================================================

#include <QThread>
#include <QProcess>
#include <climits>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

// ============================================================================
// TimingProfile
// ============================================================================

TimingProfile TimingProfile::humanAdvanced() {
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

TimingProfile TimingProfile::fastHuman() {
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

TimingProfile TimingProfile::slowTired() {
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

TimingProfile TimingProfile::professional() {
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

// ============================================================================
// RandomGenerator
// ============================================================================

double RandomGenerator::gamma(double shape, double scale) {
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

double RandomGenerator::normal(double mean, double stddev) {
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

int RandomGenerator::range(int min, int max) {
    if (min > max) std::swap(min, max);
    return QRandomGenerator::global()->bounded(min, max + 1);
}

double RandomGenerator::uniform() {
    return QRandomGenerator::global()->generateDouble();
}

// ============================================================================
// KeyboardLayout
// ============================================================================

const QString KeyboardLayout::rows[3] = {
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm"
};

QChar KeyboardLayout::getNeighborKey(QChar c) {
    bool upper = c.isUpper();
    QChar lower = c.toLower();
    
    int rowIndex = -1;
    int colIndex = -1;
    
    for (int r = 0; r < 3; ++r) {
        int idx = rows[r].indexOf(lower);
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
        const QString &row = rows[r];
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

bool KeyboardLayout::isLetter(QChar c) {
    return c.isLetter();
}

// ============================================================================
// TypingDynamics
// ============================================================================

TypingDynamics::TypingDynamics(const TimingProfile& profile, const DelayRange& delays)
    : profile_(profile)
    , delays_(delays)
    , previousChar_()
    , rhythmPhase_(RandomGenerator::uniform() * 6.28318530718)
    , fatigueFactor_(1.0)
    , burstRemaining_(0)
    , totalCharsTyped_(0)
{}

void TypingDynamics::reset() {
    previousChar_ = QChar();
    rhythmPhase_ = RandomGenerator::uniform() * 6.28318530718;
    fatigueFactor_ = 1.0;
    burstRemaining_ = 0;
    totalCharsTyped_ = 0;
}

void TypingDynamics::updateState(QChar currentChar, bool isNewWord) {
    previousChar_ = currentChar;
    totalCharsTyped_++;
    
    // Update fatigue based on total progress (approximation)
    if (totalCharsTyped_ % 50 == 0) {
        fatigueFactor_ = 1.0 + 0.25 * std::min(1.0, totalCharsTyped_ / 1000.0);
    }
}

bool TypingDynamics::shouldBurst() {
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

bool TypingDynamics::shouldThinkingPause(int wordsSinceBreak) {
    return wordsSinceBreak > RandomGenerator::range(8, 15) &&
           RandomGenerator::uniform() < 0.3;
}

double TypingDynamics::rhythmicVariation() {
    rhythmPhase_ += 0.03;
    double rhythm = std::sin(rhythmPhase_) * 0.5 + 0.5;
    return 0.85 + rhythm * 0.3;
}

double TypingDynamics::digraphFactor(QChar prev, QChar curr) {
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

int TypingDynamics::calculateDelay(QChar ch, bool isSentenceEnd, bool isBurst, bool isThinkingPause) {
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
    
    return qBound(15, int(delay), 8000);
}

int TypingDynamics::generateHoldTime(QChar ch) {
    double hold = RandomGenerator::gamma(2.5, 20.0);
    
    if (ch.isUpper()) {
        hold *= 1.2;
    }
    
    hold *= (0.9 + RandomGenerator::uniform() * 0.2);
    
    return qBound(40, int(hold), 180);
}

// ============================================================================
// ImperfectionGenerator
// ============================================================================

ImperfectionGenerator::ImperfectionGenerator(const ImperfectionSettings& settings)
    : settings_(settings)
    , charsTypedTotal_(0)
    , charsSinceLastTypo_(0)
    , charsSinceLastDouble_(0)
    , nextTypoAt_(INT_MAX)
    , nextDoubleAt_(INT_MAX)
{
    reset();
}

void ImperfectionGenerator::reset() {
    charsTypedTotal_ = 0;
    charsSinceLastTypo_ = 0;
    charsSinceLastDouble_ = 0;
    scheduleNextTypo();
    scheduleNextDouble();
}

void ImperfectionGenerator::scheduleNextTypo() {
    if (settings_.enableTypos) {
        nextTypoAt_ = RandomGenerator::range(settings_.typoMin, settings_.typoMax);
    } else {
        nextTypoAt_ = INT_MAX;
    }
}

void ImperfectionGenerator::scheduleNextDouble() {
    if (settings_.enableDoubleKeys) {
        nextDoubleAt_ = RandomGenerator::range(settings_.doubleMin, settings_.doubleMax);
    } else {
        nextDoubleAt_ = INT_MAX;
    }
}

ImperfectionResult ImperfectionGenerator::processCharacter(QChar original) {
    ImperfectionResult result;
    result.character = original;
    
    charsTypedTotal_++;
    charsSinceLastTypo_++;
    charsSinceLastDouble_++;
    
    // Typo check
    if (charsSinceLastTypo_ >= nextTypoAt_ && KeyboardLayout::isLetter(original)) {
        result.character = KeyboardLayout::getNeighborKey(original);
        charsSinceLastTypo_ = 0;
        scheduleNextTypo();
        
        if (settings_.enableAutoCorrection &&
            RandomGenerator::range(0, 99) < settings_.correctionProbability) {
            result.shouldCorrect = true;
        }
    }
    
    // Double key check
    if (charsSinceLastDouble_ >= nextDoubleAt_ && !original.isSpace()) {
        result.shouldDouble = true;
        charsSinceLastDouble_ = 0;
        scheduleNextDouble();
    }
    
    return result;
}

// ============================================================================
// TextChunker
// ============================================================================

TextChunker::TextChunker(const QString& text)
    : text_(text)
    , currentIndex_(0)
{}

bool TextChunker::hasMore() const {
    return currentIndex_ < text_.length();
}

QString TextChunker::nextChunk() {
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
    int limit = 12;
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

int TextChunker::progressPercent() const {
    if (text_.length() == 0) return 100;
    return (currentIndex_ * 100) / text_.length();
}

// ============================================================================
// Platform Implementations
// ============================================================================

#ifdef Q_OS_LINUX
void LinuxKeyboardSimulator::typeCharacter(QChar c, int holdTimeMs) {
    if (c == '\n') {
        sendKey(28, holdTimeMs);
        return;
    } else if (c == '\t') {
        sendKey(43, holdTimeMs);
        return;
    }
    
    // Use ydotool type for Unicode support
    QProcess::execute("ydotool", {"type", "--", QString(c)});
    QThread::msleep(holdTimeMs);
}

void LinuxKeyboardSimulator::pressBackspace() {
    QProcess::execute("ydotool", {"key", "14:1"});
    QThread::msleep(10);
    QProcess::execute("ydotool", {"key", "14:0"});
}

void LinuxKeyboardSimulator::sendKey(int keycode, int holdMs) {
    QProcess::execute("ydotool", {"key", QString::number(keycode) + ":1"});
    QThread::msleep(holdMs);
    QProcess::execute("ydotool", {"key", QString::number(keycode) + ":0"});
}

void LinuxKeyboardSimulator::releaseAllKeys() {
    QList<int> mods = {28, 42, 29, 56, 125, 97, 100, 102};
    for (int keycode : mods) {
        QProcess::execute("ydotool", {"key", QString::number(keycode) + ":0"});
    }
}
#endif

#ifdef Q_OS_MAC
void MacKeyboardSimulator::typeCharacter(QChar c, int holdTimeMs) {
    CGEventRef down = nullptr;
    CGEventRef up = nullptr;
    
    if (c == '\n') {
        down = CGEventCreateKeyboardEvent(nullptr, 0x24, true);
        up = CGEventCreateKeyboardEvent(nullptr, 0x24, false);
    } else {
        UniChar uc = c.unicode();
        down = CGEventCreateKeyboardEvent(nullptr, 0, true);
        up = CGEventCreateKeyboardEvent(nullptr, 0, false);
        CGEventKeyboardSetUnicodeString(down, 1, &uc);
        CGEventKeyboardSetUnicodeString(up, 1, &uc);
    }
    
    CGEventPost(kCGHIDEventTap, down);
    QThread::msleep(holdTimeMs);
    CGEventPost(kCGHIDEventTap, up);
    
    CFRelease(down);
    CFRelease(up);
}

void MacKeyboardSimulator::pressBackspace() {
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 51, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, 51, false);
    CGEventPost(kCGHIDEventTap, down);
    QThread::msleep(10);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

void MacKeyboardSimulator::releaseAllKeys() {
    // macOS doesn't typically need this
}
#endif

// ============================================================================
// TypingEngine
// ============================================================================

TypingEngine::TypingEngine(IKeyboardSimulator* simulator,
                           const TimingProfile& profile,
                           const DelayRange& delays,
                           const ImperfectionSettings& imperfections)
    : simulator_(simulator)
    , profile_(profile)
    , delays_(delays)
    , imperfections_(imperfections)
    , chunker_(nullptr)
    , dynamics_(nullptr)
    , imperfectionGen_(nullptr)
    , wordsSinceBreak_(0)
{}

void TypingEngine::setText(const QString& text) {
    delete chunker_;
    delete dynamics_;
    delete imperfectionGen_;
    
    chunker_ = new TextChunker(text);
    dynamics_ = new TypingDynamics(profile_, delays_);
    imperfectionGen_ = new ImperfectionGenerator(imperfections_);
    wordsSinceBreak_ = 0;
}

bool TypingEngine::hasMoreToType() const {
    return chunker_ && chunker_->hasMore();
}

int TypingEngine::typeNextChunk() {
    if (!hasMoreToType()) return 0;
    
    QString chunk = chunker_->nextChunk();
    if (chunk.isEmpty()) return 0;
    
    // Type each character in chunk with imperfections
    for (QChar originalChar : chunk) {
        ImperfectionResult result = imperfectionGen_->processCharacter(originalChar);
        
        int holdTime = dynamics_->generateHoldTime(result.character);
        simulator_->typeCharacter(result.character, holdTime);
        
        if (result.shouldDouble) {
            int secondHold = dynamics_->generateHoldTime(result.character);
            QThread::msleep(RandomGenerator::range(10, 40));
            simulator_->typeCharacter(result.character, secondHold);
        }
        
        if (result.shouldCorrect) {
            QThread::msleep(RandomGenerator::range(60, 160));
            simulator_->pressBackspace();
            int corrHold = dynamics_->generateHoldTime(originalChar);
            QThread::msleep(RandomGenerator::range(40, 90));
            simulator_->typeCharacter(originalChar, corrHold);
        }
        
        bool isNewWord = originalChar.isSpace();
        if (isNewWord) wordsSinceBreak_++;
        
        dynamics_->updateState(originalChar, isNewWord);
    }
    
    // Calculate delay for next chunk
    QChar lastChar = chunk.back();
    bool isSentenceEnd = (lastChar == '.' || lastChar == '!' || lastChar == '?');
    bool isBurst = dynamics_->shouldBurst();
    bool isThinkingPause = dynamics_->shouldThinkingPause(wordsSinceBreak_);
    
    if (isThinkingPause) wordsSinceBreak_ = 0;
    
    return dynamics_->calculateDelay(lastChar, isSentenceEnd, isBurst, isThinkingPause);
}

int TypingEngine::progressPercent() const {
    return chunker_ ? chunker_->progressPercent() : 0;
}

void TypingEngine::reset() {
    if (dynamics_) dynamics_->reset();
    if (imperfectionGen_) imperfectionGen_->reset();
    wordsSinceBreak_ = 0;
}