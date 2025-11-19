#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QRandomGenerator>
#include <QKeyEvent>
#include <QSpinBox>
#include <QGroupBox>
#include <QComboBox>
#include <QDateTime>
#include <QThread>
#include <QCheckBox>
#include <cmath>
#include <climits>

#ifdef Q_OS_LINUX
#include <QProcess>
#endif

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

class AutoTyper : public QMainWindow {
    Q_OBJECT

public:
    AutoTyper(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();

        typingTimer = new QTimer(this);
        connect(typingTimer, &QTimer::timeout, this, &AutoTyper::typeNextChunk);

        countdownTimer = new QTimer(this);
        connect(countdownTimer, &QTimer::timeout, this, &AutoTyper::updateCountdown);

        watchdog = new QTimer(this);
        connect(watchdog, &QTimer::timeout, this, &AutoTyper::watchdogCheck);

        applyProfile(0);
    }

    ~AutoTyper() override {
        releaseAllKeys();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        releaseAllKeys();
        QMainWindow::closeEvent(event);
    }

private:
    // === UI ===
    QPlainTextEdit *textEdit = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *stopButton = nullptr;
    QLabel *statusLabel = nullptr;
    QSpinBox *minDelaySpinBox = nullptr;
    QSpinBox *maxDelaySpinBox = nullptr;
    QComboBox *profileCombo = nullptr;

    // Human Imperfection controls
    QCheckBox *typoCheck = nullptr;
    QSpinBox  *typoMinSpin = nullptr;
    QSpinBox  *typoMaxSpin = nullptr;

    QCheckBox *doubleCheck = nullptr;
    QSpinBox  *doubleMinSpin = nullptr;
    QSpinBox  *doubleMaxSpin = nullptr;

    QCheckBox *autoCorrectCheck = nullptr;
    QSpinBox  *autoCorrectProbSpin = nullptr;

    // === Runtime ===
    QTimer *typingTimer = nullptr;
    QTimer *countdownTimer = nullptr;
    QTimer *watchdog = nullptr;

    QString textToType;
    int currentIndex = 0;
    int countdownValue = 0;
    bool isTyping = false;
    qint64 lastActionTime = 0;

    // Timing / dynamics state
    QChar previousChar;
    double rhythmPhase = 0.0;
    int wordsSinceBreak = 0;

    int burstRemaining = 0;
    double fatigueFactor = 1.0;

    struct ProfileParams {
        double baseSpeedFactor = 1.0;
        double microStutterProb = 0.1;
        double idlePauseProb = 0.009;
        double burstProb = 0.14;
        int burstMin = 2;
        int burstMax = 6;

        double gammaShape = 2.0;
        double gammaScale = 1.0;
        double noiseLevel = 0.15;
    } profile;

    struct ImperfectionSettings {
        bool enableTypos = true;
        int typoMin = 300;
        int typoMax = 500;

        bool enableDoubleKeys = true;
        int doubleMin = 250;
        int doubleMax = 400;

        bool enableAutoCorrection = true;
        int correctionProbability = 15; // %
    } imperfections;

    // Counters for imperfections
    int charsTypedTotal = 0;
    int charsSinceLastTypo = 0;
    int charsSinceLastDouble = 0;
    int nextTypoAt = INT_MAX;
    int nextDoubleAt = INT_MAX;

    // ---------- RANDOM HELPERS ----------------------------------------------

