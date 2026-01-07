// main.cpp - Qt UI Layer
#include "typing_engine.h"
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QKeyEvent>
#include <QSpinBox>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QRegularExpression>

class AutoTyperWindow : public QMainWindow {
    Q_OBJECT

public:
    AutoTyperWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        
        // Create platform-specific simulator
#ifdef Q_OS_LINUX
        simulator_ = new LinuxKeyboardSimulator();
        mouseSimulator_ = new LinuxMouseSimulator();
#elif defined(Q_OS_MAC)
        simulator_ = new MacKeyboardSimulator();
        mouseSimulator_ = new MacMouseSimulator();
#else
        simulator_ = nullptr;
        mouseSimulator_ = nullptr;
#endif
        
        typingTimer_ = new QTimer(this);
        connect(typingTimer_, &QTimer::timeout, this, &AutoTyperWindow::typeNextChunk);
        
        countdownTimer_ = new QTimer(this);
        connect(countdownTimer_, &QTimer::timeout, this, &AutoTyperWindow::updateCountdown);
        
        watchdog_ = new QTimer(this);
        connect(watchdog_, &QTimer::timeout, this, &AutoTyperWindow::watchdogCheck);
        
        // Idle scroll timer - checks every second if we should scroll
        idleScrollTimer_ = new QTimer(this);
        connect(idleScrollTimer_, &QTimer::timeout, this, &AutoTyperWindow::checkIdleScroll);
        idleScrollTimer_->start(1000);  // Check every second
        
        lastActivityTime_ = QDateTime::currentMSecsSinceEpoch();
        
        // Install event filter to detect user activity
        qApp->installEventFilter(this);
    }
    
    ~AutoTyperWindow() {
        if (engine_) {
            delete engine_;
            engine_ = nullptr;
        }
        if (simulator_) {
            delete simulator_;
            simulator_ = nullptr;
        }
        if (mouseSimulator_) {
            delete mouseSimulator_;
            mouseSimulator_ = nullptr;
        }
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (simulator_) {
            simulator_->releaseAllKeys();
        }
        QMainWindow::closeEvent(event);
    }

