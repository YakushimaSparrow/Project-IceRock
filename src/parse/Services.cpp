#include "sandbox.grpc.pb.h"
#include "Services.hpp"

using namespace tinkoff::public_::invest::api::contract::v1;

// ==================== ServiceReply Implementation ====================

ServiceReply::ServiceReply() = default;

ServiceReply::ServiceReply(const std::shared_ptr<google::protobuf::Message> protoMsg, 
                           const Status& status, 
                           const std::string& messageIfError)
    : m_replyPtr(protoMsg)
    , m_status(status)
    , m_errorMessage(messageIfError)
{
}

const std::string ServiceReply::accountID(int i) const
{
    auto response = dynamic_cast<GetAccountsResponse*>(ptr().get());
    if (response && i < response->accounts_size()) {
        return response->accounts(i).id();
    }
    return "";
}

const std::string ServiceReply::accountName(int i) const
{
    auto response = dynamic_cast<GetAccountsResponse*>(ptr().get());
    if (response && i < response->accounts_size()) {
        return response->accounts(i).name();
    }
    return "";
}

int ServiceReply::accountCount() const
{
    auto response = dynamic_cast<GetAccountsResponse*>(ptr().get());
    return response ? response->accounts_size() : 0;
}

const std::shared_ptr<google::protobuf::Message> ServiceReply::ptr() const
{
    return m_replyPtr;
}

const Status& ServiceReply::GetStatus() const
{
    return m_status;
}

const std::string& ServiceReply::GetErrorMessage() const
{
    return m_errorMessage;
}

// ==================== RpcHandler Implementation ====================

RpcHandler::RpcHandler()
    : tags(this)
{
}

void RpcHandler::handlingThread(CompletionQueue* cq)
{
    void* raw_tag = nullptr;
    bool ok = false;

    while (cq->Next(&raw_tag, &ok)) {
        auto* tag = reinterpret_cast<TagData*>(raw_tag);
        if (ok && tag->handler) {
            switch (tag->evt) {
                case TagData::Type::start_done:
                    tag->handler->on_ready();
                    break;
                case TagData::Type::read_done:
                    tag->handler->on_recv();
                    break;
                case TagData::Type::write_done:
                    tag->handler->on_write_done();
                    break;
            }
        }
    }
}

// ==================== MarketDataHandler Implementation ====================

MarketDataHandler::MarketDataHandler(responder_ptr responder, CallbackFunc callback)
    : responder_(std::move(responder))
    , callback_(std::move(callback))
{
    responder_->StartCall(&tags.start_done);
}

MarketDataHandler::MarketDataHandler(grpc::CompletionQueue& cq_,
                                     std::unique_ptr<MarketDataStreamService::Stub>& stub_,
                                     const std::string& token,
                                     CallbackFunc callback)
    : callback_(std::move(callback))
{
    const std::string meta_value = "Bearer " + token;
    context.AddMetadata("authorization", meta_value);
    context.AddMetadata("x-app-name", APP_NAME);

    responder_ = stub_->PrepareAsyncMarketDataStream(&context, &cq_);
    responder_->StartCall(&tags.start_done);
}

MarketDataHandler::~MarketDataHandler() = default;

void MarketDataHandler::send(const MarketDataRequest& msg)
{
    if (ready_ && !sending_) {
        sending_ = true;
        responder_->Write(msg, &tags.write_done);
    } else {
        queued_msgs_.push(msg);
    }
}

void MarketDataHandler::on_ready()
{
    ready_ = true;

    if (!queued_msgs_.empty()) {
        sending_ = true;
        responder_->Write(queued_msgs_.front(), &tags.start_done);
        queued_msgs_.pop();
    } else {
        responder_->Read(&incoming_, &tags.read_done);
    }
}

void MarketDataHandler::on_recv()
{
    auto data = ServiceReply(std::make_shared<MarketDataResponse>(incoming_), {});
    if (callback_) {
        callback_(data);
    }
    responder_->Read(&incoming_, &tags.read_done);
}

void MarketDataHandler::on_write_done()
{
    if (!queued_msgs_.empty()) {
        responder_->Write(queued_msgs_.front(), &tags.write_done);
        queued_msgs_.pop();
    } else {
        sending_ = false;
    }
}

// ==================== CustomService Implementation ====================

CustomService::CustomService(const std::string& token)
    : m_token(token)
{
}

