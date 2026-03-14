#include "mainwindow.hpp"
#include <QApplication>
#include <QScrollBar>

// ================================================================
// Хелперы генерации данных
// ================================================================
static QVector<CandleData> makeCandles(double start,int n,double vol,quint32 seed)
{
    QVector<CandleData> out; double price=start;
    QDate d=QDate::currentDate().addDays(-n);
    QRandomGenerator rng(seed);
    for(int i=0;i<n;++i){
        double open=price,trend=(rng.generateDouble()-0.47)*vol*price;
        double close=open+trend;
        double hi=qMax(open,close)+rng.generateDouble()*vol*price*0.3;
        double lo=qMin(open,close)-rng.generateDouble()*vol*price*0.3;
        lo=qMax(lo,price*0.5);
        long long v=(long long)(rng.bounded(5000000)+100000);
        out.append({d,open,hi,lo,close,v});
        price=close; d=d.addDays(1);
        while(d.dayOfWeek()>5) d=d.addDays(1);
    }
    return out;
}

static QVector<Asset> generateAssets()
{
    QVector<Asset> list;
    auto add=[&](const QString&tk,const QString&nm,const QString&sec,
                 int tc,double avg,double vol,double pri,double sp,quint32 seed){
        Asset a; a.ticker=tk; a.name=nm; a.sector=sec;
        a.tickCount=tc; a.avgPrice=avg; a.personalRisk=pri;
        a.candles=makeCandles(sp,120,vol,seed);
        a.currentPrice=a.candles.last().close;
        a.prevPrice=a.candles[a.candles.size()-2].close;
        a.profit=(a.currentPrice-avg)*tc; a.liveData=false;
        return a;
    };
    list<<add("SBER","Сбербанк",   "Финансы",   1200, 268.5,0.022,1.8, 240.0, 42);
    list<<add("GAZP","Газпром",    "Энергетика", 800, 154.2,0.025,2.4, 148.0, 77);
    list<<add("LKOH","Лукойл",     "Нефть/газ",  300,6820.0,0.018,2.1,6500.0, 99);
    list<<add("YNDX","Яндекс",     "Технологии", 450,3250.0,0.032,3.6,3000.0,123);
    list<<add("GMKN","НорНикель",  "Металлы",    180,15400.0,0.020,3.1,16000.0,156);
    list<<add("ROSN","Роснефть",   "Нефть/газ",  650, 520.0,0.021,2.3, 500.0,200);
    list<<add("GOLD","Золото",     "Сырьё",       50,5620.0,0.015,1.4,5400.0,211);
    list<<add("USDRUB","Доллар США","Валюта",    2000,  84.2,0.012,2.8,  82.0,333);
    list<<add("OFZ238","ОФЗ 26238","Облигации",  5000, 612.0,0.008,0.8, 600.0,444);
    list<<add("MGNT","Магнит",     "Ритейл",      220,5800.0,0.024,2.9,5600.0,555);
    return list;
}

static QVector<NewsItem> generateNews()
{
    QDate t=QDate::currentDate();
    return {
        {"РБК","Россия наращивает золотые резервы","Банк России: +12 т за квартал.","GOLD",t.addDays(-1)},
        {"Ведомости","Сбербанк повысил ипотеку до 18,5%","Решение после заседания ЦБ.","SBER",t.addDays(-2)},
        {"Коммерсантъ","Газпром: СПГ-контракты с Азией на 15 лет","Сделки с КНР и Индией.","GAZP",t.addDays(-3)},
        {"Forbes","Яндекс запускает B2B-облако","Акции +3,2% по итогам дня.","YNDX",t.addDays(-3)},
        {"Forbes","Лукойл: рекордные дивиденды 1 100 ₽","Решение совета директоров.","LKOH",t.addDays(-4)},
        {"ТАСС","ЦБ РФ: ставка 16%","Инфляционное давление сохраняется.","OFZ238",t.addDays(-5)},
        {"РБК","Доллар выше 91 рубля","Рост импорта давит на рубль.","USDRUB",t.addDays(-6)},
        {"Ведомости","НорНикель: палладий -8%","Ремонт рудника.","GMKN",t.addDays(-7)},
        {"Коммерсантъ","Магнит: 200 новых магазинов","Региональная экспансия.","MGNT",t.addDays(-8)},
        {"Интерфакс","Роснефть: лицензия на Арктику","Три блока для геологоразведки.","ROSN",t.addDays(-9)},
    };
}

static QColor sectorColor(const QString& s)
{
    static const QMap<QString,QColor> m={
        {"Финансы",{79,110,247}},{"Энергетика",{245,158,11}},
        {"Нефть/газ",{249,115,22}},{"Технологии",{139,92,246}},
        {"Металлы",{107,114,128}},{"Сырьё",{251,191,36}},
        {"Валюта",{6,182,212}},{"Облигации",{52,211,153}},{"Ритейл",{236,72,153}},
    };
    return m.value(s,{75,85,99});
}

static QString fmtS(double v)
{
    bool neg=v<0; v=qAbs(v); QString s;
    if(v>=1e9) s=QString::number(v/1e9,'f',2)+" млрд";
    else if(v>=1e6) s=QString::number(v/1e6,'f',1)+" млн";
    else if(v>=1e3) s=QString::number(v/1e3,'f',1)+" тыс";
    else s=QString::number(v,'f',2);
    return (neg?"−":"+")+s;
}
static QString fmtRub(double v){return fmtS(v)+" ₽";}

// ================================================================
// MainWindow
// ================================================================
MainWindow::MainWindow(const QString& token, QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("IceRock · InvestPro");
    setMinimumSize(1280,800); resize(1500,920);
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_assets=generateAssets();
    m_news=generateNews();
    setupUI();
    applyTheme();

    if(!token.isEmpty()){
        m_bridge=new TinkoffBridge(token,this);
        connect(m_bridge,&TinkoffBridge::allPricesUpdated,this,&MainWindow::onAllPricesUpdated);
        connect(m_bridge,&TinkoffBridge::connectionError,this,&MainWindow::onConnectionError);
        connect(m_bridge,&TinkoffBridge::connected,this,&MainWindow::onBridgeConnected);
        m_bridge->start();
        if(m_liveStatusLabel) m_liveStatusLabel->setText("⏳ Sandbox...");
    } else {
        if(m_liveStatusLabel) m_liveStatusLabel->setText("🟡 Симуляция");
        m_simTimer=new QTimer(this);
        connect(m_simTimer,&QTimer::timeout,this,&MainWindow::onSimTick);
        m_simTimer->start(1000);
    }
}
MainWindow::~MainWindow()=default;

// ================================================================
// API слоты — ВАЖНО: цвета и стили не меняются при обновлении данных
// ================================================================
void MainWindow::onBridgeConnected()
{
    if(m_liveStatusLabel) m_liveStatusLabel->setText("🟢 Sandbox LIVE");
}
void MainWindow::onConnectionError(const QString& msg)
{
    Q_UNUSED(msg)
    if(m_liveStatusLabel) m_liveStatusLabel->setText("🔴 Нет связи");
}
void MainWindow::onAllPricesUpdated(const QVector<LivePrice>& prices)
{
    for(const auto& lp:prices){
        for(auto& a:m_assets){
            if(a.ticker!=lp.ticker) continue;
            a.prevPrice=lp.prevPrice;
            a.currentPrice=lp.price;
            a.profit=(a.currentPrice-a.avgPrice)*a.tickCount;
            a.liveData=true;
            if(!a.candles.isEmpty()){
                CandleData& last=a.candles.last();
                last.close=lp.price;
                last.high=qMax(last.high,lp.price);
                last.low=qMin(last.low,lp.price);
            }
            break;
        }
    }
    // Обновляем ТОЛЬКО данные виджетов, НЕ пересоздавая их — стили сохраняются
    populateAssetList();
    if(m_pieChart) m_pieChart->setAssets(m_assets);
    if(m_tickerBar) m_tickerBar->setAssets(m_assets);
    if(m_allocBar){m_allocBar->setAssets(m_assets); m_allocBar->update();}
}
void MainWindow::onSimTick()
{
    auto* rng=QRandomGenerator::global();
    for(auto& a:m_assets){
        double d=(rng->generateDouble()-0.499)*0.004*a.currentPrice;
        a.prevPrice=a.currentPrice; a.currentPrice+=d;
        a.profit=(a.currentPrice-a.avgPrice)*a.tickCount;
        if(!a.candles.isEmpty()){
            CandleData& last=a.candles.last();
            last.close=a.currentPrice;
            last.high=qMax(last.high,a.currentPrice);
            last.low=qMin(last.low,a.currentPrice);
        }
    }
    populateAssetList();
    if(m_pieChart) m_pieChart->setAssets(m_assets);
    if(m_tickerBar) m_tickerBar->setAssets(m_assets);
}