private slots:
    void startTyping() {
        QString text = textEdit_->toPlainText();
        if (text.isEmpty()) {
            statusLabel_->setText("Error: No text to process!");
            return;
        }
        
        // Create engine with current settings
        TimingProfile profile = getSelectedProfile();
        
        DelayRange delays;
        delays.minMs = minDelaySpinBox_->value();
        delays.maxMs = maxDelaySpinBox_->value();
        
        ImperfectionSettings imperfections;
        imperfections.enableTypos = typoCheck_->isChecked();
        imperfections.typoMin = typoMinSpin_->value();
        imperfections.typoMax = typoMaxSpin_->value();
        imperfections.enableDoubleKeys = doubleCheck_->isChecked();
        imperfections.doubleMin = doubleMinSpin_->value();
        imperfections.doubleMax = doubleMaxSpin_->value();
        imperfections.enableAutoCorrection = autoCorrectCheck_->isChecked();
        imperfections.correctionProbability = autoCorrectProbSpin_->value();
        
        KeyboardLayoutType layout = getSelectedLayout();
        
        delete engine_;
        engine_ = new TypingEngine(simulator_, mouseSimulator_, profile, delays, imperfections, layout);
        engine_->setText(text);
        engine_->setMouseMovementEnabled(mouseMovementCheck_->isChecked());
        // Scroll is now idle-based, not typing-based
        
        // Hide warning from previous session
        warningLabel_->setVisible(false);
        
        countdownValue_ = 5;
        isTyping_ = true;
        
        startButton_->setEnabled(false);
        stopButton_->setEnabled(true);
        
        statusLabel_->setText("Get ready... 5");
        countdownTimer_->start(1000);
        
        lastActionTime_ = QDateTime::currentMSecsSinceEpoch();
        watchdog_->start(1000);
    }
    
    void stopTyping() {
        typingTimer_->stop();
        countdownTimer_->stop();
        watchdog_->stop();
        isTyping_ = false;
        
        if (simulator_) {
            simulator_->releaseAllKeys();
        }
        
        startButton_->setEnabled(true);
        stopButton_->setEnabled(false);
        
        bool finished = engine_ && !engine_->hasMoreToType();
        statusLabel_->setText(finished ? "Completed!" : "Stopped");
    }
    
    void updateCountdown() {
        countdownValue_--;
        if (countdownValue_ > 0) {
            statusLabel_->setText(QString("Get ready... %1").arg(countdownValue_));
        } else {
            countdownTimer_->stop();
            statusLabel_->setText("Processing...");
            typeNextChunk();
        }
    }
    
    void typeNextChunk() {
        if (!isTyping_ || !engine_ || !engine_->hasMoreToType()) {
            stopTyping();
            return;
        }
        
        lastActionTime_ = QDateTime::currentMSecsSinceEpoch();
        
        int delayMs = engine_->typeNextChunk();

        statusLabel_->setText(QString("Processing... %1%").arg(engine_->progressPercent()));
        
        // Check for skipped characters
        int skippedCount = engine_->getSkippedCharCount();
        if (skippedCount > 0) {
            QString preview = engine_->getSkippedCharsPreview();
            warningLabel_->setText(QString("⚠️ WARNING: %1 non-ASCII character(s) skipped: [%2] - These may cause detection!")
                                  .arg(skippedCount)
                                  .arg(preview));
            warningLabel_->setVisible(true);
        }
        
        if (engine_->hasMoreToType()) {
            typingTimer_->start(delayMs);
        } else {
            stopTyping();
        }
    }
    
    void watchdogCheck() {
        if (!isTyping_) return;
        
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastActionTime_ > 10000) {
            statusLabel_->setText("Watchdog triggered — Reset");
            if (simulator_) {
                simulator_->releaseAllKeys();
            }
            stopTyping();
        }
    }
    
    void checkIdleScroll() {
        if (!scrollCheck_->isChecked()) return;
        if (!mouseSimulator_) return;
        
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 idleTime = now - lastActivityTime_;
        
        // If idle for more than 30 seconds, perform a scroll
        if (idleTime >= 30000) {
            int amount = RandomGenerator::range(TypingConstants::MIN_SCROLL_AMOUNT,
                                                TypingConstants::MAX_SCROLL_AMOUNT);
            
            // 80% chance to scroll down, 20% to scroll up
            if (RandomGenerator::uniform() > TypingConstants::SCROLL_DOWN_PROBABILITY) {
                amount = -amount;
            }
            
            mouseSimulator_->scroll(amount);
            
            // Don't reset activity time - keep scrolling while idle
        }
    }
    
    bool eventFilter(QObject *obj, QEvent *event) override {
        // Track any keyboard or mouse activity to reset idle timer
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease ||
            event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::Wheel) {
            lastActivityTime_ = QDateTime::currentMSecsSinceEpoch();
        }
        
        return QMainWindow::eventFilter(obj, event);
    }
    
    void updateStats() {
        QString text = textEdit_->toPlainText();
        
        // Count characters
        int charCount = text.length();
        
        // Count words (split by whitespace, filter empty)
        QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        int wordCount = words.size();
        
        // Estimate tokens (rough approximation: ~1.3 tokens per word for English)
        // This is based on GPT tokenization averages
        int tokenEstimate = qRound(wordCount * 1.3);
        
        // Count lines
        int lineCount = text.split('\n').size();
        
        // Update stats label
        if (charCount == 0) {
            statsLabel_->setText("");
        } else {
            statsLabel_->setText(QString("Characters: %1  |  Words: %2  |  Lines: %3  |  Tokens (est.): ~%4")
                .arg(charCount)
                .arg(wordCount)
                .arg(lineCount)
                .arg(tokenEstimate));
        }
    }

