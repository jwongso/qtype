// qtype_server.cpp - Linux Server with Qt WebSocket
// Compile: Add to CMakeLists.txt or use qmake with QtWebSockets

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QNetworkInterface>

class QTypeServer : public QMainWindow {
    Q_OBJECT

public:
    QTypeServer(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        startServer();
    }
    
    ~QTypeServer() {
        // Properly disconnect all clients before closing server
        for (QWebSocket *client : clients_) {
            if (client) {
                client->close(QWebSocketProtocol::CloseCodeNormal, "Server shutting down");
                client->deleteLater();
            }
        }
        clients_.clear();

        if (wsServer_) {
            wsServer_->close();
            wsServer_->deleteLater();
        }
    }

private slots:
    void onNewConnection() {
        QWebSocket *client = wsServer_->nextPendingConnection();
        
        connect(client, &QWebSocket::textMessageReceived, this, &QTypeServer::onMessageReceived);
        connect(client, &QWebSocket::disconnected, this, &QTypeServer::onClientDisconnected);
        
        clients_.append(client);
        updateClientList();
        
        QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        statusLabel_->setText(QString("Client connected: %1").arg(clientInfo));
        
        // Send welcome message
        QJsonObject welcome;
        welcome["type"] = "welcome";
        welcome["message"] = "Connected to qtype server";
        client->sendTextMessage(QJsonDocument(welcome).toJson(QJsonDocument::Compact));
    }
    
    void onClientDisconnected() {
        QWebSocket *client = qobject_cast<QWebSocket*>(sender());
        if (client) {
            QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
            clients_.removeAll(client);
            client->deleteLater();
            updateClientList();
            statusLabel_->setText(QString("Client disconnected: %1").arg(clientInfo));
        }
    }
    
    void onMessageReceived(const QString &message) {
        QWebSocket *client = qobject_cast<QWebSocket*>(sender());
        
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        QJsonObject obj = doc.object();
        
        QString type = obj["type"].toString();
        
        if (type == "status") {
            // Client sending status update
            QString status = obj["status"].toString();
            int progress = obj["progress"].toInt();
            
            QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
            statusLabel_->setText(QString("%1 - %2 (%3%)").arg(clientInfo).arg(status).arg(progress));
        }
        else if (type == "ready") {
            statusLabel_->setText("Client is ready");
        }
        else if (type == "completed") {
            statusLabel_->setText("Typing completed on client");
            startButton_->setEnabled(true);
            stopButton_->setEnabled(false);
        }
    }
    
    void startTyping() {
        if (clients_.isEmpty()) {
            statusLabel_->setText("Error: No clients connected!");
            return;
        }
        
        QString text = textEdit_->toPlainText();
        if (text.isEmpty()) {
            statusLabel_->setText("Error: No text to type!");
            return;
        }
        
        // Get settings
        QJsonObject settings;
        settings["profile"] = profileCombo_->currentIndex();
        settings["minDelay"] = minDelaySpinBox_->value();
        settings["maxDelay"] = maxDelaySpinBox_->value();
        settings["enableTypos"] = typoCheck_->isChecked();
        settings["typoMin"] = typoMinSpin_->value();
        settings["typoMax"] = typoMaxSpin_->value();
        settings["enableDoubleKeys"] = doubleCheck_->isChecked();
        settings["doubleMin"] = doubleMinSpin_->value();
        settings["doubleMax"] = doubleMaxSpin_->value();
        settings["enableAutoCorrection"] = autoCorrectCheck_->isChecked();
        settings["correctionProbability"] = autoCorrectProbSpin_->value();
        
        // Create command
        QJsonObject command;
        command["type"] = "start_typing";
        command["text"] = text;
        command["settings"] = settings;
        
        QString json = QJsonDocument(command).toJson(QJsonDocument::Compact);
        
        // Send to selected client or all clients
        int selectedRow = clientList_->currentRow();
        if (selectedRow >= 0 && selectedRow < clients_.size()) {
            clients_[selectedRow]->sendTextMessage(json);
            statusLabel_->setText("Command sent to selected client");
        } else {
            // Send to all clients
            for (QWebSocket *client : clients_) {
                client->sendTextMessage(json);
            }
            statusLabel_->setText(QString("Command sent to %1 client(s)").arg(clients_.size()));
        }
        
        startButton_->setEnabled(false);
        stopButton_->setEnabled(true);
    }
    