// ================================================================
// setupUI
// ================================================================
void MainWindow::setupUI()
{
    m_central=new QWidget(this); m_central->setObjectName("centralWidget");
    setCentralWidget(m_central);
    auto* root=new QVBoxLayout(m_central);
    root->setContentsMargins(12,12,12,12); root->setSpacing(6);

    m_tickerBar=new TickerBarWidget(m_assets);
    m_tickerBar->setFixedHeight(26); root->addWidget(m_tickerBar);

    buildHeader(); root->addWidget(m_headerWidget);

    auto* split=new QSplitter(Qt::Horizontal);
    split->setObjectName("mainSplitter"); split->setHandleWidth(4);
    buildLeftPanel(); buildRightPanel();
    split->addWidget(m_leftPanel); split->addWidget(m_rightPanel);
    split->setStretchFactor(0,0); split->setStretchFactor(1,1);
    split->setSizes({330,1150});
    root->addWidget(split,1);
}

void MainWindow::buildHeader()
{
    m_headerWidget=new QWidget; m_headerWidget->setObjectName("headerWidget");
    m_headerWidget->setFixedHeight(74);
    auto* hl=new QHBoxLayout(m_headerWidget);
    hl->setContentsMargins(20,0,16,0); hl->setSpacing(0);

    auto* logo=new QLabel("🧊 IceRock"); logo->setObjectName("logoLabel");
    hl->addWidget(logo); hl->addSpacing(20);

    auto* av=new QLabel("IR"); av->setObjectName("avatarLabel");
    av->setFixedSize(48,48); av->setAlignment(Qt::AlignCenter);
    hl->addWidget(av); hl->addSpacing(10);

    auto* nc=new QVBoxLayout; nc->setSpacing(1);
    auto* nl=new QLabel("IceRock Portfolio"); nl->setObjectName("accountLabel");
    auto* rl=new QLabel("Tinkoff Sandbox API"); rl->setObjectName("roleLabel");
    nc->addWidget(nl); nc->addWidget(rl); hl->addLayout(nc); hl->addSpacing(14);

    m_liveStatusLabel=new QLabel("🟡 Симуляция");
    m_liveStatusLabel->setObjectName("liveStatus");
    hl->addWidget(m_liveStatusLabel); hl->addSpacing(14);

    auto sep=[&](){auto* s=new QFrame;s->setFrameShape(QFrame::VLine);s->setObjectName("hdrSep");hl->addWidget(s);hl->addSpacing(18);};
    auto stat=[&](const QString&ic,const QString&ti,const QString&va,const QString&ob){
        auto* bx=new QVBoxLayout; bx->setSpacing(1);
        auto* tl=new QLabel(ic+"  "+ti); tl->setObjectName("statTitle");
        auto* vl=new QLabel(va); vl->setObjectName(ob);
        bx->addWidget(tl); bx->addWidget(vl); hl->addLayout(bx); hl->addSpacing(18);
    };
    double tot=0; for(const auto&a:m_assets) tot+=a.currentPrice*a.tickCount;
    sep(); stat("📈","Доходность","+14.73%","statGreen");
    sep(); stat("🛡","Инд. риска","2.34 / 10","statBlue");
    sep(); stat("💰","Портфель",QString::number(tot/1e9,'f',2)+" млрд ₽","statBalance");
    sep(); stat("📊","Позиций",QString::number(m_assets.size())+" акт.","statBlue");
    hl->addStretch();

    auto* nb=new QPushButton("🔔"); nb->setObjectName("iconBtn"); nb->setFixedSize(36,36); hl->addWidget(nb); hl->addSpacing(6);
    auto* cb=new QPushButton("✕"); cb->setObjectName("closeBtn"); cb->setFixedSize(36,36);
    connect(cb,&QPushButton::clicked,this,&QWidget::close);
    hl->addWidget(cb);
}

void MainWindow::buildLeftPanel()
{
    m_leftPanel=new QWidget; m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setMinimumWidth(300); m_leftPanel->setMaximumWidth(370);
    auto* ll=new QVBoxLayout(m_leftPanel);
    ll->setContentsMargins(8,10,6,8); ll->setSpacing(6);

    auto* sb=new QWidget; sb->setObjectName("searchBar"); sb->setFixedHeight(34);
    auto* sbl=new QHBoxLayout(sb); sbl->setContentsMargins(10,0,10,0);
    auto* si=new QLabel("🔍"); si->setFixedWidth(18);
    auto* se=new QLineEdit; se->setObjectName("searchEdit");
    se->setPlaceholderText("Поиск актива..."); se->setFrame(false);
    sbl->addWidget(si); sbl->addWidget(se); ll->addWidget(sb);

    auto* cr=new QHBoxLayout; cr->setSpacing(4);
    for(const QString&lb:{"Все","Акции","Сырьё","Валюта"}){
        auto* chip=new QPushButton(lb); chip->setObjectName("chipBtn"); chip->setFixedHeight(24); cr->addWidget(chip);
    }
    cr->addStretch(); ll->addLayout(cr);

    auto* pt=new QLabel("  АКТИВЫ  ·  "+QString::number(m_assets.size())); pt->setObjectName("panelTitle"); ll->addWidget(pt);

    m_assetList=new QListWidget; m_assetList->setObjectName("assetList");
    m_assetList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_assetList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_assetList->setSpacing(3);
    populateAssetList();
    connect(m_assetList,&QListWidget::itemDoubleClicked,this,&MainWindow::onAssetDoubleClicked);
    ll->addWidget(m_assetList,1);

    double tp=0; for(const auto&a:m_assets) tp+=a.profit;
    auto* mn=new QWidget; mn->setObjectName("miniSummary"); mn->setFixedHeight(50);
    auto* ml=new QHBoxLayout(mn); ml->setContentsMargins(12,6,12,6);
    auto* ml1=new QLabel("Итого P/L:"); ml1->setObjectName("miniLabel");
    auto* ml2=new QLabel(fmtRub(tp)); ml2->setObjectName(tp>=0?"miniValueGreen":"miniValueRed");
    ml->addWidget(ml1); ml->addStretch(); ml->addWidget(ml2);
    ll->addWidget(mn);
}

void MainWindow::buildRightPanel()
{
    m_rightPanel=new QWidget; m_rightPanel->setObjectName("rightPanel");
    auto* rl=new QVBoxLayout(m_rightPanel);
    rl->setContentsMargins(4,0,0,0); rl->setSpacing(6);
    buildMenuBar(); rl->addWidget(m_menuBarWidget);
    m_pageStack=new QStackedWidget; m_pageStack->setObjectName("pageStack");
    buildHomePage(); buildNewsPage(); buildAnalyticsPage();
    buildCalendarPage(); buildScreenerPage(); buildSettingsPage();
    rl->addWidget(m_pageStack,1);
}

void MainWindow::buildMenuBar()
{
    m_menuBarWidget=new QWidget; m_menuBarWidget->setObjectName("menuBarWidget");
    m_menuBarWidget->setFixedHeight(48);
    auto* ml=new QHBoxLayout(m_menuBarWidget);
    ml->setContentsMargins(12,6,12,6); ml->setSpacing(4);
    struct B{QString ic,tx;int pg;};
    const QVector<B> bs={{"⌂","Главная",0},{"📰","Новости",1},{"📊","Аналитика",2},{"📅","Календарь",3},{"🔭","Скринер",4}};
    for(const auto&b:bs){
        auto* btn=new QPushButton(b.ic+"  "+b.tx);
        btn->setProperty("menuBtn",true); btn->setObjectName("menuBtn");
        btn->setCursor(Qt::PointingHandCursor); int pg=b.pg;
        connect(btn,&QPushButton::clicked,[this,pg]{switchPage(pg);}); ml->addWidget(btn);
    }
    ml->addStretch();
    auto* sep=new QFrame; sep->setFrameShape(QFrame::VLine); sep->setObjectName("menuSep"); ml->addWidget(sep);
    auto* sb=new QPushButton("⚙  Настройки"); sb->setProperty("menuBtn",true); sb->setObjectName("menuBtn");
    connect(sb,&QPushButton::clicked,[this]{switchPage(5);}); ml->addWidget(sb);
    m_themeBtn=new QPushButton("☀  Тема"); m_themeBtn->setProperty("menuBtn",true);
    m_themeBtn->setObjectName("menuBtnTheme"); m_themeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_themeBtn,&QPushButton::clicked,this,&MainWindow::toggleTheme);
    ml->addWidget(m_themeBtn);
}