private:
    void setupUI() {
        setWindowTitle("qtype - Text Input Practice & Analysis");
        setMinimumSize(780, 520);
        
        QWidget *central = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        
        auto *instructions = new QLabel(
            "Adaptive text input rehearsal system\n"
            "• Natural keystroke timing calibration\n"
            "• Context-aware cadence adjustment\n"
            "• Ergonomic pacing with fatigue modeling\n"
            "• Error pattern simulation for training"
        );
        instructions->setWordWrap(true);
        instructions->setStyleSheet("padding: 10px; background-color:#fff3cd; font-size: 11px;");
        mainLayout->addWidget(instructions);
        
        QHBoxLayout *topLayout = new QHBoxLayout();
        
        // Profile
        QGroupBox *profileGroup = new QGroupBox("Timing Profile");
        QVBoxLayout *profileLayout = new QVBoxLayout(profileGroup);
        
        profileCombo_ = new QComboBox(this);
        profileCombo_->addItem("Human (Advanced)");
        profileCombo_->addItem("Fast Human");
        profileCombo_->addItem("Slow & Tired");
        profileCombo_->addItem("Professional");
        
        profileLayout->addWidget(new QLabel("Behavior:"));
        profileLayout->addWidget(profileCombo_);
        
        profileLayout->addSpacing(10);
        
        layoutCombo_ = new QComboBox(this);
        layoutCombo_->addItem("🇺🇸 US QWERTY");
        layoutCombo_->addItem("🇬🇧 UK QWERTY");
        layoutCombo_->addItem("🇩🇪 German QWERTZ");
        layoutCombo_->addItem("🇫🇷 French AZERTY");
        layoutCombo_->setToolTip("Choose your keyboard layout for accurate typo simulation");
        
        profileLayout->addWidget(new QLabel("Keyboard:"));
        profileLayout->addWidget(layoutCombo_);
        topLayout->addWidget(profileGroup);
        
        // Delay
        QGroupBox *delayGroup = new QGroupBox("Base Delay Range");
        QHBoxLayout *delayLayout = new QHBoxLayout(delayGroup);
        
        minDelaySpinBox_ = new QSpinBox(this);
        minDelaySpinBox_->setRange(5, 5000);
        minDelaySpinBox_->setValue(120);
        minDelaySpinBox_->setSuffix(" ms");
        
        maxDelaySpinBox_ = new QSpinBox(this);
        maxDelaySpinBox_->setRange(5, 5000);
        maxDelaySpinBox_->setValue(2000);
        maxDelaySpinBox_->setSuffix(" ms");
        
        delayLayout->addWidget(new QLabel("Min:"));
        delayLayout->addWidget(minDelaySpinBox_);
        delayLayout->addSpacing(10);
        delayLayout->addWidget(new QLabel("Max:"));
        delayLayout->addWidget(maxDelaySpinBox_);
        topLayout->addWidget(delayGroup);
        
        // Imperfections
        QGroupBox *imperfGroup = new QGroupBox("Realism Simulation");
        QVBoxLayout *imperfLayout = new QVBoxLayout(imperfGroup);
        
        QHBoxLayout *typoLayout = new QHBoxLayout();
        typoCheck_ = new QCheckBox("Adjacent key error patterns", this);
        typoCheck_->setChecked(true);
        typoMinSpin_ = new QSpinBox(this);
        typoMinSpin_->setRange(50, 10000);
        typoMinSpin_->setValue(300);
        typoMaxSpin_ = new QSpinBox(this);
        typoMaxSpin_->setRange(50, 10000);
        typoMaxSpin_->setValue(500);
        
        typoLayout->addWidget(typoCheck_);
        typoLayout->addStretch();
        typoLayout->addWidget(new QLabel("every"));
        typoLayout->addWidget(typoMinSpin_);
        typoLayout->addWidget(new QLabel("–"));
        typoLayout->addWidget(typoMaxSpin_);
        typoLayout->addWidget(new QLabel("chars"));
        
        QHBoxLayout *doubleLayout = new QHBoxLayout();
        doubleCheck_ = new QCheckBox("Repeated keypress variation", this);
        doubleCheck_->setChecked(true);
        doubleMinSpin_ = new QSpinBox(this);
        doubleMinSpin_->setRange(50, 10000);
        doubleMinSpin_->setValue(250);
        doubleMaxSpin_ = new QSpinBox(this);
        doubleMaxSpin_->setRange(50, 10000);
        doubleMaxSpin_->setValue(400);
        
        doubleLayout->addWidget(doubleCheck_);
        doubleLayout->addStretch();
        doubleLayout->addWidget(new QLabel("every"));
        doubleLayout->addWidget(doubleMinSpin_);
        doubleLayout->addWidget(new QLabel("–"));
        doubleLayout->addWidget(doubleMaxSpin_);
        doubleLayout->addWidget(new QLabel("chars"));
        
        QHBoxLayout *autoLayout = new QHBoxLayout();
        autoCorrectCheck_ = new QCheckBox("Backspace correction training", this);
        autoCorrectCheck_->setChecked(true);
        autoCorrectProbSpin_ = new QSpinBox(this);
        autoCorrectProbSpin_->setRange(0, 100);
        autoCorrectProbSpin_->setValue(15);
        
        autoLayout->addWidget(autoCorrectCheck_);
        autoLayout->addStretch();
        autoLayout->addWidget(new QLabel("chance:"));
        autoLayout->addWidget(autoCorrectProbSpin_);
        autoLayout->addWidget(new QLabel("%"));
        
        QHBoxLayout *mouseLayout = new QHBoxLayout();
        mouseMovementCheck_ = new QCheckBox("Subtle mouse movement simulation", this);
        mouseMovementCheck_->setChecked(false);
        mouseMovementCheck_->setToolTip("Occasionally moves mouse by a few pixels during typing pauses");
        mouseLayout->addWidget(mouseMovementCheck_);
        
        QHBoxLayout *scrollLayout = new QHBoxLayout();
        scrollCheck_ = new QCheckBox("Idle scrolling (screensaver-like)", this);
        scrollCheck_->setChecked(false);
        scrollCheck_->setToolTip("Scrolls automatically after 30 seconds of keyboard/mouse inactivity");
        scrollLayout->addWidget(scrollCheck_);
        
        imperfLayout->addLayout(typoLayout);
        imperfLayout->addLayout(doubleLayout);
        imperfLayout->addLayout(autoLayout);
        imperfLayout->addLayout(mouseLayout);
        imperfLayout->addLayout(scrollLayout);
        
        topLayout->addWidget(imperfGroup);
        
        mainLayout->addLayout(topLayout);
        
        // Text edit
        textEdit_ = new QPlainTextEdit(this);
        textEdit_->setPlaceholderText("Paste your text here...");
        
        // Connect to update stats when text changes
        connect(textEdit_, &QPlainTextEdit::textChanged, this, &AutoTyperWindow::updateStats);
        
        mainLayout->addWidget(textEdit_);
        
        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        startButton_ = new QPushButton("Start (5s delay)");
        stopButton_ = new QPushButton("Stop");
        stopButton_->setEnabled(false);
        
        connect(startButton_, &QPushButton::clicked, this, &AutoTyperWindow::startTyping);
        connect(stopButton_, &QPushButton::clicked, this, &AutoTyperWindow::stopTyping);
        
        buttonLayout->addWidget(startButton_);
        buttonLayout->addWidget(stopButton_);
        mainLayout->addLayout(buttonLayout);
        
        // Status
        statusLabel_ = new QLabel("Ready");
        statusLabel_->setAlignment(Qt::AlignCenter);
        statusLabel_->setStyleSheet("padding: 8px; font-size: 13px;");
        mainLayout->addWidget(statusLabel_);
        
        // Warning label for non-ASCII characters
        warningLabel_ = new QLabel("");
        warningLabel_->setAlignment(Qt::AlignCenter);
        warningLabel_->setStyleSheet("padding: 8px; font-size: 12px; color: #d9534f; background-color: #f2dede; border: 1px solid #ebccd1; border-radius: 4px; font-weight: bold;");
        warningLabel_->setWordWrap(true);
        warningLabel_->setVisible(false);
        mainLayout->addWidget(warningLabel_);
        
        // Stats bar at bottom
        statsLabel_ = new QLabel("");
        statsLabel_->setAlignment(Qt::AlignRight);
        statsLabel_->setStyleSheet("padding: 5px; font-size: 11px; color: #666; background-color: #f5f5f5;");
        mainLayout->addWidget(statsLabel_);
        
        setCentralWidget(central);
    }
    
    TimingProfile getSelectedProfile() {
        switch (profileCombo_->currentIndex()) {
            case 1: return TimingProfile::fastHuman();
            case 2: return TimingProfile::slowTired();
            case 3: return TimingProfile::professional();
            default: return TimingProfile::humanAdvanced();
        }
    }
    
    KeyboardLayoutType getSelectedLayout() {
        switch (layoutCombo_->currentIndex()) {
            case 1: return KeyboardLayoutType::UK_QWERTY;
            case 2: return KeyboardLayoutType::GERMAN_QWERTZ;
            case 3: return KeyboardLayoutType::FRENCH_AZERTY;
            default: return KeyboardLayoutType::US_QWERTY;
        }
    }
    
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape && isTyping_) {
            stopTyping();
            return;
        }
        QMainWindow::keyPressEvent(e);
    }

