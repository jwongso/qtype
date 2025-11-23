// tests.cpp - Google Test Unit Tests
#include "typing_engine.h"
#include <gtest/gtest.h>
#include <QCoreApplication>

// Mock keyboard simulator for testing
class MockKeyboardSimulator : public IKeyboardSimulator {
public:
    struct KeyPress {
        QChar character;
        int holdTimeMs;
    };
    
    std::vector<KeyPress> keyPresses;
    int backspaceCount = 0;
    int releaseCount = 0;
    
    void typeCharacter(QChar c, int holdTimeMs) override {
        keyPresses.push_back({c, holdTimeMs});
    }
    
    void pressBackspace() override {
        backspaceCount++;
    }
    
    void releaseAllKeys() override {
        releaseCount++;
    }
    
    void reset() {
        keyPresses.clear();
        backspaceCount = 0;
        releaseCount = 0;
    }
    
    QString getTypedText() const {
        QString result;
        for (const auto& kp : keyPresses) {
            result += kp.character;
        }
        return result;
    }
};

// ============================================================================
// RandomGenerator Tests
// ============================================================================

TEST(RandomGeneratorTest, RangeProducesValidValues) {
    for (int i = 0; i < 100; i++) {
        int val = RandomGenerator::range(10, 20);
        EXPECT_GE(val, 10);
        EXPECT_LE(val, 20);
    }
}

TEST(RandomGeneratorTest, UniformInRange) {
    for (int i = 0; i < 100; i++) {
        double val = RandomGenerator::uniform();
        EXPECT_GE(val, 0.0);
        EXPECT_LT(val, 1.0);
    }
}

TEST(RandomGeneratorTest, GammaPositive) {
    for (int i = 0; i < 100; i++) {
        double val = RandomGenerator::gamma(2.0, 1.0);
        EXPECT_GT(val, 0.0);
    }
}

TEST(RandomGeneratorTest, NormalDistribution) {
    double sum = 0;
    int count = 1000;
    for (int i = 0; i < count; i++) {
        sum += RandomGenerator::normal(10.0, 2.0);
    }
    double mean = sum / count;
    EXPECT_NEAR(mean, 10.0, 1.0); // Should be close to expected mean
}

// ============================================================================
// KeyboardLayout Tests
// ============================================================================

TEST(KeyboardLayoutTest, NeighborKeyForLetter) {
    QChar neighbor = KeyboardLayout::getNeighborKey('a');
    EXPECT_TRUE(neighbor.isLetter());
    EXPECT_NE(neighbor, 'a'); // Should be different (most of the time)
}

TEST(KeyboardLayoutTest, PreservesCase) {
    QChar lower = KeyboardLayout::getNeighborKey('a');
    EXPECT_TRUE(lower.isLower());
    
    QChar upper = KeyboardLayout::getNeighborKey('A');
    EXPECT_TRUE(upper.isUpper());
}

TEST(KeyboardLayoutTest, NonLetterUnchanged) {
    EXPECT_EQ(KeyboardLayout::getNeighborKey('1'), '1');
    EXPECT_EQ(KeyboardLayout::getNeighborKey(' '), ' ');
    EXPECT_EQ(KeyboardLayout::getNeighborKey('!'), '!');
}

// ============================================================================
// TextChunker Tests
// ============================================================================

TEST(TextChunkerTest, EmptyText) {
    TextChunker chunker("");
    EXPECT_FALSE(chunker.hasMore());
    EXPECT_EQ(chunker.progressPercent(), 100);
}

TEST(TextChunkerTest, SimpleWord) {
    TextChunker chunker("hello");
    EXPECT_TRUE(chunker.hasMore());
    
    QString chunk = chunker.nextChunk();
    EXPECT_EQ(chunk, "hello");
    EXPECT_FALSE(chunker.hasMore());
}

TEST(TextChunkerTest, MultipleWords) {
    TextChunker chunker("hello world");
    
    QString chunk1 = chunker.nextChunk();
    EXPECT_EQ(chunk1, "hello");
    
    QString chunk2 = chunker.nextChunk();
    EXPECT_EQ(chunk2, " ");
    
    QString chunk3 = chunker.nextChunk();
    EXPECT_EQ(chunk3, "world");
    
    EXPECT_FALSE(chunker.hasMore());
}