std::shared_ptr<grpc::ClientContext> CustomService::makeContext()
{
    auto context = std::make_shared<grpc::ClientContext>();
    const std::string meta_value = "Bearer " + m_token;
    context->AddMetadata("authorization", meta_value);
    context->AddMetadata("x-app-name", APP_NAME);
    return context;
}

void CustomService::StartThread()
{
    if (!m_grpcThread) {
        m_grpcThread = std::make_unique<std::thread>(&RpcHandler::handlingThread, &m_cq);
    }
}

// ==================== Sandbox Implementation ====================

Sandbox::Sandbox(std::shared_ptr<grpc::Channel> channel, const std::string& token)
    : CustomService(token)
    , m_sandboxService(SandboxService::NewStub(channel))
{
}

Sandbox::~Sandbox() = default;

ServiceReply Sandbox::OpenSandboxAccount()
{
    OpenSandboxAccountRequest request;
    OpenSandboxAccountResponse reply;
    Status status = m_sandboxService->OpenSandboxAccount(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxAccounts()
{
    GetAccountsRequest request;
    GetAccountsResponse reply;
    Status status = m_sandboxService->GetSandboxAccounts(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::CloseSandboxAccount(const std::string& accountId)
{
    CloseSandboxAccountRequest request;
    request.set_account_id(accountId);
    CloseSandboxAccountResponse reply;
    Status status = m_sandboxService->CloseSandboxAccount(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::PostSandboxOrder(const std::string& figi,
                                      int64_t quantity,
                                      int64_t units,
                                      int32_t nano,
                                      OrderDirection direction,
                                      const std::string& accountId,
                                      OrderType orderType,
                                      const std::string& orderId)
{
    PostOrderRequest request;
    
    // Подавляем предупреждения об устаревшем FIGI
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    request.set_figi(figi);
    #pragma GCC diagnostic pop
    
    request.set_quantity(quantity);
    request.set_direction(direction);
    request.set_account_id(accountId);
    request.set_order_type(orderType);
    request.set_order_id(orderId);

    auto price = std::make_unique<Quotation>();
    price->set_units(units);
    price->set_nano(nano);
    request.set_allocated_price(price.release());

    PostOrderResponse reply;
    Status status = m_sandboxService->PostSandboxOrder(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxOrders(const std::string& accountId)
{
    GetOrdersRequest request;
    request.set_account_id(accountId);
    GetOrdersResponse reply;
    Status status = m_sandboxService->GetSandboxOrders(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::CancelSandboxOrder(const std::string& accountId, const std::string& orderId)
{
    CancelOrderRequest request;
    request.set_account_id(accountId);
    request.set_order_id(orderId);
    CancelOrderResponse reply;
    Status status = m_sandboxService->CancelSandboxOrder(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxOrderState(const std::string& accountId, const std::string& orderId)
{
    GetOrderStateRequest request;
    request.set_account_id(accountId);
    request.set_order_id(orderId);
    OrderState reply;
    Status status = m_sandboxService->GetSandboxOrderState(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxPositions(const std::string& accountId)
{
    PositionsRequest request;
    request.set_account_id(accountId);
    PositionsResponse reply;
    Status status = m_sandboxService->GetSandboxPositions(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxOperations(const std::string& accountId,
                                          int64_t fromseconds,
                                          int32_t fromnanos,
                                          int64_t toseconds,
                                          int32_t tonanos)
{
    OperationsRequest request;
    request.set_account_id(accountId);

    auto from = std::make_unique<google::protobuf::Timestamp>();
    from->set_seconds(fromseconds);
    from->set_nanos(fromnanos);
    request.set_allocated_from(from.release());

    auto to = std::make_unique<google::protobuf::Timestamp>();
    to->set_seconds(toseconds);
    to->set_nanos(tonanos);
    request.set_allocated_to(to.release());

    OperationsResponse reply;
    Status status = m_sandboxService->GetSandboxOperations(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::GetSandboxPortfolio(const std::string& accountId,
                                         PortfolioRequest_CurrencyRequest currency)
{
    PortfolioRequest request;
    request.set_account_id(accountId);
    request.set_currency(currency);
    PortfolioResponse reply;
    Status status = m_sandboxService->GetSandboxPortfolio(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

ServiceReply Sandbox::SandboxPayIn(const std::string& accountId,
                                  const std::string& currency,
                                  int64_t units,
                                  int32_t nano)
{
    SandboxPayInRequest request;
    request.set_account_id(accountId);

    auto amount = std::make_unique<MoneyValue>();
    amount->set_currency(currency);
    amount->set_units(units);
    amount->set_nano(nano);
    request.set_allocated_amount(amount.release());

    SandboxPayInResponse reply;
    Status status = m_sandboxService->SandboxPayIn(makeContext().get(), request, &reply);
    return ServiceReply::prepareServiceAnswer(status, reply);
}

// ==================== MarketDataStream Implementation ====================

MarketDataStream::MarketDataStream(std::shared_ptr<grpc::Channel> channel, const std::string& token)
    : CustomService(token)
    , m_marketDataStreamService(MarketDataStreamService::NewStub(channel))
{
}

MarketDataStream::~MarketDataStream()
{
    if (m_grpcThread) {
        m_grpcThread->join();
    }
}

namespace {
    bool processStreamingRequest(
        std::unique_ptr<MarketDataStreamService::Stub>& stub,
        const std::string& token,
        const MarketDataRequest& request,
        CallbackFunc callback)
    {
        ClientContext context;
        const std::string meta_value = "Bearer " + token;
        context.AddMetadata("authorization", meta_value);
        context.AddMetadata("x-app-name", APP_NAME);

        auto stream = stub->MarketDataStream(&context);

        std::thread writer([stream = stream.get(), request]() {
            stream->Write(request);
            stream->WritesDone();
        });

        MarketDataResponse reply;
        while (stream->Read(&reply)) {
            if (callback) {
                auto data = ServiceReply(std::make_shared<MarketDataResponse>(reply), {});
                callback(data);
            }
        }

        writer.join();
        Status status = stream->Finish();
        return status.ok();
    }
}

bool MarketDataStream::SubscribeCandles(
    const std::vector<std::pair<std::string, SubscriptionInterval>>& candleInstruments,
    CallbackFunc callback)
{
    MarketDataRequest request;
    auto scr = std::make_unique<SubscribeCandlesRequest>();
    scr->set_subscription_action(SubscriptionAction::SUBSCRIPTION_ACTION_SUBSCRIBE);

    for (const auto& [figi, interval] : candleInstruments) {
        auto instr = scr->add_instruments();
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        instr->set_figi(figi);
        #pragma GCC diagnostic pop
        
        instr->set_interval(interval);
    }

    request.set_allocated_subscribe_candles_request(scr.release());
    return processStreamingRequest(m_marketDataStreamService, m_token, request, callback);
}

bool MarketDataStream::SubscribeOrderBook(const std::string& figi, int32_t depth, CallbackFunc callback)
{
    MarketDataRequest request;
    auto sobr = std::make_unique<SubscribeOrderBookRequest>();
    sobr->set_subscription_action(SubscriptionAction::SUBSCRIPTION_ACTION_SUBSCRIBE);

    auto obi = sobr->add_instruments();
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    obi->set_figi(figi);
    #pragma GCC diagnostic pop
    
    obi->set_depth(depth);

    request.set_allocated_subscribe_order_book_request(sobr.release());
    return processStreamingRequest(m_marketDataStreamService, m_token, request, callback);
}

void MarketDataStream::SubscribeLastPriceAsync(const Strings& figis, CallbackFunc callback)
{
    MarketDataRequest request;
    auto slpr = std::make_unique<SubscribeLastPriceRequest>();
    slpr->set_subscription_action(SubscriptionAction::SUBSCRIPTION_ACTION_SUBSCRIBE);

    for (const auto& figi : figis) {
        auto instr = slpr->add_instruments();
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        instr->set_figi(figi);
        #pragma GCC diagnostic pop
    }

    request.set_allocated_subscribe_last_price_request(slpr.release());
    SendRequest(request, std::move(callback));
}

bool MarketDataStream::UnSubscribeLastPrice()
{
    MarketDataRequest request;
    auto slpr = std::make_unique<SubscribeLastPriceRequest>();
    slpr->set_subscription_action(SubscriptionAction::SUBSCRIPTION_ACTION_UNSUBSCRIBE);
    request.set_allocated_subscribe_last_price_request(slpr.release());
    return processStreamingRequest(m_marketDataStreamService, m_token, request, nullptr);
}

void MarketDataStream::SendRequest(const MarketDataRequest& request, CallbackFunc callback)
{
    StartThread();
    auto handler = std::make_shared<MarketDataHandler>(
        m_cq, m_marketDataStreamService, m_token, std::move(callback)
    );
    handler->send(request);
    m_currentHandlers.insert(std::move(handler));
}