void MainWindow::buildHomePage()
{
    auto* page=new QWidget;
    auto* pl=new QHBoxLayout(page); pl->setContentsMargins(8,8,8,8); pl->setSpacing(10);
    auto* lc=new QWidget; auto* lcl=new QVBoxLayout(lc); lcl->setContentsMargins(0,0,0,0); lcl->setSpacing(8);
    m_pieChart=new PieChartWidget(m_assets,m_darkTheme); m_pieChart->setMinimumSize(340,340); lcl->addWidget(m_pieChart);
    auto* pc=new QWidget; pc->setObjectName("card");
    auto* pcl=new QVBoxLayout(pc); pcl->setContentsMargins(14,10,14,10);
    auto* pct=new QLabel("🛡  Personal Risk Index"); pct->setObjectName("cardTitle");
    auto* pgb=new RiskGaugeWidget(2.34,10.0); pgb->setFixedHeight(68);
    auto* pcd=new QLabel("Умеренный уровень риска (2.34/10). Защитные активы (ОФЗ, золото) — 38% портфеля.");
    pcd->setObjectName("cardText"); pcd->setWordWrap(true);
    pcl->addWidget(pct); pcl->addWidget(pgb); pcl->addWidget(pcd);
    lcl->addWidget(pc); lcl->addStretch();

    auto* rc=new QWidget; auto* rcl=new QVBoxLayout(rc); rcl->setContentsMargins(0,0,0,0); rcl->setSpacing(8);
    auto* kr=new QHBoxLayout; kr->setSpacing(8);
    struct KP{QString ic,ti,va,cl;};
    const QVector<KP> kps={{"💹","Прибыль сегодня","+287 430 ₽","#34d399"},{"📉","Макс. просадка","−3.8%","#f87171"},{"🔄","Оборот (30 дн.)","18.4 млрд ₽","#93c5fd"},{"⭐","Шарп / Сортино","1.42 / 1.87","#fbbf24"}};
    for(const auto&k:kps){
        auto* card=new QWidget; card->setObjectName("kpiCard");
        auto* cl=new QVBoxLayout(card); cl->setContentsMargins(14,12,14,12); cl->setSpacing(3);
        auto* ti=new QLabel(k.ic+"  "+k.ti); ti->setObjectName("kpiTitle");
        auto* va=new QLabel(k.va); va->setStyleSheet(QString("color:%1;font-size:16px;font-weight:800;").arg(k.cl));
        cl->addWidget(ti); cl->addWidget(va); kr->addWidget(card,1);
    }
    rcl->addLayout(kr);
    auto* sl=new QLabel("📋  Сводка портфеля"); sl->setObjectName("sectionTitle"); rcl->addWidget(sl);
    auto* sc=new QScrollArea; sc->setWidgetResizable(true); sc->setObjectName("summaryScroll");
    sc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* scw=new QWidget; auto* scl=new QVBoxLayout(scw);
    scl->setContentsMargins(4,4,4,4); scl->setSpacing(5);
    for(const Asset&a:m_assets) scl->addWidget(new SummaryRowWidget(a));
    scl->addStretch(); sc->setWidget(scw); rcl->addWidget(sc,1);
    pl->addWidget(lc,0); pl->addWidget(rc,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildNewsPage()
{
    auto* page=new QWidget; auto* pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto* ti=new QLabel("📰  Новостная лента"); ti->setObjectName("pageTitle"); pl->addWidget(ti);
    auto* sc=new QScrollArea; sc->setWidgetResizable(true); sc->setObjectName("newsScroll");
    sc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* cw=new QWidget; auto* cl=new QVBoxLayout(cw);
    cl->setContentsMargins(4,4,4,4); cl->setSpacing(8);
    for(const NewsItem&n:m_news) cl->addWidget(new NewsCardWidget(n));
    cl->addStretch(); sc->setWidget(cw); pl->addWidget(sc,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildAnalyticsPage()
{
    auto* page=new QWidget; auto* pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto* ti=new QLabel("📊  Аналитика"); ti->setObjectName("pageTitle"); pl->addWidget(ti);
    auto* row=new QHBoxLayout; row->setSpacing(8);
    struct M{QString nm,va,sb,cl,ic;};
    const QVector<M> ms={{"Beta","0.87","vs MOEX","#93c5fd","β"},{"Alpha","+2.4%","годовых","#34d399","α"},{"VaR 95%","−4.2%","дневной","#f87171","⚡"},{"Корреляция","0.61","между акт.","#fbbf24","∞"},{"Просадка","−8.3%","12 мес.","#f87171","↓"},{"Волатильность","12.4%","год.","#fbbf24","~"}};
    for(const auto&m:ms){
        auto* card=new QWidget; card->setObjectName("analyticsCard");
        auto* cl=new QVBoxLayout(card); cl->setContentsMargins(14,12,14,12);
        auto* ic=new QLabel(m.ic); ic->setStyleSheet(QString("color:%1;font-size:22px;font-weight:800;").arg(m.cl));
        auto* nm=new QLabel(m.nm); nm->setObjectName("analyticsCardTitle");
        auto* va=new QLabel(m.va); va->setStyleSheet(QString("color:%1;font-size:20px;font-weight:800;").arg(m.cl));
        auto* sb=new QLabel(m.sb); sb->setObjectName("analyticsCardSub");
        cl->addWidget(ic); cl->addWidget(nm); cl->addWidget(va); cl->addWidget(sb); row->addWidget(card,1);
    }
    pl->addLayout(row);
    auto* al=new QLabel("  Аллокация по секторам"); al->setObjectName("sectionTitle"); pl->addWidget(al);
    m_allocBar=new AllocationBarWidget(m_assets,m_darkTheme); m_allocBar->setMinimumHeight(220);
    pl->addWidget(m_allocBar,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildCalendarPage()
{
    auto* page=new QWidget; auto* pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto* ti=new QLabel("📅  Дивидендный календарь"); ti->setObjectName("pageTitle"); pl->addWidget(ti);
    struct DV{QString tk,co,dt,am,yd;};
    const QVector<DV> evs={{"LKOH","Лукойл","15.07.2025","1 100 ₽/акц","8.4%"},{"SBER","Сбербанк","23.07.2025","33.5 ₽/акц","6.1%"},{"MGNT","Магнит","02.08.2025","412 ₽/акц","5.2%"},{"GAZP","Газпром","10.08.2025","25.0 ₽/акц","4.8%"},{"ROSN","Роснефть","28.08.2025","38.6 ₽/акц","4.1%"},{"GMKN","НорНикель","15.09.2025","780 ₽/акц","3.9%"},{"OFZ238","ОФЗ 26238","20.09.2025","37.9 ₽/бум","6.0%"}};
    auto* sc=new QScrollArea; sc->setWidgetResizable(true); sc->setObjectName("summaryScroll");
    auto* cw=new QWidget; auto* cl=new QVBoxLayout(cw); cl->setContentsMargins(4,4,4,4); cl->setSpacing(6);
    for(const auto&e:evs){
        auto* row=new QWidget; row->setObjectName("calRow"); row->setFixedHeight(56);
        auto* rl=new QHBoxLayout(row); rl->setContentsMargins(14,6,14,6);
        auto* bg=new QLabel(e.tk); bg->setAlignment(Qt::AlignCenter); bg->setFixedSize(70,32);
        bg->setStyleSheet("background:#4f6ef7;border-radius:8px;color:white;font-weight:800;font-size:11px;");
        auto* co=new QLabel(e.co); co->setObjectName("calCompany");
        auto* dt=new QLabel("📅 "+e.dt); dt->setObjectName("calDate");
        auto* am=new QLabel(e.am); am->setObjectName("calAmount");
        auto* yd=new QLabel(e.yd); yd->setObjectName("calYield");
        rl->addWidget(bg); rl->addSpacing(12); rl->addWidget(co,2); rl->addWidget(dt,2); rl->addWidget(am,2); rl->addWidget(yd,1);
        cl->addWidget(row);
    }
    cl->addStretch(); sc->setWidget(cw); pl->addWidget(sc,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildScreenerPage()
{
    auto* page=new QWidget; auto* pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto* ti=new QLabel("🔭  Скринер"); ti->setObjectName("pageTitle"); pl->addWidget(ti);
    auto* fr=new QHBoxLayout; fr->setSpacing(8);
    for(const QString&f:{"P/E < 15","Div > 5%","Кап > 500 млрд","ROE > 20%","Beta < 1"}){
        auto* chip=new QPushButton(f); chip->setObjectName("screenChip"); chip->setCheckable(true); fr->addWidget(chip);
    }
    fr->addStretch(); pl->addLayout(fr);
    struct SR{QString tk,nm;double pe,dv,roe,bt,cap;};
    const QVector<SR> rows={{"SBER","Сбербанк",5.1,6.1,23.4,0.85,6420},{"GAZP","Газпром",3.2,4.8,11.2,0.72,3150},{"LKOH","Лукойл",6.4,8.4,19.6,0.91,5200},{"YNDX","Яндекс",28.7,0.0,12.1,1.41,1820},{"GMKN","НорНикель",8.9,3.9,31.2,0.78,2780},{"ROSN","Роснефть",4.6,4.1,14.3,0.88,4100},{"MGNT","Магнит",11.2,5.2,22.8,0.95,1640}};
    auto* sc=new QScrollArea; sc->setWidgetResizable(true); sc->setObjectName("summaryScroll");
    auto* cw=new QWidget; auto* cl=new QVBoxLayout(cw); cl->setContentsMargins(4,4,4,4); cl->setSpacing(4);
    auto* hdr=new QWidget; hdr->setObjectName("tableHdrRow"); hdr->setFixedHeight(32);
    auto* hrl=new QHBoxLayout(hdr); hrl->setContentsMargins(14,0,14,0);
    for(const QString&h:{"Тикер","Название","P/E","Дивид.","ROE","Beta","Кап."}){
        auto* l=new QLabel(h); l->setObjectName("tableHdr"); hrl->addWidget(l,h=="Название"?2:1);
    }
    cl->addWidget(hdr);
    for(const auto&r:rows){
        auto* row=new QWidget; row->setObjectName("calRow"); row->setFixedHeight(44);
        auto* rl=new QHBoxLayout(row); rl->setContentsMargins(14,6,14,6);
        auto col=[&](const QString&t,int s,const QString&st){auto* l=new QLabel(t);l->setStyleSheet(st);rl->addWidget(l,s);};
        col(r.tk,1,"color:#93c5fd;font-size:12px;font-weight:800;");
        col(r.nm,2,"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.pe,'f',1),1,r.pe<15?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.dv,'f',1)+"%",1,r.dv>5?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.roe,'f',1)+"%",1,r.roe>20?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.bt,'f',2),1,r.bt<1?"color:#34d399;font-size:12px;":"color:#fbbf24;font-size:12px;");
        col(QString::number(r.cap,'f',0),1,"color:#9ca3af;font-size:12px;");
        cl->addWidget(row);
    }
    cl->addStretch(); sc->setWidget(cw); pl->addWidget(sc,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildSettingsPage()
{
    auto* page=new QWidget; auto* l=new QVBoxLayout(page);
    auto* lb=new QLabel("⚙  Настройки\n\nРаздел в разработке");
    lb->setObjectName("placeholderLabel"); lb->setAlignment(Qt::AlignCenter); l->addWidget(lb);
    m_pageStack->addWidget(page);
}

void MainWindow::populateAssetList()
{
    m_assetList->clear();
    for(int i=0;i<m_assets.size();++i){
        const Asset&a=m_assets[i];
        bool up=a.currentPrice>=a.prevPrice;
        double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
        auto* item=new QListWidgetItem(m_assetList);
        item->setData(Qt::UserRole,i);
        auto* w=new AssetRowWidget(a,up,pct,a.liveData);
        item->setSizeHint(w->sizeHint());
        m_assetList->setItemWidget(item,w);
    }
}

void MainWindow::showAssetDetail(int index)
{
    if(index<0||index>=m_assets.size()) return;
    if(m_detailPage){m_pageStack->removeWidget(m_detailPage);delete m_detailPage;m_detailPage=nullptr;}
    const Asset&a=m_assets[index];
    bool up=a.currentPrice>=a.prevPrice;
    double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;

    m_detailPage=new QWidget; auto* dl=new QVBoxLayout(m_detailPage);
    dl->setContentsMargins(10,10,10,10); dl->setSpacing(8);

    auto* hc=new QWidget; hc->setObjectName("detailHdrCard");
    auto* hcl=new QHBoxLayout(hc); hcl->setContentsMargins(16,12,16,12);
    auto* bb=new QPushButton("← Назад"); bb->setObjectName("backBtn");
    connect(bb,&QPushButton::clicked,[this]{switchPage(0);});
    auto* ic=new QLabel(a.ticker.left(2)); ic->setFixedSize(50,50); ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet(QString("background:%1;border-radius:25px;color:white;font-weight:800;font-size:14px;").arg(sectorColor(a.sector).name()));
    auto* nb=new QVBoxLayout;
    auto* nl=new QLabel(a.name); nl->setObjectName("detailName");
    auto* sl=new QLabel(a.sector+" · "+a.ticker); sl->setObjectName("detailSector");
    nb->addWidget(nl); nb->addWidget(sl);
    auto* lb=new QLabel(a.liveData?"🟢 Sandbox LIVE":"🟡 Симуляция");
    lb->setStyleSheet(a.liveData?"color:#34d399;font-size:10px;font-weight:700;":"color:#fbbf24;font-size:10px;font-weight:700;");
    auto* pb=new QVBoxLayout; pb->setAlignment(Qt::AlignRight);
    auto* pl=new QLabel(QString::number(a.currentPrice,'f',2)+" ₽");
    pl->setObjectName(up?"detailPriceUp":"detailPriceDown"); pl->setAlignment(Qt::AlignRight);
    auto* cl=new QLabel((up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%");
    cl->setObjectName(up?"changeUp":"changeDown"); cl->setAlignment(Qt::AlignRight);
    pb->addWidget(pl); pb->addWidget(cl);
    hcl->addWidget(bb); hcl->addSpacing(14); hcl->addWidget(ic); hcl->addSpacing(10);
    hcl->addLayout(nb); hcl->addSpacing(8); hcl->addWidget(lb); hcl->addStretch(); hcl->addLayout(pb);
    dl->addWidget(hc); dl->addWidget(buildKPIStrip(a));
    auto* ctl=new QLabel("  📈  Японские свечи · 120 дней"); ctl->setObjectName("sectionTitle"); dl->addWidget(ctl);
    auto* cc=new CandleChartWidget(a.candles); cc->setMinimumHeight(360); dl->addWidget(cc,1);
    m_pageStack->addWidget(m_detailPage); m_pageStack->setCurrentWidget(m_detailPage);
}

QWidget* MainWindow::buildKPIStrip(const Asset& a)
{
    bool up=a.currentPrice>=a.prevPrice;
    auto* w=new QWidget; w->setObjectName("kpiStrip");
    auto* l=new QHBoxLayout(w); l->setContentsMargins(0,0,0,0); l->setSpacing(6);
    struct KV{QString t,v,o;};
    const QVector<KV> items={
        {"Цена",QString::number(a.currentPrice,'f',2)+"₽",up?"kpiValUp":"kpiValDown"},
        {"Кол-во",QString::number(a.tickCount)+" шт.","kpiVal"},
        {"Ср. цена",QString::number(a.avgPrice,'f',2)+"₽","kpiVal"},
        {"Позиция",fmtS(a.currentPrice*a.tickCount)+"₽","kpiVal"},
        {"P/L",fmtRub(a.profit),a.profit>=0?"kpiValUp":"kpiValDown"},
        {"PRI",QString::number(a.personalRisk,'f',1)+"/10","kpiVal"},
        {"Сектор",a.sector,"kpiVal"},
    };
    for(const auto&kv:items){
        auto* card=new QWidget; card->setObjectName("kpiCard2");
        auto* cl=new QVBoxLayout(card); cl->setContentsMargins(12,8,12,8); cl->setSpacing(2);
        auto* t=new QLabel(kv.t); t->setObjectName("kpiTitle");
        auto* v=new QLabel(kv.v); v->setObjectName(kv.o);
        cl->addWidget(t); cl->addWidget(v); l->addWidget(card,1);
    }
    return w;
}

void MainWindow::switchPage(int idx)
{if(idx>=0&&idx<m_pageStack->count()) m_pageStack->setCurrentIndex(idx);}

void MainWindow::onAssetDoubleClicked(QListWidgetItem* item)
{if(item) showAssetDetail(item->data(Qt::UserRole).toInt());}

void MainWindow::toggleTheme()
{
    m_darkTheme=!m_darkTheme;
    m_themeBtn->setText(m_darkTheme?"☀  Тема":"🌙  Тема");
    if(m_pieChart) m_pieChart->setDark(m_darkTheme);
    if(m_allocBar) m_allocBar->setDark(m_darkTheme);
    applyTheme(); update();
}

// ================================================================
// ТЁМНАЯ ТЕМА — объекты названы по objectName, всё через QSS
// ================================================================
void MainWindow::applyTheme(){m_darkTheme?applyDarkTheme():applyLightTheme();}

void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(R"(
#centralWidget{background:#080c14;border-radius:18px;}
#headerWidget{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0e1526,stop:1 #0b1120);border-radius:14px;border:1px solid #1a2340;}
#logoLabel{color:#4f6ef7;font-size:18px;font-weight:800;letter-spacing:1px;}
#avatarLabel{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4f6ef7,stop:1 #8b5cf6);border-radius:24px;color:white;font-weight:800;font-size:16px;}
#accountLabel{color:#f0f4ff;font-size:15px;font-weight:700;}
#roleLabel{color:#4f6ef7;font-size:10px;}
#liveStatus{color:#34d399;font-size:11px;font-weight:700;padding:3px 8px;background:#34d39920;border-radius:8px;border:1px solid #34d39940;}
#statTitle{color:#3a4560;font-size:9px;letter-spacing:1px;}
#statGreen{color:#34d399;font-size:19px;font-weight:900;}
#statBlue{color:#60a5fa;font-size:19px;font-weight:900;}
#statBalance{color:#ffffff;font-size:16px;font-weight:900;}
#hdrSep{color:#1a2340;}
#iconBtn{background:#0e1526;border:1px solid #1a2340;border-radius:10px;color:#6b7280;font-size:16px;}
#iconBtn:hover{background:#1a2a44;color:#e8eaf6;}
#closeBtn{background:#0e1526;border:1px solid #1a2340;border-radius:10px;color:#4b5563;font-size:13px;}
#closeBtn:hover{background:#ef4444;color:white;border-color:#ef4444;}
QSplitter::handle{background:#1a2340;}
#leftPanel{background:#0c1020;border-radius:14px;border:1px solid #1a2340;}
#searchBar{background:#111828;border-radius:10px;border:1px solid #1a2340;}
#searchEdit{background:transparent;color:#e8eaf6;font-size:12px;}
#chipBtn{background:#111828;border:1px solid #1a2340;color:#4b5563;border-radius:10px;padding:2px 10px;font-size:10px;}
#chipBtn:hover{background:#1a2a44;color:#e8eaf6;border-color:#4f6ef7;}
#panelTitle{color:#2a3555;font-size:9px;font-weight:700;letter-spacing:2px;padding:2px 6px;}
#assetList{background:transparent;border:none;}
#assetList::item{border-radius:10px;margin:1px 2px;}
#assetList::item:selected{background:#111828;border:1px solid #1a2340;}
#assetList::item:hover{background:#0e1624;}
#assetTicker{color:#e8eaf6;font-weight:800;font-size:13px;}
#assetSector{color:#2a3555;font-size:9px;}
#assetMini{color:#2a3555;font-size:9px;}
#miniSummary{background:#111828;border-radius:10px;border:1px solid #1a2340;}
#miniLabel{color:#4b5563;font-size:11px;}
#miniValueGreen{color:#34d399;font-size:13px;font-weight:800;}
#miniValueRed{color:#f87171;font-size:13px;font-weight:800;}
#menuBarWidget{background:#0c1020;border-radius:12px;border:1px solid #1a2340;}
QPushButton[menuBtn="true"]{background:transparent;border:none;color:#4b5563;font-size:12px;padding:6px 14px;border-radius:8px;}
QPushButton[menuBtn="true"]:hover{background:#111828;color:#e8eaf6;}
#menuBtnTheme{background:#111828;border:1px solid #1a2340;color:#fbbf24;font-size:12px;padding:6px 14px;border-radius:8px;}
#menuBtnTheme:hover{background:#1a2a44;}
#menuSep{color:#1a2340;}
#pageStack{background:#0c1020;border-radius:14px;border:1px solid #1a2340;}
#card,#kpiCard,#analyticsCard{background:#111828;border-radius:12px;border:1px solid #1a2340;}
#cardTitle{color:#60a5fa;font-size:13px;font-weight:700;}
#cardText{color:#4b5563;font-size:11px;}
#kpiTitle{color:#2a3555;font-size:9px;letter-spacing:1px;}
#kpiCard2{background:#111828;border-radius:10px;border:1px solid #1a2340;}
#kpiStrip{background:transparent;}
#kpiVal{color:#e8eaf6;font-size:13px;font-weight:800;}
#kpiValUp{color:#34d399;font-size:13px;font-weight:800;}
#kpiValDown{color:#f87171;font-size:13px;font-weight:800;}
#analyticsCardTitle{color:#6b7280;font-size:10px;}
#analyticsCardSub{color:#2a3555;font-size:10px;}
#sectionTitle{color:#d0d8f8;font-size:14px;font-weight:800;padding:4px 8px;}
#pageTitle{color:#d0d8f8;font-size:17px;font-weight:900;padding:4px 8px;}
#placeholderLabel{color:#1a2340;font-size:28px;}
QScrollArea{background:transparent;border:none;}
#summaryScroll,#newsScroll{background:transparent;border:none;}
QScrollBar:vertical{background:#0c1020;width:5px;border-radius:3px;}
QScrollBar::handle:vertical{background:#1a2340;border-radius:3px;min-height:24px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#detailHdrCard{background:#111828;border-radius:12px;border:1px solid #1a2340;}
#backBtn{background:transparent;border:1px solid #1a2340;color:#4b5563;padding:7px 14px;border-radius:8px;font-size:12px;}
#backBtn:hover{background:#1a2a44;color:#e8eaf6;}
#detailName{color:#f0f4ff;font-size:20px;font-weight:900;}
#detailSector{color:#4b5563;font-size:11px;}
#detailPriceUp{color:#34d399;font-size:24px;font-weight:900;}
#detailPriceDown{color:#f87171;font-size:24px;font-weight:900;}
#changeUp{color:#34d399;font-size:13px;font-weight:800;}
#changeDown{color:#f87171;font-size:13px;font-weight:800;}
#calRow{background:#111828;border-radius:10px;border:1px solid #1a2340;}
#calCompany{color:#e8eaf6;font-size:12px;font-weight:600;}
#calDate{color:#4b5563;font-size:11px;}
#calAmount{color:#34d399;font-size:12px;font-weight:800;}
#calYield{color:#fbbf24;font-size:12px;font-weight:800;}
#tableHdrRow{background:#080c14;border-radius:6px;}
#tableHdr{color:#2a3555;font-size:10px;font-weight:700;letter-spacing:1px;}
#screenChip{background:#111828;border:1px solid #1a2340;color:#4b5563;border-radius:10px;padding:4px 12px;font-size:11px;}
#screenChip:hover{background:#1a2a44;color:#e8eaf6;border-color:#4f6ef7;}
#screenChip:checked{background:#4f6ef720;color:#4f6ef7;border-color:#4f6ef7;}
)");
}

void MainWindow::applyLightTheme()
{
    qApp->setStyleSheet(R"(
#centralWidget{background:#eef2ff;border-radius:18px;}
#headerWidget{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #ffffff,stop:1 #f8faff);border-radius:14px;border:1px solid #d1d9f0;}
#logoLabel{color:#4f6ef7;font-size:18px;font-weight:800;letter-spacing:1px;}
#avatarLabel{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4f6ef7,stop:1 #8b5cf6);border-radius:24px;color:white;font-weight:800;font-size:16px;}
#accountLabel{color:#1e293b;font-size:15px;font-weight:700;}
#roleLabel{color:#4f6ef7;font-size:10px;}
#liveStatus{color:#059669;font-size:11px;font-weight:700;padding:3px 8px;background:#05966920;border-radius:8px;border:1px solid #05966940;}
#statTitle{color:#94a3b8;font-size:9px;letter-spacing:1px;}
#statGreen{color:#059669;font-size:19px;font-weight:900;}
#statBlue{color:#3b82f6;font-size:19px;font-weight:900;}
#statBalance{color:#1e293b;font-size:16px;font-weight:900;}
#hdrSep{color:#d1d9f0;}
#iconBtn{background:#f1f5f9;border:1px solid #d1d9f0;border-radius:10px;color:#64748b;font-size:16px;}
#iconBtn:hover{background:#e8eeff;color:#1e293b;}
#closeBtn{background:#f1f5f9;border:1px solid #d1d9f0;border-radius:10px;color:#64748b;font-size:13px;}
#closeBtn:hover{background:#ef4444;color:white;border-color:#ef4444;}
QSplitter::handle{background:#d1d9f0;}
#leftPanel{background:#ffffff;border-radius:14px;border:1px solid #d1d9f0;}
#searchBar{background:#f1f5f9;border-radius:10px;border:1px solid #d1d9f0;}
#searchEdit{background:transparent;color:#1e293b;font-size:12px;}
#chipBtn{background:#f1f5f9;border:1px solid #d1d9f0;color:#64748b;border-radius:10px;padding:2px 10px;font-size:10px;}
#chipBtn:hover{background:#eef2ff;color:#1e293b;border-color:#4f6ef7;}
#panelTitle{color:#94a3b8;font-size:9px;font-weight:700;letter-spacing:2px;padding:2px 6px;}
#assetList{background:transparent;border:none;}
#assetList::item{border-radius:10px;margin:1px 2px;}
#assetList::item:selected{background:#eef2ff;border:1px solid #d1d9f0;}
#assetList::item:hover{background:#f5f7ff;}
#assetTicker{color:#1e293b;font-weight:800;font-size:13px;}
#assetSector{color:#64748b;font-size:9px;}
#assetMini{color:#94a3b8;font-size:9px;}
#miniSummary{background:#f1f5f9;border-radius:10px;border:1px solid #d1d9f0;}
#miniLabel{color:#64748b;font-size:11px;}
#miniValueGreen{color:#059669;font-size:13px;font-weight:800;}
#miniValueRed{color:#dc2626;font-size:13px;font-weight:800;}
#menuBarWidget{background:#ffffff;border-radius:12px;border:1px solid #d1d9f0;}
QPushButton[menuBtn="true"]{background:transparent;border:none;color:#64748b;font-size:12px;padding:6px 14px;border-radius:8px;}
QPushButton[menuBtn="true"]:hover{background:#eef2ff;color:#1e293b;}
#menuBtnTheme{background:#fefce8;border:1px solid #fde68a;color:#d97706;font-size:12px;padding:6px 14px;border-radius:8px;}
#menuBtnTheme:hover{background:#fef9c3;}
#menuSep{color:#d1d9f0;}
#pageStack{background:#ffffff;border-radius:14px;border:1px solid #d1d9f0;}
#card,#kpiCard,#analyticsCard{background:#f8faff;border-radius:12px;border:1px solid #d1d9f0;}
#cardTitle{color:#3b82f6;font-size:13px;font-weight:700;}
#cardText{color:#64748b;font-size:11px;}
#kpiTitle{color:#94a3b8;font-size:9px;letter-spacing:1px;}
#kpiCard2{background:#f8faff;border-radius:10px;border:1px solid #d1d9f0;}
#kpiStrip{background:transparent;}
#kpiVal{color:#1e293b;font-size:13px;font-weight:800;}
#kpiValUp{color:#059669;font-size:13px;font-weight:800;}
#kpiValDown{color:#dc2626;font-size:13px;font-weight:800;}
#analyticsCardTitle{color:#64748b;font-size:10px;}
#analyticsCardSub{color:#94a3b8;font-size:10px;}
#sectionTitle{color:#1e293b;font-size:14px;font-weight:800;padding:4px 8px;}
#pageTitle{color:#1e293b;font-size:17px;font-weight:900;padding:4px 8px;}
#placeholderLabel{color:#cbd5e1;font-size:28px;}
QScrollArea{background:transparent;border:none;}
#summaryScroll,#newsScroll{background:transparent;border:none;}
QScrollBar:vertical{background:#f1f5f9;width:5px;border-radius:3px;}
QScrollBar::handle:vertical{background:#cbd5e1;border-radius:3px;min-height:24px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#detailHdrCard{background:#f8faff;border-radius:12px;border:1px solid #d1d9f0;}
#backBtn{background:transparent;border:1px solid #d1d9f0;color:#64748b;padding:7px 14px;border-radius:8px;font-size:12px;}
#backBtn:hover{background:#eef2ff;color:#1e293b;}
#detailName{color:#1e293b;font-size:20px;font-weight:900;}
#detailSector{color:#64748b;font-size:11px;}
#detailPriceUp{color:#059669;font-size:24px;font-weight:900;}
#detailPriceDown{color:#dc2626;font-size:24px;font-weight:900;}
#changeUp{color:#059669;font-size:13px;font-weight:800;}
#changeDown{color:#dc2626;font-size:13px;font-weight:800;}
#calRow{background:#f8faff;border-radius:10px;border:1px solid #d1d9f0;}
#calCompany{color:#1e293b;font-size:12px;font-weight:600;}
#calDate{color:#64748b;font-size:11px;}
#calAmount{color:#059669;font-size:12px;font-weight:800;}
#calYield{color:#d97706;font-size:12px;font-weight:800;}
#tableHdrRow{background:#eef2ff;border-radius:6px;}
#tableHdr{color:#94a3b8;font-size:10px;font-weight:700;letter-spacing:1px;}
#screenChip{background:#f1f5f9;border:1px solid #d1d9f0;color:#64748b;border-radius:10px;padding:4px 12px;font-size:11px;}
#screenChip:hover{background:#eef2ff;color:#1e293b;border-color:#4f6ef7;}
#screenChip:checked{background:#eef2ff;color:#4f6ef7;border-color:#4f6ef7;}
)");
}

// ================================================================
// paintEvent / drag
// ================================================================
void MainWindow::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addRoundedRect(rect(),18,18);
    QLinearGradient g(0,0,0,height());
    if(m_darkTheme){g.setColorAt(0,QColor("#080c14"));g.setColorAt(1,QColor("#050810"));}
    else{g.setColorAt(0,QColor("#eef2ff"));g.setColorAt(1,QColor("#e8edff"));}
    p.fillPath(path,g);
}
void MainWindow::mousePressEvent(QMouseEvent* e)
{
    if(e->button()==Qt::LeftButton&&e->pos().y()<80){m_dragging=true;m_dragPos=e->globalPosition().toPoint()-frameGeometry().topLeft();}
    QMainWindow::mousePressEvent(e);
}
void MainWindow::mouseMoveEvent(QMouseEvent* e)
{
    if(m_dragging&&(e->buttons()&Qt::LeftButton)) move(e->globalPosition().toPoint()-m_dragPos);
    QMainWindow::mouseMoveEvent(e);
}
void MainWindow::mouseReleaseEvent(QMouseEvent* e){m_dragging=false;QMainWindow::mouseReleaseEvent(e);}

// ================================================================
// AssetRowWidget — цвет тикера/сектора через QSS (objectName)
// зелёный/красный цвет цены всегда одинаков в обеих темах
// ================================================================
AssetRowWidget::AssetRowWidget(const Asset& a,bool up,double pct,bool live,QWidget* parent)
    :QWidget(parent)
{
    setFixedHeight(64);
    auto* l=new QHBoxLayout(this); l->setContentsMargins(8,6,8,6); l->setSpacing(6);
    auto* ic=new QLabel(a.ticker.left(2)); ic->setFixedSize(34,34); ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet(QString("background:%1;border-radius:17px;color:white;font-weight:800;font-size:10px;").arg(sectorColor(a.sector).name()));
    auto* cv=new QVBoxLayout; cv->setSpacing(2);
    auto* r1=new QHBoxLayout;
    auto* tl=new QLabel(a.ticker); tl->setObjectName("assetTicker");
    auto* sl=new QLabel(a.sector); sl->setObjectName("assetSector");
    r1->addWidget(tl);
    if(live){auto* dot=new QLabel("●");dot->setStyleSheet("color:#34d399;font-size:8px;margin-left:2px;");r1->addWidget(dot);}
    r1->addSpacing(4); r1->addWidget(sl); r1->addStretch();
    QString mini=QString("TC:%1 | CP:%2 | PL:%3 | PRI:%4")
        .arg(a.tickCount).arg(QString::number(a.currentPrice,'f',1))
        .arg(a.profit>=0?"+"+fmtS(a.profit):fmtS(a.profit))
        .arg(QString::number(a.personalRisk,'f',1));
    auto* ml=new QLabel(mini); ml->setObjectName("assetMini");
    cv->addLayout(r1); cv->addWidget(ml);
    // Цена — зелёный/красный фиксирован, не меняется от темы
    const QString clr=up?"#34d399":"#f87171";
    auto* rv=new QVBoxLayout; rv->setSpacing(1); rv->setAlignment(Qt::AlignRight);
    auto* pl=new QLabel(QString::number(a.currentPrice,'f',1));
    pl->setStyleSheet(QString("color:%1;font-size:13px;font-weight:800;").arg(clr)); pl->setAlignment(Qt::AlignRight);
    auto* pc=new QLabel((up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%");
    pc->setStyleSheet(QString("color:%1;font-size:10px;font-weight:700;").arg(clr)); pc->setAlignment(Qt::AlignRight);
    rv->addWidget(pl); rv->addWidget(pc);
    l->addWidget(ic); l->addLayout(cv,1); l->addLayout(rv);
}

// ================================================================
// PieChartWidget
// ================================================================
PieChartWidget::PieChartWidget(const QVector<Asset>& a,bool dark,QWidget* parent)
    :QWidget(parent),m_assets(a),m_dark(dark)
{
    setMouseTracking(true); setMinimumSize(300,300);
    m_palette={{79,110,247},{139,92,246},{52,211,153},{248,159,49},{248,114,114},{6,182,212},{251,191,36},{236,72,153},{107,114,128},{34,211,238}};
}
void PieChartWidget::setAssets(const QVector<Asset>& a){m_assets=a;update();}
void PieChartWidget::setDark(bool d){m_dark=d;update();}
void PieChartWidget::paintEvent(QPaintEvent*)
{
    if(m_assets.isEmpty()) return;
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    double tot=0; for(const auto&a:m_assets) tot+=a.currentPrice*a.tickCount;
    if(tot<=0) return;
    int cx=width()/2,cy=height()/2,R=qMin(cx,cy)-28,iR=R*52/100;
    double sa=-90.0*16;
    for(int i=0;i<m_assets.size();++i){
        double share=(m_assets[i].currentPrice*m_assets[i].tickCount)/tot;
        int span=qRound(share*360*16);
        QColor col=m_palette[i%m_palette.size()];
        bool hov=(i==m_hovered);
        int off=hov?10:0;
        double mid=(sa/16.0+span/32.0)*M_PI/180.0;
        int ox=hov?qRound(cos(mid)*off):0, oy=hov?qRound(sin(mid)*off):0;
        p.setBrush(hov?col.lighter(120):col); p.setPen(hov?QPen(Qt::white,2):Qt::NoPen);
        p.drawPie(cx-R+ox,cy-R+oy,R*2,R*2,(int)sa,span); sa+=span;
    }
    QColor bg1=m_dark?QColor("#131929"):QColor("#f8faff");
    QColor bg2=m_dark?QColor("#0e1526"):QColor("#eef2ff");
    QRadialGradient hole(cx,cy,iR); hole.setColorAt(0,bg1); hole.setColorAt(1,bg2);
    p.setBrush(hole); p.setPen(Qt::NoPen); p.drawEllipse(cx-iR,cy-iR,iR*2,iR*2);
    QColor tp=m_dark?QColor("#e8eaf6"):QColor("#1e293b");
    QColor ts=m_dark?QColor("#4b5563"):QColor("#64748b");
    if(m_hovered>=0&&m_hovered<m_assets.size()){
        const Asset&a=m_assets[m_hovered];
        double share=(a.currentPrice*a.tickCount)/tot*100.0;
        bool up=a.currentPrice>=a.prevPrice;
        double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
        QColor ac=m_palette[m_hovered%m_palette.size()];
        p.setFont(QFont("Arial",13,QFont::Bold)); p.setPen(ac);
        p.drawText(QRect(cx-60,cy-38,120,22),Qt::AlignCenter,a.ticker);
        p.setFont(QFont("Arial",8)); p.setPen(ts);
        p.drawText(QRect(cx-60,cy-16,120,16),Qt::AlignCenter,a.sector);
        p.setFont(QFont("Arial",10,QFont::Bold)); p.setPen(tp);
        p.drawText(QRect(cx-60,cy+2,120,18),Qt::AlignCenter,QString::number(share,'f',1)+"% портфеля");
        p.setFont(QFont("Arial",9,QFont::Bold)); p.setPen(up?QColor("#34d399"):QColor("#f87171"));
        p.drawText(QRect(cx-60,cy+22,120,16),Qt::AlignCenter,(up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%");
    } else {
        p.setFont(QFont("Arial",11,QFont::Bold)); p.setPen(tp);
        p.drawText(QRect(cx-70,cy-18,140,20),Qt::AlignCenter,QString::number(tot/1e9,'f',2)+" млрд ₽");
        p.setFont(QFont("Arial",8)); p.setPen(ts);
        p.drawText(QRect(cx-70,cy+4,140,16),Qt::AlignCenter,"Портфель · "+QString::number(m_assets.size())+" активов");
    }
    int ly=cy+R+12; if(ly+m_assets.size()*17>height()-4) ly=4;
    p.setFont(QFont("Arial",8));
    for(int i=0;i<m_assets.size();++i){
        int lx=(i<5)?6:width()/2+4; int y=ly+(i%5)*17;
        QColor c=m_palette[i%m_palette.size()];
        QPainterPath dot; dot.addRoundedRect(lx,y+1,10,10,3,3); p.fillPath(dot,c);
        double share=(m_assets[i].currentPrice*m_assets[i].tickCount)/tot*100.0;
        p.setPen(i==m_hovered?tp:ts);
        p.drawText(lx+14,y,150,13,Qt::AlignLeft|Qt::AlignVCenter,m_assets[i].ticker+"  "+QString::number(share,'f',1)+"%");
    }
}
void PieChartWidget::mouseMoveEvent(QMouseEvent* e)
{
    if(m_assets.isEmpty()) return;
    double tot=0; for(const auto&a:m_assets) tot+=a.currentPrice*a.tickCount;
    if(tot<=0) return;
    int cx=width()/2,cy=height()/2,R=qMin(cx,cy)-28,iR=R*52/100;
    QPointF pt=e->position(); double dx=pt.x()-cx,dy=pt.y()-cy,dist=sqrt(dx*dx+dy*dy);
    if(dist<iR||dist>R+12){if(m_hovered!=-1){m_hovered=-1;update();}return;}
    double angle=atan2(dy,dx)*180.0/M_PI+90.0; if(angle<0) angle+=360.0;
    double sa=0; int nh=-1;
    for(int i=0;i<m_assets.size();++i){
        double span=(m_assets[i].currentPrice*m_assets[i].tickCount)/tot*360.0;
        if(angle>=sa&&angle<sa+span){nh=i;break;} sa+=span;
    }
    if(nh!=m_hovered){m_hovered=nh;update();}
}
void PieChartWidget::leaveEvent(QEvent*){if(m_hovered!=-1){m_hovered=-1;update();}}

// ================================================================
// RiskGaugeWidget
// ================================================================
RiskGaugeWidget::RiskGaugeWidget(double v,double mx,QWidget* p):QWidget(p),m_value(v),m_max(mx){}
void RiskGaugeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w=width(),h=height(),bH=10,bY=h/2-bH/2-10,bW=w-16,bX=8;
    QPainterPath bg; bg.addRoundedRect(bX,bY,bW,bH,5,5); p.fillPath(bg,QColor("#1a2340"));
    double ratio=qBound(0.0,m_value/m_max,1.0); int fW=qRound(bW*ratio);
    if(fW>0){
        QLinearGradient g(bX,0,bX+bW,0);
        g.setColorAt(0.0,QColor("#34d399")); g.setColorAt(0.35,QColor("#fbbf24"));
        g.setColorAt(0.7,QColor("#f97316")); g.setColorAt(1.0,QColor("#ef4444"));
        QPainterPath f; f.addRoundedRect(bX,bY,fW,bH,5,5); p.fillPath(f,g);
    }
    int mx=bX+qRound(bW*ratio); p.setBrush(Qt::white); p.setPen(QPen(QColor("#1a2340"),2)); p.drawEllipse(mx-7,bY-3,14,bH+6);
    const QStringList zn={"Мин","Низкий","Средний","Высокий","Экстрем"};
    const QStringList zc={"#34d399","#86efac","#fbbf24","#f97316","#ef4444"};
    p.setFont(QFont("Arial",7));
    for(int i=0;i<=4;++i){double nx=bX+bW*i/4.0;p.setPen(QColor(zc[i]));p.drawText((int)nx-18,bY+bH+8,36,12,Qt::AlignCenter,zn[i]);}
    p.setFont(QFont("Arial",16,QFont::Bold)); p.setPen(QColor("#e8eaf6"));
    p.drawText(0,0,w,bY-2,Qt::AlignCenter,QString::number(m_value,'f',2)+" / "+QString::number(m_max,'f',0));
}

// ================================================================
// AllocationBarWidget
// ================================================================
AllocationBarWidget::AllocationBarWidget(const QVector<Asset>& a,bool dark,QWidget* p)
    :QWidget(p),m_assets(a),m_dark(dark){}
void AllocationBarWidget::setAssets(const QVector<Asset>& a){m_assets=a;}
void AllocationBarWidget::setDark(bool d){m_dark=d;update();}
void AllocationBarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QMap<QString,double> sec; double tot=0;
    for(const auto&a:m_assets){sec[a.sector]+=a.currentPrice*a.tickCount;tot+=a.currentPrice*a.tickCount;}
    if(tot<=0) return;
    QColor lc=m_dark?QColor("#6b7280"):QColor("#475569");
    QColor tc=m_dark?QColor("#111828"):QColor("#e2e8f0");
    QColor vc=m_dark?QColor("#e8eaf6"):QColor("#1e293b");
    int bH=26,bG=10,lW=110,vW=120,bW=width()-lW-vW-24,x0=lW+8,y=10;
    p.setFont(QFont("Arial",10,QFont::Bold));
    for(auto it=sec.begin();it!=sec.end();++it){
        double r=it.value()/tot; int fW=qRound(bW*r);
        p.setPen(lc); p.drawText(0,y,lW,bH,Qt::AlignRight|Qt::AlignVCenter,it.key());
        QPainterPath bg; bg.addRoundedRect(x0,y+4,bW,bH-8,4,4); p.fillPath(bg,tc);
        if(fW>6){
            QColor col=sectorColor(it.key()); QLinearGradient g(x0,0,x0+bW,0);
            g.setColorAt(0,col.lighter(120)); g.setColorAt(1,col.darker(130));
            QPainterPath f; f.addRoundedRect(x0,y+4,fW,bH-8,4,4); p.fillPath(f,g);
        }
        p.setPen(vc);
        p.drawText(x0+bW+8,y,vW,bH,Qt::AlignLeft|Qt::AlignVCenter,QString::number(r*100,'f',1)+"%  "+fmtS(it.value())+"₽");
        y+=bH+bG;
    }
}

// ================================================================
// CandleChartWidget
// ================================================================
CandleChartWidget::CandleChartWidget(const QVector<CandleData>& c,QWidget* p)
    :QWidget(p),m_candles(c)
{setMinimumSize(400,280);setMouseTracking(true);m_viewStart=qMax(0,c.size()-100);}
void CandleChartWidget::paintEvent(QPaintEvent*)
{
    if(m_candles.isEmpty()) return;
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QPainterPath bg; bg.addRoundedRect(rect(),12,12);
    QLinearGradient bgG(0,0,0,height()); bgG.setColorAt(0,QColor("#111828")); bgG.setColorAt(1,QColor("#0c1020"));
    p.fillPath(bg,bgG);
    const int PL=64,PR=14,PT=20,PB=36; int cw=width()-PL-PR,ch=height()-PT-PB;
    QVector<CandleData> vis=m_candles.mid(m_viewStart); if(vis.isEmpty()) return;
    double mn=1e18,mx=-1e18; for(const auto&c:vis){mn=qMin(mn,c.low);mx=qMax(mx,c.high);}
    double rng=mx-mn; if(rng<1e-9) rng=1;
    auto py=[&](double price)->int{return PT+ch-(int)((price-mn)/rng*ch);};
    p.setFont(QFont("Arial",7));
    for(int i=0;i<=6;++i){double pr=mn+rng*i/6.0;int y=py(pr);
        p.setPen(QPen(QColor("#1a2340"),1,Qt::DotLine)); p.drawLine(PL,y,PL+cw,y);
        p.setPen(QColor("#2a3555")); p.drawText(0,y-8,PL-4,16,Qt::AlignRight|Qt::AlignVCenter,QString::number(pr,'f',1));}
    int n=vis.size(); double cW=(double)cw/n,bW=qMax(1.5,cW*0.68);
    for(int i=0;i<n;++i){
        const CandleData&c=vis[i]; bool bull=c.close>=c.open;
        QColor body=bull?QColor("#34d399"):QColor("#f87171");
        QColor wick=bull?QColor("#22c47a"):QColor("#e05555");
        double cx2=PL+(i+0.5)*cW; int oY=py(c.open),cY=py(c.close),hY=py(c.high),lY=py(c.low);
        p.setPen(QPen(wick,1.2)); p.drawLine((int)cx2,hY,(int)cx2,lY);
        int bT=qMin(oY,cY),bH=qMax(1,qAbs(cY-oY)); int bx=(int)(cx2-bW/2);
        if(bH>2){QPainterPath bp;bp.addRoundedRect(bx,bT,(int)bW,bH,2,2);p.fillPath(bp,body);}
        else p.fillRect(bx,bT,(int)bW,qMax(2,bH),body);
    }
    int step=qMax(1,n/8);
    for(int i=0;i<n;i+=step){double cx2=PL+(i+0.5)*cW;p.setPen(QColor("#2a3555"));p.drawText((int)cx2-22,PT+ch+4,44,18,Qt::AlignCenter,vis[i].date.toString("dd.MM"));}
    if(m_hoverIdx>=0&&m_hoverIdx<n){
        double cx2=PL+(m_hoverIdx+0.5)*cW;
        p.setPen(QPen(QColor("#4f6ef760"),1,Qt::DashLine)); p.drawLine((int)cx2,PT,(int)cx2,PT+ch);
        const CandleData&hc=vis[m_hoverIdx]; int cY=py(hc.close);
        p.drawLine(PL,cY,PL+cw,cY);
        bool hBull=hc.close>=hc.open;
        QString tip=QString(" %1 O:%2 H:%3 L:%4 C:%5 V:%6K ").arg(hc.date.toString("dd.MM")).arg(hc.open,0,'f',1).arg(hc.high,0,'f',1).arg(hc.low,0,'f',1).arg(hc.close,0,'f',1).arg(hc.volume/1000);
        p.setFont(QFont("Arial",8,QFont::Bold)); QFontMetrics fm(p.font()); int tw=fm.horizontalAdvance(tip)+8;
        int tx=qBound(PL,(int)cx2-tw/2,PL+cw-tw);
        QRect tr(tx,PT,tw,18); QPainterPath tbg; tbg.addRoundedRect(tr,5,5);
        p.fillPath(tbg,QColor("#1a2a44ee")); p.setPen(hBull?QColor("#34d399"):QColor("#f87171")); p.drawText(tr,Qt::AlignCenter,tip);
        QRect ym(0,cY-9,PL-2,18); p.fillRect(ym,QColor("#4f6ef7")); p.setPen(Qt::white); p.drawText(ym,Qt::AlignCenter,QString::number(hc.close,'f',1));
    }
    p.setPen(QPen(QColor("#1a2340"),1)); p.setBrush(Qt::NoBrush); p.drawPath(bg);
}
void CandleChartWidget::mouseMoveEvent(QMouseEvent* e)
{int n=m_candles.size()-m_viewStart;if(n<=0)return;const int PL=64,PR=14;int cw=width()-PL-PR;double cW=(double)cw/n;if(cW<=0)return;m_hoverIdx=qBound(0,(int)((e->pos().x()-PL)/cW),n-1);update();}
void CandleChartWidget::leaveEvent(QEvent*){m_hoverIdx=-1;update();}

// ================================================================
// TickerBarWidget
// ================================================================
TickerBarWidget::TickerBarWidget(const QVector<Asset>& a,QWidget* p)
    :QWidget(p),m_assets(a)
{
    setFixedHeight(26);
    m_timer=new QTimer(this);
    connect(m_timer,&QTimer::timeout,[this]{m_offset-=1;int tot=m_assets.size()*180;if(tot>0&&-m_offset>tot)m_offset=0;update();});
    m_timer->start(16);
}
void TickerBarWidget::setAssets(const QVector<Asset>& a){m_assets=a;}
void TickerBarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QPainterPath bg; bg.addRoundedRect(rect(),8,8); p.fillPath(bg,QColor("#0c1020"));
    if(m_assets.isEmpty()) return;
    p.setFont(QFont("Arial",9,QFont::Bold)); int x=m_offset;
    for(int rep=0;rep<3;++rep){
        for(const auto&a:m_assets){
            bool up=a.currentPrice>=a.prevPrice;
            double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
            p.setPen(QColor("#4f6ef7")); QString tk="  "+a.ticker+"  ";
            int tw=p.fontMetrics().horizontalAdvance(tk); p.drawText(x,0,tw,height(),Qt::AlignVCenter,tk); x+=tw;
            p.setPen(up?QColor("#34d399"):QColor("#f87171"));
            QString rest=QString("%1  %2%3%  ·").arg(QString::number(a.currentPrice,'f',2)).arg(up?"▲+":"▼").arg(pct,0,'f',2);
            int rw=p.fontMetrics().horizontalAdvance(rest); p.drawText(x,0,rw,height(),Qt::AlignVCenter,rest); x+=rw+6;
        }
    }
}

// ================================================================
// SummaryRowWidget
// ================================================================
SummaryRowWidget::SummaryRowWidget(const Asset& a,QWidget* p):QWidget(p)
{
    setFixedHeight(50); setObjectName("calRow");
    auto* l=new QHBoxLayout(this); l->setContentsMargins(12,6,12,6); l->setSpacing(0);
    bool up=a.currentPrice>=a.prevPrice;
    double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
    auto col=[&](const QString&t,int s,const QString&st,Qt::Alignment al=Qt::AlignLeft|Qt::AlignVCenter){
        auto* lb=new QLabel(t); lb->setStyleSheet(st); lb->setAlignment(al); l->addWidget(lb,s);};
    col(a.ticker,1,"color:#93c5fd;font-size:11px;font-weight:800;");
    col(a.name,2,"color:#6b7280;font-size:11px;");
    col(QString::number(a.tickCount),1,"color:#e8eaf6;font-size:11px;",Qt::AlignCenter);
    col(QString::number(a.currentPrice,'f',1)+" ₽",2,QString("color:%1;font-size:12px;font-weight:700;").arg(up?"#34d399":"#f87171"),Qt::AlignRight|Qt::AlignVCenter);
    col((up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%",1,QString("color:%1;font-size:11px;font-weight:700;").arg(up?"#34d399":"#f87171"),Qt::AlignRight|Qt::AlignVCenter);
    col(fmtRub(a.profit),2,QString("color:%1;font-size:11px;").arg(a.profit>=0?"#34d399":"#f87171"),Qt::AlignRight|Qt::AlignVCenter);
}

// ================================================================
// NewsCardWidget
// ================================================================
NewsCardWidget::NewsCardWidget(const NewsItem& n,QWidget* p):QWidget(p)
{
    setObjectName("newsCard");
    setStyleSheet("QWidget#newsCard{background:#111828;border-radius:12px;border-left:4px solid #4f6ef7;}");
    auto* l=new QVBoxLayout(this); l->setContentsMargins(16,10,16,10); l->setSpacing(5);
    auto* hdr=new QHBoxLayout;
    auto* src=new QLabel("📰  "+n.source); src->setStyleSheet("color:#4f6ef7;font-size:11px;font-weight:800;");
    auto* tick=new QLabel("["+n.relatedTicker+"]"); tick->setStyleSheet("color:#34d399;font-size:10px;font-weight:700;");
    auto* date=new QLabel(n.date.toString("dd MMMM yyyy")); date->setStyleSheet("color:#2a3555;font-size:10px;"); date->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    hdr->addWidget(src); hdr->addSpacing(8); hdr->addWidget(tick); hdr->addStretch(); hdr->addWidget(date);
    auto* ti=new QLabel(n.title); ti->setStyleSheet("color:#e8eaf6;font-size:13px;font-weight:700;"); ti->setWordWrap(true);
    auto* bo=new QLabel(n.body); bo->setStyleSheet("color:#4b5563;font-size:11px;"); bo->setWordWrap(true);
    l->addLayout(hdr); l->addWidget(ti); l->addWidget(bo);
}
