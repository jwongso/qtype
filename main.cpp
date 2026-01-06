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
#elif defined(Q_OS_MAC)
        simulator_ = new MacKeyboardSimulator();
#else
        simulator_ = nullptr;
#endif
        
        typingTimer_ = new QTimer(this);
        connect(typingTimer_, &QTimer::timeout, this, &AutoTyperWindow::typeNextChunk);
        
        countdownTimer_ = new QTimer(this);
        connect(countdownTimer_, &QTimer::timeout, this, &AutoTyperWindow::updateCountdown);
        
        watchdog_ = new QTimer(this);
        connect(watchdog_, &QTimer::timeout, this, &AutoTyperWindow::watchdogCheck);
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
        
        delete engine_;
        engine_ = new TypingEngine(simulator_, profile, delays, imperfections);
        engine_->setText(text);
        
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
        
        imperfLayout->addLayout(typoLayout);
        imperfLayout->addLayout(doubleLayout);
        imperfLayout->addLayout(autoLayout);
        
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
    QSpinBox *minDelaySpinBox_ = nullptr;
    QSpinBox *maxDelaySpinBox_ = nullptr;
    QComboBox *profileCombo_ = nullptr;
    
    QCheckBox *typoCheck_ = nullptr;
    QSpinBox *typoMinSpin_ = nullptr;
    QSpinBox *typoMaxSpin_ = nullptr;
    
    QCheckBox *doubleCheck_ = nullptr;
    QSpinBox *doubleMinSpin_ = nullptr;
    QSpinBox *doubleMaxSpin_ = nullptr;
    
    QCheckBox *autoCorrectCheck_ = nullptr;
    QSpinBox *autoCorrectProbSpin_ = nullptr;
    
    // Stats
    QLabel *statsLabel_ = nullptr;
    
    // Engine
    IKeyboardSimulator *simulator_ = nullptr;
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