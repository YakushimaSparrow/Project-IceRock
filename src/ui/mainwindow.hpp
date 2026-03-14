#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QSplitter>
#include <QScrollArea>
#include <QLineEdit>
#include <QFrame>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDate>
#include <QString>
#include <QVector>
#include <QMap>
#include <QRandomGenerator>
#include <QFont>
#include <QFontMetrics>
#include <QColor>
#include <QtMath>
#include <cmath>

#include "TinkoffBridge.hpp"

// -------------------------------------------------------
// Структуры данных
// -------------------------------------------------------
struct CandleData {
    QDate     date;
    double    open   = 0.0;
    double    high   = 0.0;
    double    low    = 0.0;
    double    close  = 0.0;
    long long volume = 0;
};

struct Asset {
    QString  ticker;
    QString  name;
    QString  sector;
    int      tickCount    = 0;
    double   avgPrice     = 0.0;
    double   currentPrice = 0.0;
    double   prevPrice    = 0.0;
    double   profit       = 0.0;
    double   personalRisk = 0.0;
    bool     liveData     = false;
    QVector<CandleData> candles;
};

struct NewsItem {
    QString source;
    QString title;
    QString body;
    QString relatedTicker;
    QDate   date;
};

// -------------------------------------------------------
// Виджеты
// -------------------------------------------------------
class AssetRowWidget : public QWidget {
    Q_OBJECT
public:
    explicit AssetRowWidget(const Asset& a, bool up, double pct,
                            bool live = false, QWidget* parent = nullptr);
};

class PieChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit PieChartWidget(const QVector<Asset>& assets,
                            bool dark = true, QWidget* parent = nullptr);
    void setAssets(const QVector<Asset>& a);
    void setDark(bool dark);
protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
private:
    QVector<Asset>  m_assets;
    QVector<QColor> m_palette;
    int  m_hovered = -1;
    bool m_dark    = true;
};

class RiskGaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit RiskGaugeWidget(double value, double maxVal = 10.0,
                             QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    double m_value, m_max;
};

class AllocationBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AllocationBarWidget(const QVector<Asset>& assets,
                                 bool dark = true, QWidget* parent = nullptr);
    void setAssets(const QVector<Asset>& a);
    void setDark(bool dark);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QVector<Asset> m_assets;
    bool           m_dark = true;
};

class CandleChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit CandleChartWidget(const QVector<CandleData>& candles,
                               QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
private:
    QVector<CandleData> m_candles;
    int m_hoverIdx  = -1;
    int m_viewStart = 0;
};

class TickerBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit TickerBarWidget(const QVector<Asset>& assets, QWidget* parent = nullptr);
    void setAssets(const QVector<Asset>& a);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QVector<Asset> m_assets;
    QTimer* m_timer  = nullptr;
    int     m_offset = 0;
};

class SummaryRowWidget : public QWidget {
    Q_OBJECT
public:
    explicit SummaryRowWidget(const Asset& a, QWidget* parent = nullptr);
};

class NewsCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit NewsCardWidget(const NewsItem& n, QWidget* parent = nullptr);
};

// -------------------------------------------------------
// Главное окно
// -------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& token = QString(),
                        QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private slots:
    void onAssetDoubleClicked(QListWidgetItem* item);
    void toggleTheme();
    void onAllPricesUpdated(const QVector<LivePrice>& prices);
    void onConnectionError(const QString& msg);
    void onBridgeConnected();
    void onSimTick();

private:
    void setupUI();
    void buildHeader();
    void buildLeftPanel();
    void buildRightPanel();
    void buildMenuBar();
    void buildHomePage();
    void buildNewsPage();
    void buildAnalyticsPage();
    void buildCalendarPage();
    void buildScreenerPage();
    void buildSettingsPage();
    void showAssetDetail(int index);
    QWidget* buildKPIStrip(const Asset& a);
    void switchPage(int idx);
    void populateAssetList();
    void applyTheme();
    void applyDarkTheme();
    void applyLightTheme();

    QVector<Asset>    m_assets;
    QVector<NewsItem> m_news;

    bool   m_darkTheme = true;
    bool   m_dragging  = false;
    QPoint m_dragPos;

    TinkoffBridge* m_bridge = nullptr;
    QTimer*        m_simTimer = nullptr;

    QLabel*          m_liveStatusLabel = nullptr;
    QWidget*         m_central         = nullptr;
    TickerBarWidget* m_tickerBar        = nullptr;
    QWidget*         m_headerWidget     = nullptr;
    QWidget*         m_leftPanel        = nullptr;
    QListWidget*     m_assetList        = nullptr;
    QWidget*         m_rightPanel       = nullptr;
    QWidget*         m_menuBarWidget    = nullptr;
    QStackedWidget*  m_pageStack        = nullptr;
    QPushButton*     m_themeBtn         = nullptr;
    PieChartWidget*  m_pieChart         = nullptr;
    AllocationBarWidget* m_allocBar     = nullptr;
    QWidget*         m_detailPage       = nullptr;
};
