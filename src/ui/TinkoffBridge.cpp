#include "TinkoffBridge.hpp"
#include "Services.hpp"
#include <QMetaObject>
#include <QDebug>
#include <stdexcept>

struct TinkoffBridge::Impl {
    std::shared_ptr<grpc::Channel>    channel;
    std::unique_ptr<MarketDataStream> stream;
    QMap<QString,QString>             figiToTicker;
};

TinkoffBridge::TinkoffBridge(const QString& token, QObject* parent)
    : QObject(parent), m_token(token), m_impl(std::make_unique<Impl>())
{
    for (auto it = TICKER_TO_FIGI.cbegin(); it != TICKER_TO_FIGI.cend(); ++it)
        m_impl->figiToTicker.insert(it.value(), it.key());

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(1000);
    connect(m_pollTimer, &QTimer::timeout, this, &TinkoffBridge::onPollTimer);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(10000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TinkoffBridge::onReconnectTimer);
}

TinkoffBridge::~TinkoffBridge() { stop(); }

void TinkoffBridge::start() { connectStreams(); }

void TinkoffBridge::stop() {
    m_pollTimer->stop();
    m_reconnectTimer->stop();
    disconnectStreams();
}

void TinkoffBridge::connectStreams()
{
    try {
        const std::string endpoint = "sandbox-invest-public-api.tinkoff.ru:443";
        auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions{});
        m_impl->channel = grpc::CreateChannel(endpoint, creds);
        m_impl->stream  = std::make_unique<MarketDataStream>(
            m_impl->channel, m_token.toStdString());

        std::vector<std::string> figis;
        figis.reserve(TICKER_TO_FIGI.size());
        for (const auto& f : TICKER_TO_FIGI.values())
            figis.push_back(f.toStdString());

        m_impl->stream->SubscribeLastPriceAsync(figis,
            [this](ServiceReply reply)
            {
                if (!reply.GetStatus().ok()) {
                    const std::string err = reply.GetStatus().error_message();
                    QMetaObject::invokeMethod(this, [this, err]() {
                        m_connected = false;
                        emit connectionError(QString::fromStdString(err));
                        m_reconnectTimer->start();
                    }, Qt::QueuedConnection);
                    return;
                }

                auto* resp = dynamic_cast<MarketDataResponse*>(reply.ptr().get());
                if (!resp || !resp->has_last_price()) return;

                const auto& lp    = resp->last_price();
                const double price = lp.price().units() + lp.price().nano() * 1e-9;
                const QString figi = QString::fromStdString(lp.figi());
                const QString ticker = m_impl->figiToTicker.value(figi);
                if (ticker.isEmpty()) return;

                QMetaObject::invokeMethod(this, [this, ticker, price]() {
                    const double prev = m_prices.value(ticker, price);
                    m_prevPrices[ticker] = prev;
                    m_prices[ticker]     = price;
                }, Qt::QueuedConnection);
            });

        m_connected = true;
        m_pollTimer->start();
        emit connected();
        qDebug() << "[TinkoffBridge] Connected to sandbox.";

    } catch (const std::exception& ex) {
        m_connected = false;
        emit connectionError(QString("Connect error: ") + ex.what());
        m_reconnectTimer->start();
    }
}

void TinkoffBridge::disconnectStreams() {
    m_impl->stream.reset();
    m_impl->channel.reset();
    m_connected = false;
}

void TinkoffBridge::onReconnectTimer() {
    qDebug() << "[TinkoffBridge] Reconnecting...";
    connectStreams();
}

void TinkoffBridge::onPollTimer()
{
    if (m_prices.isEmpty()) return;
    QVector<LivePrice> batch;
    batch.reserve(m_prices.size());
    for (auto it = m_prices.cbegin(); it != m_prices.cend(); ++it) {
        LivePrice lp;
        lp.ticker    = it.key();
        lp.price     = it.value();
        lp.prevPrice = m_prevPrices.value(it.key(), it.value());
        lp.isValid   = true;
        batch.append(lp);
        emit priceUpdated(lp.ticker, lp.price, lp.prevPrice);
    }
    emit allPricesUpdated(batch);
}
