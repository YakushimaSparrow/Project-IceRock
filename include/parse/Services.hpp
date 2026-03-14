#pragma once

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <thread>
#include <vector>

#include "marketdata.grpc.pb.h"
#include "sandbox.grpc.pb.h"
#include "tinkoffinvestsdk_export.h"

#ifndef TINKOFFINVESTSDK_EXPORT
#define TINKOFFINVESTSDK_EXPORT
#endif

using grpc::ClientAsyncReaderWriter;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using namespace tinkoff::public_::invest::api::contract::v1;

static const std::string APP_NAME = "IceRock.TinkoffInvestSDK";

// ==================== Type Aliases ====================

using CallbackFunc = std::function<void(class ServiceReply)>;
using Strings = std::vector<std::string>;

// ==================== ServiceReply ====================

/*!
    \brief Класс-обертка над proto-ответами сервисов
*/
class TINKOFFINVESTSDK_EXPORT ServiceReply
{
public:
    ServiceReply();
    ServiceReply(const std::shared_ptr<google::protobuf::Message> protoMsg,
                 const Status& status,
                 const std::string& messageIfError = "");

    const std::shared_ptr<google::protobuf::Message> ptr() const;
    const std::string accountID(int i) const;
    const std::string accountName(int i) const;
    int accountCount() const;
    const Status& GetStatus() const;
    const std::string& GetErrorMessage() const;

    template <class T>
    static ServiceReply prepareServiceAnswer(const Status& status,
                                             const T& protoMsg,
                                             const std::string& messageIfError = "")
    {
        return status.ok()
            ? ServiceReply(std::make_shared<T>(protoMsg), status)
            : ServiceReply(nullptr, status, messageIfError);
    }

private:
    std::shared_ptr<google::protobuf::Message> m_replyPtr;
    Status m_status;
    std::string m_errorMessage;
};

// ==================== RpcHandler (базовый класс для стриминга) ====================

/*!
    \brief Базовый класс для асинхронных RPC вызовов
*/
class RpcHandler
{
protected:
    struct TagData
    {
        enum class Type
        {
            start_done,
            read_done,
            write_done
        };

        RpcHandler* handler;
        Type evt;
    };

    struct TagSet
    {
        explicit TagSet(RpcHandler* self)
            : start_done{self, TagData::Type::start_done}
            , read_done{self, TagData::Type::read_done}
            , write_done{self, TagData::Type::write_done}
        {}

        TagData start_done;
        TagData read_done;
        TagData write_done;
    };

public:
    RpcHandler();
    virtual ~RpcHandler() = default;

    TagSet tags;
    ClientContext context;

    virtual void on_ready() = 0;
    virtual void on_recv() = 0;
    virtual void on_write_done() = 0;

    static void handlingThread(CompletionQueue* cq);
};

// ==================== MarketDataHandler (для стриминга цен) ====================

/*!
    \brief Обработчик асинхронных вызовов MarketDataStream сервиса
*/
class MarketDataHandler final : public RpcHandler
{
public:
    using responder_ptr = std::unique_ptr<ClientAsyncReaderWriter<MarketDataRequest, MarketDataResponse>>;

    MarketDataHandler(responder_ptr responder, CallbackFunc callback);
    MarketDataHandler(CompletionQueue& cq,
                      std::unique_ptr<MarketDataStreamService::Stub>& stub,
                      const std::string& token,
                      CallbackFunc callback);
    ~MarketDataHandler() override;

    void send(const MarketDataRequest& msg);

private:
    void on_ready() override;
    void on_recv() override;
    void on_write_done() override;

    responder_ptr responder_;
    MarketDataResponse incoming_;
    bool sending_ = false;
    bool ready_ = false;
    std::queue<MarketDataRequest> queued_msgs_;
    CallbackFunc callback_;
};

// ==================== CustomService ====================

/*!
    \brief Родительский класс для всех сервисов
*/
class TINKOFFINVESTSDK_EXPORT CustomService
{
public:
    explicit CustomService(const std::string& token);
    virtual ~CustomService() = default;

protected:
    const std::string m_token;
    CompletionQueue m_cq;
    std::unique_ptr<std::thread> m_grpcThread;

    std::shared_ptr<ClientContext> makeContext();
    void StartThread();
};

// ==================== Sandbox (для портфельного менеджера) ====================

/*!
    \brief Сервис для работы с песочницей
*/
class TINKOFFINVESTSDK_EXPORT Sandbox : public CustomService
{
public:
    Sandbox(std::shared_ptr<grpc::Channel> channel, const std::string& token);
    ~Sandbox() override;

    // Управление счетами
    ServiceReply OpenSandboxAccount();
    ServiceReply GetSandboxAccounts();
    ServiceReply CloseSandboxAccount(const std::string& accountId);

    // Информация о портфеле
    ServiceReply GetSandboxPortfolio(const std::string& accountId,
                                     PortfolioRequest_CurrencyRequest currency);
    ServiceReply GetSandboxPositions(const std::string& accountId);
    ServiceReply GetSandboxOperations(const std::string& accountId,
                                      int64_t fromSeconds,
                                      int32_t fromNanos,
                                      int64_t toSeconds,
                                      int32_t toNanos);

    // Управление средствами
    ServiceReply SandboxPayIn(const std::string& accountId,
                              const std::string& currency,
                              int64_t units,
                              int32_t nano);

    // Торговые операции
    ServiceReply PostSandboxOrder(const std::string& figi,
                                  int64_t quantity,
                                  int64_t units,
                                  int32_t nano,
                                  OrderDirection direction,
                                  const std::string& accountId,
                                  OrderType orderType,
                                  const std::string& orderId);
    ServiceReply GetSandboxOrderState(const std::string& accountId,
                                      const std::string& orderId);
    ServiceReply CancelSandboxOrder(const std::string& accountId,
                                    const std::string& orderId);
    ServiceReply GetSandboxOrders(const std::string& accountId);

private:
    std::unique_ptr<SandboxService::Stub> m_sandboxService;
};

// ==================== MarketDataStream (только для получения цен) ====================

/*!
    \brief Сервис получения биржевой информации в режиме стриминга
*/
class TINKOFFINVESTSDK_EXPORT MarketDataStream : public CustomService
{
public:
    MarketDataStream(std::shared_ptr<grpc::Channel> channel, const std::string& token);
    ~MarketDataStream() override;

    // Для отображения графиков
    bool SubscribeCandles(const std::vector<std::pair<std::string, SubscriptionInterval>>& candleInstruments,
                          CallbackFunc callback);

    // Для отображения глубины рынка
    bool SubscribeOrderBook(const std::string& figi,
                            int32_t depth,
                            CallbackFunc callback);

    // Для получения текущих цен (асинхронный режим для real-time обновлений)
    void SubscribeLastPriceAsync(const Strings& figis,
                                 CallbackFunc callback);

    // Отмена подписки на цены
    bool UnSubscribeLastPrice();

private:
    void SendRequest(const MarketDataRequest& request, CallbackFunc callback = nullptr);

    std::unique_ptr<MarketDataStreamService::Stub> m_marketDataStreamService;
    std::set<std::shared_ptr<MarketDataHandler>> m_currentHandlers;
};