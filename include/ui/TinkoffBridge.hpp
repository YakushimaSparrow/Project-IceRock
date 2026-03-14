#pragma once
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QString>
#include <QMap>
#include <memory>

// Живая цена одного инструмента
struct LivePrice {
    QString ticker;
    double  price     = 0.0;
    double  prevPrice = 0.0;
    bool    isValid   = false;
};

// FIGI инструментов MOEX
static const QMap<QString,QString> TICKER_TO_FIGI = {
    {"SBER",   "BBG004730N88"},
    {"GAZP",   "BBG004730RP0"},
    {"LKOH",   "BBG004731032"},
    {"YNDX",   "BBG006L8G4H1"},
    {"GMKN",   "BBG004731489"},
    {"ROSN",   "BBG004731354"},
    {"GOLD",   "BBG000CRF6Q8"},
    {"USDRUB", "BBG0013HGFT4"},
    {"OFZ238", "BBG00X2FC966"},
    {"MGNT",   "BBG004RVFCY3"},
};

// TinkoffBridge — Qt-обёртка над MarketDataStream (parse-модуль).
// Работает только с sandbox API Тинькофф.
// Живёт на UI-потоке; gRPC-колбэки переключаются через QueuedConnection.
class TinkoffBridge : public QObject {
    Q_OBJECT
public:
    explicit TinkoffBridge(const QString& token, QObject* parent = nullptr);
    ~TinkoffBridge() override;

    void start();
    void stop();
    bool isConnected() const { return m_connected; }

signals:
    void priceUpdated(const QString& ticker, double price, double prevPrice);
    void allPricesUpdated(const QVector<LivePrice>& prices);
    void connectionError(const QString& msg);
    void connected();

private slots:
    void onPollTimer();
    void onReconnectTimer();

private:
    void connectStreams();
    void disconnectStreams();

    QString m_token;
    bool    m_connected = false;
    QMap<QString,double> m_prices;
    QMap<QString,double> m_prevPrices;
    QTimer* m_pollTimer      = nullptr;
    QTimer* m_reconnectTimer = nullptr;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