TEST(TextChunkerTest, SpecialCharacters) {
    TextChunker chunker("hello!\nworld");
    
    EXPECT_EQ(chunker.nextChunk(), "hello");
    EXPECT_EQ(chunker.nextChunk(), "!");
    EXPECT_EQ(chunker.nextChunk(), "\n");
    EXPECT_EQ(chunker.nextChunk(), "world");
}

TEST(TextChunkerTest, ProgressTracking) {
    TextChunker chunker("1234567890");
    EXPECT_EQ(chunker.progressPercent(), 0);
    
    chunker.nextChunk(); // Gets "1234567890"
    EXPECT_EQ(chunker.progressPercent(), 100);
}

TEST(TextChunkerTest, UnicodeCharacters) {
    TextChunker chunker("Hello — World");
    
    QString chunk1 = chunker.nextChunk();
    EXPECT_EQ(chunk1, "Hello");
    
    QString chunk2 = chunker.nextChunk();
    EXPECT_EQ(chunk2, " ");
    
    QString chunk3 = chunker.nextChunk();
    EXPECT_EQ(chunk3, "—"); // Em dash
    
    QString chunk4 = chunker.nextChunk();
    EXPECT_EQ(chunk4, " ");
    
    QString chunk5 = chunker.nextChunk();
    EXPECT_EQ(chunk5, "World");
}

// ============================================================================
// TypingDynamics Tests
// ============================================================================

TEST(TypingDynamicsTest, DelayInRange) {
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{100, 200};
    TypingDynamics dynamics(profile, delays);
    
    for (int i = 0; i < 50; i++) {
        int delay = dynamics.calculateDelay('a', false, false, false);
        EXPECT_GE(delay, 15); // Minimum
        EXPECT_LE(delay, 8000); // Maximum
    }
}

TEST(TypingDynamicsTest, HoldTimeInRange) {
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{100, 200};
    TypingDynamics dynamics(profile, delays);
    
    for (int i = 0; i < 50; i++) {
        int hold = dynamics.generateHoldTime('a');
        EXPECT_GE(hold, 40);
        EXPECT_LE(hold, 180);
    }
}

TEST(TypingDynamicsTest, UpperCaseLongerHold) {
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{100, 200};
    TypingDynamics dynamics(profile, delays);
    
    double lowerSum = 0, upperSum = 0;
    int samples = 100;
    
    for (int i = 0; i < samples; i++) {
        lowerSum += dynamics.generateHoldTime('a');
        upperSum += dynamics.generateHoldTime('A');
    }
    
    EXPECT_GT(upperSum / samples, lowerSum / samples);
}

TEST(TypingDynamicsTest, DigraphFactorCommonPairs) {
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{100, 200};
    TypingDynamics dynamics(profile, delays);
    
    // Common digraphs should be faster
    double factor = dynamics.digraphFactor('t', 'h');
    EXPECT_LT(factor, 1.0);
}

// ============================================================================
// ImperfectionGenerator Tests
// ============================================================================

TEST(ImperfectionGeneratorTest, NoImperfectionsWhenDisabled) {
    ImperfectionSettings settings;
    settings.enableTypos = false;
    settings.enableDoubleKeys = false;
    
    ImperfectionGenerator gen(settings);
    
    for (int i = 0; i < 1000; i++) {
        ImperfectionResult result = gen.processCharacter('a');
        EXPECT_EQ(result.character, 'a');
        EXPECT_FALSE(result.shouldDouble);
        EXPECT_FALSE(result.shouldCorrect);
    }
}