    double gammaRandom(double shape, double scale) {
        if (shape < 1.0) {
            return gammaRandom(1.0 + shape, scale) *
                   std::pow(QRandomGenerator::global()->generateDouble(), 1.0 / shape);
        }

        const double d = shape - 1.0 / 3.0;
        const double c = 1.0 / std::sqrt(9.0 * d);

        while (true) {
            double x, v;
            do {
                x = normalRandom(0.0, 1.0);
                v = 1.0 + c * x;
            } while (v <= 0.0);

            v = v * v * v;
            const double u = QRandomGenerator::global()->generateDouble();

            if (u < 1.0 - 0.0331 * x * x * x * x) {
                return d * v * scale;
            }
            if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v))) {
                return d * v * scale;
            }
        }
    }

    double normalRandom(double mean, double stddev) {
        static bool hasSpare = false;
        static double spare;

        if (hasSpare) {
            hasSpare = false;
            return mean + stddev * spare;
        }

        double u, v, s;
        do {
            u = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
            v = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
            s = u * u + v * v;
        } while (s >= 1.0 || s == 0.0);

        s = std::sqrt(-2.0 * std::log(s) / s);
        spare = v * s;
        hasSpare = true;

        return mean + stddev * u * s;
    }

    double rhythmicVariation() {
        rhythmPhase += 0.03;
        const double rhythm = std::sin(rhythmPhase) * 0.5 + 0.5; // [0,1]
        return 0.85 + rhythm * 0.3;                              // [0.85,1.15]
    }

    double digraphFactor(QChar prev, QChar curr) {
        const QString digraph = QString(prev) + QString(curr);
        static const QStringList fast = {"th","he","in","er","an","re","on","at","en","nd"};

        if (fast.contains(digraph.toLower())) {
            return 0.75; // Faster
        }

        if ((prev == 'q' && curr == 'z') ||
            (prev == 'z' && curr == 'q') ||
            (prev == 'p' && curr == 'q')) {
            return 1.4; // Slower
        }

        static const QString leftHand  = "qwertasdfgzxcvb";
        static const QString rightHand = "yuiophjklnm";

        const bool bothLeft  = leftHand.contains(prev.toLower())  && leftHand.contains(curr.toLower());
        const bool bothRight = rightHand.contains(prev.toLower()) && rightHand.contains(curr.toLower());

        if (bothLeft || bothRight) {
            return 1.08;
        }

        return 1.0;
    }

    int randomRange(int a, int b) {
        if (a > b) std::swap(a, b);
        return QRandomGenerator::global()->bounded(a, b + 1);
    }

    // ---------- UI -----------------------------------------------------------

    void setupUI() {
        setWindowTitle("qtype - Advanced Typing Automation");
        setMinimumSize(780, 520);

        QWidget *central = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);

        auto *instructions = new QLabel(
            "Human-like typing automation\n"
            "• Non-uniform timing (gamma distribution)\n"
            "• Digraph-based timing variation\n"
            "• Natural rhythm + fatigue over long texts\n"
            "• Optional occasional typos/double-keys with possible self-correction"
        );
        instructions->setWordWrap(true);
        instructions->setStyleSheet("padding: 10px; background-color:#fff3cd; font-size: 11px;");
        mainLayout->addWidget(instructions);

        // Top layout: profile + delay + imperfections
        QHBoxLayout *topLayout = new QHBoxLayout();

        // Profile box
        QGroupBox *profileGroup = new QGroupBox("Timing Profile");
        QVBoxLayout *profileLayout = new QVBoxLayout(profileGroup);

        profileCombo = new QComboBox(this);
        profileCombo->addItem("Human (Advanced)");
        profileCombo->addItem("Fast Human");
        profileCombo->addItem("Slow & Tired");
        profileCombo->addItem("Professional");

        connect(profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &AutoTyper::applyProfile);

        profileLayout->addWidget(new QLabel("Behavior:"));
        profileLayout->addWidget(profileCombo);
        topLayout->addWidget(profileGroup);

        // Delay box
        QGroupBox *delayGroup = new QGroupBox("Base Delay Range");
        QHBoxLayout *delayLayout = new QHBoxLayout(delayGroup);

        QLabel *minLabel = new QLabel("Min:", this);
        minDelaySpinBox = new QSpinBox(this);
        minDelaySpinBox->setRange(5, 5000);
        minDelaySpinBox->setValue(120);
        minDelaySpinBox->setSuffix(" ms");

        QLabel *maxLabel = new QLabel("Max:", this);
        maxDelaySpinBox = new QSpinBox(this);
        maxDelaySpinBox->setRange(5, 5000);
        maxDelaySpinBox->setValue(2000);
        maxDelaySpinBox->setSuffix(" ms");

        connect(minDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v > maxDelaySpinBox->value())
                        maxDelaySpinBox->setValue(v);
                });
        connect(maxDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v < minDelaySpinBox->value())
                        minDelaySpinBox->setValue(v);
                });

        delayLayout->addWidget(minLabel);
        delayLayout->addWidget(minDelaySpinBox);
        delayLayout->addSpacing(10);
        delayLayout->addWidget(maxLabel);
        delayLayout->addWidget(maxDelaySpinBox);
        topLayout->addWidget(delayGroup);

        // Human imperfections box
        QGroupBox *imperfGroup = new QGroupBox("Human Imperfections");
        QVBoxLayout *imperfLayout = new QVBoxLayout(imperfGroup);

        // Typos row
        QHBoxLayout *typoLayout = new QHBoxLayout();
        typoCheck = new QCheckBox("Neighbor-key typos", this);
        typoCheck->setChecked(true);
        QLabel *typoFreqLabel = new QLabel("every", this);
        typoMinSpin = new QSpinBox(this);
        typoMinSpin->setRange(50, 10000);
        typoMinSpin->setValue(300);
        QLabel *typoDash = new QLabel("–", this);
        typoMaxSpin = new QSpinBox(this);
        typoMaxSpin->setRange(50, 10000);
        typoMaxSpin->setValue(500);
        QLabel *typoChars = new QLabel("chars", this);

        connect(typoMinSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v > typoMaxSpin->value())
                        typoMaxSpin->setValue(v);
                });
        connect(typoMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v < typoMinSpin->value())
                        typoMinSpin->setValue(v);
                });

        typoLayout->addWidget(typoCheck);
        typoLayout->addStretch();
        typoLayout->addWidget(typoFreqLabel);
        typoLayout->addWidget(typoMinSpin);
        typoLayout->addWidget(typoDash);
        typoLayout->addWidget(typoMaxSpin);
        typoLayout->addWidget(typoChars);

        // Double-key row
        QHBoxLayout *doubleLayout = new QHBoxLayout();
        doubleCheck = new QCheckBox("Double-key bounce", this);
        doubleCheck->setChecked(true);
        QLabel *doubleFreqLabel = new QLabel("every", this);
        doubleMinSpin = new QSpinBox(this);
        doubleMinSpin->setRange(50, 10000);
        doubleMinSpin->setValue(250);
        QLabel *doubleDash = new QLabel("–", this);
        doubleMaxSpin = new QSpinBox(this);
        doubleMaxSpin->setRange(50, 10000);
        doubleMaxSpin->setValue(400);
        QLabel *doubleChars = new QLabel("chars", this);

        connect(doubleMinSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v > doubleMaxSpin->value())
                        doubleMaxSpin->setValue(v);
                });
        connect(doubleMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) {
                    if (v < doubleMinSpin->value())
                        doubleMinSpin->setValue(v);
                });

        doubleLayout->addWidget(doubleCheck);
        doubleLayout->addStretch();
        doubleLayout->addWidget(doubleFreqLabel);
        doubleLayout->addWidget(doubleMinSpin);
        doubleLayout->addWidget(doubleDash);
        doubleLayout->addWidget(doubleMaxSpin);
        doubleLayout->addWidget(doubleChars);

        // Auto-correct row
        QHBoxLayout *autoLayout = new QHBoxLayout();
        autoCorrectCheck = new QCheckBox("Occasional self-correction", this);
        autoCorrectCheck->setChecked(true);
        QLabel *autoProbLabel = new QLabel("chance:", this);
        autoCorrectProbSpin = new QSpinBox(this);
        autoCorrectProbSpin->setRange(0, 100);
        autoCorrectProbSpin->setValue(15);
        QLabel *autoPercent = new QLabel("%", this);

        autoLayout->addWidget(autoCorrectCheck);
        autoLayout->addStretch();
        autoLayout->addWidget(autoProbLabel);
        autoLayout->addWidget(autoCorrectProbSpin);
        autoLayout->addWidget(autoPercent);

        imperfLayout->addLayout(typoLayout);
        imperfLayout->addLayout(doubleLayout);
        imperfLayout->addLayout(autoLayout);

        topLayout->addWidget(imperfGroup);

        mainLayout->addLayout(topLayout);

        // Text edit
        textEdit = new QPlainTextEdit(this);
        textEdit->setPlaceholderText("Paste your text here...");
        mainLayout->addWidget(textEdit);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        startButton = new QPushButton("Start (5s delay)");
        stopButton = new QPushButton("Stop");
        stopButton->setEnabled(false);

        connect(startButton, &QPushButton::clicked, this, &AutoTyper::startTyping);
        connect(stopButton, &QPushButton::clicked, this, &AutoTyper::stopTyping);

        buttonLayout->addWidget(startButton);
        buttonLayout->addWidget(stopButton);
        mainLayout->addLayout(buttonLayout);

        // Status
        statusLabel = new QLabel("Ready");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setStyleSheet("padding: 8px; font-size: 13px;");
        mainLayout->addWidget(statusLabel);

        setCentralWidget(central);
    }

    // ---------- PROFILE ------------------------------------------------------

    void applyProfile(int index) {
        switch (index) {
        case 1: // Fast human
            profile.baseSpeedFactor = 0.7;
            profile.microStutterProb = 0.06;
            profile.idlePauseProb = 0.004;
            profile.burstProb = 0.2;
            profile.burstMin = 3;
            profile.burstMax = 8;
            profile.gammaShape = 1.8;
            profile.gammaScale = 0.9;
            profile.noiseLevel = 0.12;
            break;
        case 2: // Slow & tired
            profile.baseSpeedFactor = 1.5;
            profile.microStutterProb = 0.15;
            profile.idlePauseProb = 0.025;
            profile.burstProb = 0.08;
            profile.burstMin = 2;
            profile.burstMax = 4;
            profile.gammaShape = 2.5;
            profile.gammaScale = 1.3;
            profile.noiseLevel = 0.22;
            break;
        case 3: // Professional
            profile.baseSpeedFactor = 0.75;
            profile.microStutterProb = 0.04;
            profile.idlePauseProb = 0.003;
            profile.burstProb = 0.25;
            profile.burstMin = 4;
            profile.burstMax = 10;
            profile.gammaShape = 1.6;
            profile.gammaScale = 0.85;
            profile.noiseLevel = 0.08;
            break;
        default: // Advanced human
            profile.baseSpeedFactor = 1.0;
            profile.microStutterProb = 0.1;
            profile.idlePauseProb = 0.009;
            profile.burstProb = 0.14;
            profile.burstMin = 2;
            profile.burstMax = 6;
            profile.gammaShape = 2.0;
            profile.gammaScale = 1.0;
            profile.noiseLevel = 0.15;
            break;
        }
    }

    void loadImperfectionSettingsFromUI() {
        imperfections.enableTypos = typoCheck->isChecked();
        imperfections.typoMin = typoMinSpin->value();
        imperfections.typoMax = typoMaxSpin->value();

        imperfections.enableDoubleKeys = doubleCheck->isChecked();
        imperfections.doubleMin = doubleMinSpin->value();
        imperfections.doubleMax = doubleMaxSpin->value();

        imperfections.enableAutoCorrection = autoCorrectCheck->isChecked();
        imperfections.correctionProbability = autoCorrectProbSpin->value();

        charsTypedTotal = 0;
        charsSinceLastTypo = 0;
        charsSinceLastDouble = 0;

        if (imperfections.enableTypos) {
            nextTypoAt = randomRange(imperfections.typoMin, imperfections.typoMax);
        } else {
            nextTypoAt = INT_MAX;
        }

        if (imperfections.enableDoubleKeys) {
            nextDoubleAt = randomRange(imperfections.doubleMin, imperfections.doubleMax);
        } else {
            nextDoubleAt = INT_MAX;
        }
    }

    // ---------- CONTROL FLOW -------------------------------------------------

    void startTyping() {
        textToType = textEdit->toPlainText();
        if (textToType.isEmpty()) {
            statusLabel->setText("Error: No text to type!");
            return;
        }

        currentIndex = 0;
        countdownValue = 5;
        isTyping = true;
        burstRemaining = 0;
        fatigueFactor = 1.0;
        previousChar = QChar();
        rhythmPhase = QRandomGenerator::global()->generateDouble() * 6.28318530718; // 2π
        wordsSinceBreak = 0;

        loadImperfectionSettingsFromUI();

        startButton->setEnabled(false);
        stopButton->setEnabled(true);

        statusLabel->setText("Get ready... 5");
        countdownTimer->start(1000);

        lastActionTime = QDateTime::currentMSecsSinceEpoch();
        watchdog->start(1000);
    }

    void updateCountdown() {
        countdownValue--;
        if (countdownValue > 0) {
            statusLabel->setText(QString("Get ready... %1").arg(countdownValue));
        } else {
            countdownTimer->stop();
            statusLabel->setText("Typing...");
            typeNextChunk();
        }
    }

    void watchdogCheck() {
        if (!isTyping) return;

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastActionTime > 10000) {
            statusLabel->setText("Watchdog triggered — Reset");
            releaseAllKeys();
            stopTyping();
        }
    }

    // ---------- BATCHING -----------------------------------------------------

    QString collectChunk() {
        if (currentIndex >= textToType.length())
            return {};

        QChar ch = textToType[currentIndex];

        if (ch == '\n' || ch == '\t') {
            currentIndex++;
            return QString(ch);
        }

        static const QString punct = "*-#`_[](){}<>!~+|\"'.,:;/?\\";
        if (punct.contains(ch)) {
            currentIndex++;
            return QString(ch);
        }

        if (ch.isSpace()) {
            currentIndex++;
            wordsSinceBreak++;
            return QString(ch);
        }

        QString chunk;
        int limit = 12;
        while (currentIndex < textToType.length() && limit--) {
            ch = textToType[currentIndex];
            if (ch == '\n' || ch == '\t') break;
            if (punct.contains(ch)) break;
            chunk += ch;
            currentIndex++;
        }
        return chunk;
    }

    // ---------- IMPERFECTIONS ------------------------------------------------

    QChar neighborKeyFor(QChar c) {
        const bool upper = c.isUpper();
        QChar lower = c.toLower();

        static const QString rows[] = {
            "qwertyuiop",
            "asdfghjkl",
            "zxcvbnm"
        };

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

        if (rowIndex == -1) {
            return c; // Not a letter we know
        }

        QList<QChar> candidates;

        auto addIfValid = [&](int r, int col) {
            if (r < 0 || r >= 3) return;
            const QString &row = rows[r];
            if (col < 0 || col >= row.size()) return;
            QChar ch = row[col];
            if (!candidates.contains(ch))
                candidates.append(ch);
        };

        // Same row neighbors
        addIfValid(rowIndex, colIndex - 1);
        addIfValid(rowIndex, colIndex + 1);
        // Vertical-ish neighbors
        addIfValid(rowIndex - 1, colIndex);
        addIfValid(rowIndex + 1, colIndex);
        addIfValid(rowIndex - 1, colIndex - 1);
        addIfValid(rowIndex - 1, colIndex + 1);
        addIfValid(rowIndex + 1, colIndex - 1);
        addIfValid(rowIndex + 1, colIndex + 1);

        if (candidates.isEmpty()) {
            return c;
        }

        const int idx = randomRange(0, candidates.size() - 1);
        QChar out = candidates[idx];
        if (upper)
            out = out.toUpper();
        return out;
    }

    QChar applyImperfectionsToChar(QChar original, bool &doDoubleKey, bool &doCorrection) {
        doDoubleKey = false;
        doCorrection = false;

        charsTypedTotal++;
        charsSinceLastTypo++;
        charsSinceLastDouble++;

        QChar out = original;

        // Occasional neighbor-key typo
        if (imperfections.enableTypos &&
            charsSinceLastTypo >= nextTypoAt &&
            original.isLetter()) {

            out = neighborKeyFor(original);
            charsSinceLastTypo = 0;
            nextTypoAt = randomRange(imperfections.typoMin, imperfections.typoMax);

            if (imperfections.enableAutoCorrection &&
                QRandomGenerator::global()->bounded(100) < imperfections.correctionProbability) {
                doCorrection = true;
            }
        }

        // Occasional double-key bounce
        if (imperfections.enableDoubleKeys &&
            charsSinceLastDouble >= nextDoubleAt &&
            !original.isSpace()) {

            doDoubleKey = true;
            charsSinceLastDouble = 0;
            nextDoubleAt = randomRange(imperfections.doubleMin, imperfections.doubleMax);
        }

        return out;
    }

    // ---------- TYPING STEP --------------------------------------------------

    void typeNextChunk() {
        if (!isTyping || currentIndex >= textToType.length()) {
            stopTyping();
            return;
        }

        lastActionTime = QDateTime::currentMSecsSinceEpoch();

        QString chunk = collectChunk();
        if (chunk.isEmpty()) {
            stopTyping();
            return;
        }

        simulateKeyPress(chunk);

        updateFatigue();

        const bool isSentenceEnd = chunk.endsWith('.') || chunk.endsWith('!') || chunk.endsWith('?');
        const bool burst = updateBurst();

        bool thinkingPause = false;
        if (wordsSinceBreak > randomRange(8, 15) &&
            QRandomGenerator::global()->generateDouble() < 0.3) {
            thinkingPause = true;
            wordsSinceBreak = 0;
        }

        int delay = computeAdvancedDelay(chunk, isSentenceEnd, burst, thinkingPause);
        typingTimer->start(delay);

        int pct = (currentIndex * 100) / textToType.length();
        statusLabel->setText(QString("Typing... %1%").arg(pct));

        previousChar = chunk.back();
    }

    void updateFatigue() {
        const double progress = double(currentIndex) / double(textToType.length());
        fatigueFactor = 1.0 + 0.25 * progress;
    }

    bool updateBurst() {
        if (burstRemaining > 0) {
            burstRemaining--;
            return true;
        }
        if (QRandomGenerator::global()->generateDouble() < profile.burstProb) {
            burstRemaining = randomRange(profile.burstMin, profile.burstMax);
            return true;
        }
        return false;
    }

    int computeAdvancedDelay(const QString &chunk,
                             bool isSentenceEnd,
                             bool isBurst,
                             bool thinkingPause) {
        const int minD = minDelaySpinBox->value();
        const int maxD = maxDelaySpinBox->value();

        double range = maxD - minD;
        double gammaValue = gammaRandom(profile.gammaShape, profile.gammaScale);
        double normalized = std::min(gammaValue / 6.0, 1.0);

        double delay = minD + range * normalized;

        delay *= rhythmicVariation();

        QChar ch = chunk.back();
        if (ch.isDigit()) delay *= 1.05;
        if (ch.isSpace()) delay *= 1.12;
        if (ch == '\n')   delay *= 1.5;
        if (ch == '.' || ch == '!' || ch == '?') delay *= 1.4;

        if (previousChar != QChar() && chunk.length() == 1) {
            delay *= digraphFactor(previousChar, ch);
        }

        if (isSentenceEnd)
            delay += gammaRandom(2.0, 150);

        if (thinkingPause)
            delay += gammaRandom(3.0, 800);

        if (QRandomGenerator::global()->generateDouble() < profile.microStutterProb)
            delay *= 1.3 + QRandomGenerator::global()->generateDouble() * 0.4;

        if (isBurst)
            delay *= 0.65;

        delay *= fatigueFactor;

        double noise = normalRandom(0.0, profile.noiseLevel);
        delay *= (1.0 + noise);

        if (delay < 15) delay = 15;
        if (delay > 8000) delay = 8000;

        return int(delay);
    }

    // ---------- KEY SIMULATION -----------------------------------------------

    void simulateKeyPress(const QString &chunk) {
        for (QChar ch : chunk) {
            bool doDouble = false;
            bool doCorrection = false;

            QChar effective = applyImperfectionsToChar(ch, doDouble, doCorrection);

            int holdTime = generateHoldTime(effective);
            sendKeyWithHold(effective, holdTime);

            if (doDouble) {
                int secondHold = generateHoldTime(effective);
                QThread::msleep(randomRange(10, 40));
                sendKeyWithHold(effective, secondHold);
            }

            if (doCorrection) {
                QThread::msleep(randomRange(60, 160)); // slight pause
                sendBackspace();
                int corrHold = generateHoldTime(ch);
                QThread::msleep(randomRange(40, 90));
                sendKeyWithHold(ch, corrHold);
            }
        }
    }

    int generateHoldTime(QChar ch) {
        double hold = gammaRandom(2.5, 20.0); // ~ 50–150 ms

        if (ch.isUpper()) {
            hold *= 1.2;
        }

        hold *= (0.9 + QRandomGenerator::global()->generateDouble() * 0.2);

        int result = int(hold);
        if (result < 40) result = 40;
        if (result > 180) result = 180;
        return result;
    }

    void sendKeyWithHold(QChar c, int holdMs) {
#ifdef Q_OS_LINUX
        if (c == '\n') {
            ydotoolKeyWithHold(28, holdMs); // Enter
            return;
        } else if (c == '\t') {
            ydotoolKeyWithHold(43, holdMs); // Tab
            return;
        }

        QProcess::execute("ydotool", {"type", QString(c)});
        QThread::msleep(holdMs);

#else // macOS
        CGEventRef down = nullptr;
        CGEventRef up = nullptr;

        if (c == '\n') {
            down = CGEventCreateKeyboardEvent(nullptr, 0x24, true);
            up   = CGEventCreateKeyboardEvent(nullptr, 0x24, false);
        } else {
            UniChar uc = c.unicode();
            down = CGEventCreateKeyboardEvent(nullptr, 0, true);
            up   = CGEventCreateKeyboardEvent(nullptr, 0, false);
            CGEventKeyboardSetUnicodeString(down, 1, &uc);
            CGEventKeyboardSetUnicodeString(up, 1, &uc);
        }

        CGEventPost(kCGHIDEventTap, down);
        QThread::msleep(holdMs);
        CGEventPost(kCGHIDEventTap, up);

        CFRelease(down);
        CFRelease(up);
#endif
    }

    void sendBackspace() {
#ifdef Q_OS_LINUX
        // Backspace keycode is usually 14
        QProcess::execute("ydotool", {"key", "14:1"});
        QThread::msleep(10);
        QProcess::execute("ydotool", {"key", "14:0"});
#else // macOS
        CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 51, true);  // delete
        CGEventRef up   = CGEventCreateKeyboardEvent(nullptr, 51, false);
        CGEventPost(kCGHIDEventTap, down);
        QThread::msleep(10);
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(down);
        CFRelease(up);
#endif
    }

#ifdef Q_OS_LINUX
    void ydotoolKeyWithHold(int code, int holdMs) {
        QProcess::execute("ydotool", {"key", QString::number(code) + ":1"});
        QThread::msleep(holdMs);
        QProcess::execute("ydotool", {"key", QString::number(code) + ":0"});
    }
#endif

    void releaseAllKeys() {
#ifdef Q_OS_LINUX
        QList<int> mods = {28, 42, 29, 56, 125, 97, 100, 102};
        for (int keycode : mods) {
            QProcess::execute("ydotool", {"key", QString::number(keycode) + ":0"});
        }
#endif
    }

    // ---------- STOP & EVENTS -----------------------------------------------

    void stopTyping() {
        typingTimer->stop();
        countdownTimer->stop();
        watchdog->stop();
        isTyping = false;
        releaseAllKeys();

        startButton->setEnabled(true);
        stopButton->setEnabled(false);

        const bool finished = (currentIndex >= textToType.length());
        statusLabel->setText(finished ? "Completed!" : "Stopped");
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape && isTyping) {
            stopTyping();
            return;
        }
        QMainWindow::keyPressEvent(e);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AutoTyper w;
    w.show();
    return app.exec();
}

#include "main.moc"