private:
    // UI widgets
    QPlainTextEdit *textEdit_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *warningLabel_ = nullptr;
    QSpinBox *minDelaySpinBox_ = nullptr;
    QSpinBox *maxDelaySpinBox_ = nullptr;
    QComboBox *profileCombo_ = nullptr;
    QComboBox *layoutCombo_ = nullptr;
    
    QCheckBox *typoCheck_ = nullptr;
    QSpinBox *typoMinSpin_ = nullptr;
    QSpinBox *typoMaxSpin_ = nullptr;
    
    QCheckBox *doubleCheck_ = nullptr;
    QSpinBox *doubleMinSpin_ = nullptr;
    QSpinBox *doubleMaxSpin_ = nullptr;
    
    QCheckBox *autoCorrectCheck_ = nullptr;
    QSpinBox *autoCorrectProbSpin_ = nullptr;
    
    QCheckBox *mouseMovementCheck_ = nullptr;
    QCheckBox *scrollCheck_ = nullptr;
    
    // Idle scroll tracking
    QTimer *idleScrollTimer_ = nullptr;
    qint64 lastActivityTime_ = 0;
    
    // Stats
    QLabel *statsLabel_ = nullptr;
    
    // Engine
    IKeyboardSimulator *simulator_ = nullptr;
    IMouseSimulator *mouseSimulator_ = nullptr;
    TypingEngine *engine_ = nullptr;
    
    // Timers
    QTimer *typingTimer_ = nullptr;
    QTimer *countdownTimer_ = nullptr;
    QTimer *watchdog_ = nullptr;
    
    // State
    int countdownValue_ = 0;
    bool isTyping_ = false;
    qint64 lastActionTime_ = 0;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AutoTyperWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"