TEST(ImperfectionGeneratorTest, TyposEnabled) {
    ImperfectionSettings settings;
    settings.enableTypos = true;
    settings.typoMin = 5;
    settings.typoMax = 10;
    settings.enableAutoCorrection = false;
    
    ImperfectionGenerator gen(settings);
    
    bool foundTypo = false;
    for (int i = 0; i < 100; i++) {
        ImperfectionResult result = gen.processCharacter('a');
        if (result.character != 'a') {
            foundTypo = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundTypo);
}

TEST(ImperfectionGeneratorTest, DoubleKeysEnabled) {
    ImperfectionSettings settings;
    settings.enableTypos = false;
    settings.enableDoubleKeys = true;
    settings.doubleMin = 5;
    settings.doubleMax = 10;
    
    ImperfectionGenerator gen(settings);
    
    bool foundDouble = false;
    for (int i = 0; i < 100; i++) {
        ImperfectionResult result = gen.processCharacter('a');
        if (result.shouldDouble) {
            foundDouble = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundDouble);
}

// ============================================================================
// TypingEngine Integration Tests
// ============================================================================

TEST(TypingEngineTest, TypesSimpleText) {
    MockKeyboardSimulator mock;
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{50, 100};
    ImperfectionSettings imperfections;
    imperfections.enableTypos = false;
    imperfections.enableDoubleKeys = false;
    
    TypingEngine engine(&mock, profile, delays, imperfections);
    engine.setText("hi");
    
    EXPECT_TRUE(engine.hasMoreToType());
    
    engine.typeNextChunk();
    engine.typeNextChunk();
    
    EXPECT_FALSE(engine.hasMoreToType());
    EXPECT_EQ(mock.getTypedText(), "hi");
}

TEST(TypingEngineTest, TypesMultipleWords) {
    MockKeyboardSimulator mock;
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{50, 100};
    ImperfectionSettings imperfections;
    imperfections.enableTypos = false;
    imperfections.enableDoubleKeys = false;
    
    TypingEngine engine(&mock, profile, delays, imperfections);
    engine.setText("hello world");
    
    while (engine.hasMoreToType()) {
        engine.typeNextChunk();
    }
    
    EXPECT_EQ(mock.getTypedText(), "hello world");
}

TEST(TypingEngineTest, HandlesUnicode) {
    MockKeyboardSimulator mock;
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{50, 100};
    ImperfectionSettings imperfections;
    imperfections.enableTypos = false;
    imperfections.enableDoubleKeys = false;
    
    TypingEngine engine(&mock, profile, delays, imperfections);
    engine.setText("café — naïve");
    
    while (engine.hasMoreToType()) {
        engine.typeNextChunk();
    }
    
    QString typed = mock.getTypedText();
    EXPECT_TRUE(typed.contains("café"));
    EXPECT_TRUE(typed.contains("—"));
    EXPECT_TRUE(typed.contains("naïve"));
}

TEST(TypingEngineTest, ProgressTracking) {
    MockKeyboardSimulator mock;
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{50, 100};
    ImperfectionSettings imperfections;
    imperfections.enableTypos = false;
    imperfections.enableDoubleKeys = false;
    
    TypingEngine engine(&mock, profile, delays, imperfections);
    engine.setText("test");
    
    EXPECT_EQ(engine.progressPercent(), 0);
    
    engine.typeNextChunk();
    EXPECT_GT(engine.progressPercent(), 0);
    EXPECT_LT(engine.progressPercent(), 100);
    
    while (engine.hasMoreToType()) {
        engine.typeNextChunk();
    }
    
    EXPECT_EQ(engine.progressPercent(), 100);
}

TEST(TypingEngineTest, TyposGenerateCorrections) {
    MockKeyboardSimulator mock;
    TimingProfile profile = TimingProfile::humanAdvanced();
    DelayRange delays{50, 100};
    ImperfectionSettings imperfections;
    imperfections.enableTypos = true;
    imperfections.typoMin = 3;
    imperfections.typoMax = 5;
    imperfections.enableAutoCorrection = true;
    imperfections.correctionProbability = 100; // Always correct
    imperfections.enableDoubleKeys = false;
    
    TypingEngine engine(&mock, profile, delays, imperfections);
    engine.setText("abcdefghij");
    
    while (engine.hasMoreToType()) {
        engine.typeNextChunk();
    }
    
    // Should have backspaces from corrections
    EXPECT_GT(mock.backspaceCount, 0);
}

// ============================================================================
// Profile Tests
// ============================================================================

TEST(ProfileTest, AllProfilesValid) {
    auto humanAdv = TimingProfile::humanAdvanced();
    EXPECT_GT(humanAdv.baseSpeedFactor, 0.0);
    EXPECT_GT(humanAdv.gammaShape, 0.0);
    
    auto fast = TimingProfile::fastHuman();
    EXPECT_LT(fast.baseSpeedFactor, humanAdv.baseSpeedFactor);
    
    auto slow = TimingProfile::slowTired();
    EXPECT_GT(slow.baseSpeedFactor, humanAdv.baseSpeedFactor);
    
    auto pro = TimingProfile::professional();
    EXPECT_GT(pro.burstProb, humanAdv.burstProb);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    // Qt needs to be initialized for QString operations
    QCoreApplication app(argc, argv);
    
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