    void stopTyping() {
        QJsonObject command;
        command["type"] = "stop_typing";
        
        QString json = QJsonDocument(command).toJson(QJsonDocument::Compact);
        
        for (QWebSocket *client : clients_) {
            client->sendTextMessage(json);
        }
        
        startButton_->setEnabled(true);
        stopButton_->setEnabled(false);
        statusLabel_->setText("Stop command sent");
    }

private:
    void setupUI() {
        setWindowTitle("qtype Server - Remote Typing Control");
        setMinimumSize(900, 600);
        
        QWidget *central = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        
        // Server info
        QLabel *serverInfo = new QLabel(this);
        QString ips = getLocalIPs();
        serverInfo->setText(QString("Server running on port 9999\nConnect clients to: %1:9999").arg(ips));
        serverInfo->setStyleSheet("padding: 10px; background-color:#d4edda; font-weight: bold;");
        mainLayout->addWidget(serverInfo);
        
        // Client list
        QGroupBox *clientGroup = new QGroupBox("Connected Clients");
        QVBoxLayout *clientLayout = new QVBoxLayout(clientGroup);
        clientList_ = new QListWidget(this);
        clientList_->setMaximumHeight(100);
        clientLayout->addWidget(new QLabel("Select target client (or send to all):"));
        clientLayout->addWidget(clientList_);
        mainLayout->addWidget(clientGroup);
        
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
        
        mainLayout->addLayout(topLayout);
        
        // Imperfections
        QGroupBox *imperfGroup = new QGroupBox("Human Imperfections");
        QHBoxLayout *imperfLayout = new QHBoxLayout(imperfGroup);
        
        QVBoxLayout *col1 = new QVBoxLayout();
        QHBoxLayout *typoLayout = new QHBoxLayout();
        typoCheck_ = new QCheckBox("Typos", this);
        typoCheck_->setChecked(true);
        typoMinSpin_ = new QSpinBox(this);
        typoMinSpin_->setRange(50, 10000);
        typoMinSpin_->setValue(300);
        typoMaxSpin_ = new QSpinBox(this);
        typoMaxSpin_->setRange(50, 10000);
        typoMaxSpin_->setValue(500);
        typoLayout->addWidget(typoCheck_);
        typoLayout->addWidget(typoMinSpin_);
        typoLayout->addWidget(new QLabel("—"));
        typoLayout->addWidget(typoMaxSpin_);
        col1->addLayout(typoLayout);
        
        QVBoxLayout *col2 = new QVBoxLayout();
        QHBoxLayout *doubleLayout = new QHBoxLayout();
        doubleCheck_ = new QCheckBox("Double-key", this);
        doubleCheck_->setChecked(true);
        doubleMinSpin_ = new QSpinBox(this);
        doubleMinSpin_->setRange(50, 10000);
        doubleMinSpin_->setValue(250);
        doubleMaxSpin_ = new QSpinBox(this);
        doubleMaxSpin_->setRange(50, 10000);
        doubleMaxSpin_->setValue(400);
        doubleLayout->addWidget(doubleCheck_);
        doubleLayout->addWidget(doubleMinSpin_);
        doubleLayout->addWidget(new QLabel("—"));
        doubleLayout->addWidget(doubleMaxSpin_);
        col2->addLayout(doubleLayout);
        
        QVBoxLayout *col3 = new QVBoxLayout();
        QHBoxLayout *autoLayout = new QHBoxLayout();
        autoCorrectCheck_ = new QCheckBox("Auto-correct", this);
        autoCorrectCheck_->setChecked(true);
        autoCorrectProbSpin_ = new QSpinBox(this);
        autoCorrectProbSpin_->setRange(0, 100);
        autoCorrectProbSpin_->setValue(15);
        autoCorrectProbSpin_->setSuffix("%");
        autoLayout->addWidget(autoCorrectCheck_);
        autoLayout->addWidget(autoCorrectProbSpin_);
        col3->addLayout(autoLayout);
        
        imperfLayout->addLayout(col1);
        imperfLayout->addLayout(col2);
        imperfLayout->addLayout(col3);
        mainLayout->addWidget(imperfGroup);
        
        // Text edit
        textEdit_ = new QPlainTextEdit(this);
        textEdit_->setPlaceholderText("Paste your text here... It will be sent to the selected client for typing.");
        mainLayout->addWidget(textEdit_);
        
        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        startButton_ = new QPushButton("Start Remote Typing");
        stopButton_ = new QPushButton("Stop");
        stopButton_->setEnabled(false);
        
        connect(startButton_, &QPushButton::clicked, this, &QTypeServer::startTyping);
        connect(stopButton_, &QPushButton::clicked, this, &QTypeServer::stopTyping);
        
        buttonLayout->addWidget(startButton_);
        buttonLayout->addWidget(stopButton_);
        mainLayout->addLayout(buttonLayout);
        
        // Status
        statusLabel_ = new QLabel("Waiting for clients...");
        statusLabel_->setAlignment(Qt::AlignCenter);
        statusLabel_->setStyleSheet("padding: 8px; font-size: 13px;");
        mainLayout->addWidget(statusLabel_);
        
        setCentralWidget(central);
    }
    
    void startServer() {
        wsServer_ = new QWebSocketServer("qtype-server", QWebSocketServer::NonSecureMode, this);
        
        if (wsServer_->listen(QHostAddress::Any, 9999)) {
            connect(wsServer_, &QWebSocketServer::newConnection, this, &QTypeServer::onNewConnection);
            statusLabel_->setText("Server started on port 9999");
        } else {
            statusLabel_->setText("Failed to start server!");
        }
    }
    
    void updateClientList() {
        clientList_->clear();
        for (QWebSocket *client : clients_) {
            QString info = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
            clientList_->addItem(info);
        }
        
        if (clients_.isEmpty()) {
            startButton_->setEnabled(false);
            statusLabel_->setText("No clients connected");
        } else {
            startButton_->setEnabled(true);
        }
    }
    
    QString getLocalIPs() {
        QStringList ips;
        for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
            if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
                !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        ips << entry.ip().toString();
                    }
                }
            }
        }
        return ips.join(", ");
    }

private:
    QWebSocketServer *wsServer_ = nullptr;
    QList<QWebSocket*> clients_;
    
    QPlainTextEdit *textEdit_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QListWidget *clientList_ = nullptr;
    
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
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QTypeServer server;
    server.show();
    return app.exec();
}

#include "qtype_server.moc"
