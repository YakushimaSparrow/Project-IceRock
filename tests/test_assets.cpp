#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <stdexcept>

// Структура для хранения данных об акции
struct TAsset {
    std::string ticker; // Название (тикер)
    double avg = 0;     // Цена покупки
    double cur = 0;     // Текущая цена
    int tc = 0;         // Количество штук
};

// Считает чистую прибыль по одной акции
static double profit(const TAsset& a) {
    return (a.cur - a.avg) * a.tc;
}

// Считает общую стоимость всего портфеля
static double portVal(const std::vector<TAsset>& v) {
    double t = 0;
    for (const auto& a : v) t += a.cur * a.tc;
    return t;
}

// Ищет акцию в списке по названию
static std::optional<TAsset> find(const std::vector<TAsset>& v, const std::string& tk) {
    for (const auto& a : v)
        if (a.ticker == tk) return a;
    return std::nullopt;
}

// Ошибка, если данные портфеля введены неверно
class PortEx : public std::runtime_error {
public:
    explicit PortEx(const std::string& m) : std::runtime_error(m) {}
};

// Проверка прибыли: в плюсе, в минусе и ноль
TEST(Asset, ProfitPos) { TAsset a{"SBER", 240, 280, 1200}; EXPECT_DOUBLE_EQ(profit(a), 48000); }
TEST(Asset, ProfitNeg) { TAsset a{"YNDX", 3500, 3200, 100}; EXPECT_DOUBLE_EQ(profit(a), -30000); }
TEST(Asset, ProfitZero) { TAsset a{"GAZP", 150, 150, 800}; EXPECT_DOUBLE_EQ(profit(a), 0); }

// Проверка стоимости: обычный портфель и пустой
TEST(Portfolio, Total) {
    std::vector<TAsset> v = {{"SBER", 240, 280, 1200}, {"GAZP", 148, 162, 800}, {"LKOH", 6500, 6820, 300}};
    EXPECT_DOUBLE_EQ(portVal(v), 280 * 1200 + 162 * 800 + 6820 * 300);
}
TEST(Portfolio, Empty) { EXPECT_DOUBLE_EQ(portVal({}), 0); }

// Проверка поиска: когда акция есть и когда её нет
TEST(Search, Found) {
    std::vector<TAsset> v = {{"SBER", 240, 280, 1200}};
    auto r = find(v, "SBER");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->ticker, "SBER");
}
TEST(Search, NotFound) {
    std::vector<TAsset> v = {{"SBER", 240, 280, 1200}};
    EXPECT_FALSE(find(v, "X").has_value());
}

// Проверка защиты от ошибок (например, нельзя купить 0 акций)
TEST(Exception, BadTC) {
    auto fn = [](int tc) { if (tc <= 0) throw PortEx("bad"); };
    EXPECT_THROW(fn(0), PortEx);
    EXPECT_NO_THROW(fn(1));
}