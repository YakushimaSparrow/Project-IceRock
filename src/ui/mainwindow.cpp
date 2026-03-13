#include "mainwindow.h"
#include <QApplication>
#include <QScrollBar>

static QVector<CandleData> makeCandles(double startPrice, int count, double vol, quint32 seed)
{
    QVector<CandleData> out;
    double price = startPrice;
    QDate date = QDate::currentDate().addDays(-count);
    QRandomGenerator rng(seed);
    for (int i = 0; i < count; ++i) {
        double open  = price;
        double trend = (rng.generateDouble() - 0.47) * vol * price;
        double close = open + trend;
        double hi    = qMax(open, close) + rng.generateDouble() * vol * price * 0.3;
        double lo    = qMin(open, close) - rng.generateDouble() * vol * price * 0.3;
        lo = qMax(lo, price * 0.5);
        long long v  = (long long)(rng.bounded(5000000) + 100000);
        out.append({date, open, hi, lo, close, v});
        price = close;
        date = date.addDays(1);
        while (date.dayOfWeek() > 5) date = date.addDays(1);
    }
    return out;
}

static QVector<Asset> generateAssets()
{
    QVector<Asset> list;
    auto make = [&](const QString &tk, const QString &nm, const QString &sec,
                    int tc, double avg, double vol, double pri, double sp, quint32 seed)
    {
        Asset a;
        a.ticker = tk; a.name = nm; a.sector = sec;
        a.tickCount = tc; a.avgPrice = avg; a.personalRisk = pri;
        a.candles = makeCandles(sp, 120, vol, seed);
        a.currentPrice = a.candles.last().close;
        a.prevPrice    = a.candles[a.candles.size()-2].close;
        a.profit = (a.currentPrice - avg) * tc;
        return a;
    };
    list << make("SBER",   "Сбербанк",    "Финансы",    1200, 268.5, 0.022, 1.8,  240.0,  42);
    list << make("GAZP",   "Газпром",     "Энергетика",  800, 154.2, 0.025, 2.4,  148.0,  77);
    list << make("LKOH",   "Лукойл",      "Нефть/газ",   300,6820.0, 0.018, 2.1, 6500.0,  99);
    list << make("YNDX",   "Яндекс",      "Технологии",  450,3250.0, 0.032, 3.6, 3000.0, 123);
    list << make("GMKN",   "НорНикель",   "Металлы",     180,15400.0,0.020, 3.1,16000.0, 156);
    list << make("ROSN",   "Роснефть",    "Нефть/газ",   650, 520.0, 0.021, 2.3,  500.0, 200);
    list << make("GOLD",   "Золото",      "Сырьё",        50,5620.0, 0.015, 1.4, 5400.0, 211);
    list << make("USDRUB", "Доллар США",  "Валюта",     2000,  84.2, 0.012, 2.8,   82.0, 333);
    list << make("OFZ238", "ОФЗ 26238",   "Облигации",  5000, 612.0, 0.008, 0.8,  600.0, 444);
    list << make("MGNT",   "Магнит",      "Ритейл",      220,5800.0, 0.024, 2.9, 5600.0, 555);
    return list;
}

static QVector<NewsItem> generateNews()
{
    return {
        {"РБК",         "Россия наращивает золотые резервы: +12 тонн за квартал",
         "Банк России сообщил об увеличении золотого запаса. Эксперты ожидают продолжения.",
         "GOLD",   QDate::currentDate().addDays(-1)},
        {"Ведомости",   "Сбербанк повысил ставки по ипотеке до 18,5%",
         "Решение принято после заседания ЦБ РФ. Аналитики ожидают охлаждения рынка жилья.",
         "SBER",   QDate::currentDate().addDays(-2)},
        {"Коммерсантъ", "Газпром подписал СПГ-контракты с Азией на 15 лет",
         "Долгосрочные соглашения с КНР и Индией. Поставки начнутся в I кв. следующего года.",
         "GAZP",   QDate::currentDate().addDays(-3)},
        {"Forbes",      "Яндекс запускает B2B-облако для корпораций",
         "Акции выросли на 3,2% по итогам торгового дня после объявления о новом продукте.",
         "YNDX",   QDate::currentDate().addDays(-3)},
        {"Forbes",      "Лукойл выплатит рекордные дивиденды: 1 100 ₽ на акцию",
         "Совет директоров рекомендовал исторически максимальные выплаты. Реестр — 15 июля.",
         "LKOH",   QDate::currentDate().addDays(-4)},
        {"ТАСС",        "ЦБ РФ сохранил ключевую ставку на уровне 16%",
         "Регулятор указал на инфляционное давление. ОФЗ отреагировали ростом доходностей.",
         "OFZ238", QDate::currentDate().addDays(-5)},
        {"РБК",         "Доллар превысил 91 рубль впервые за три месяца",
         "Ослабление рубля связано с ростом импорта и сезонным спросом на валюту.",
         "USDRUB", QDate::currentDate().addDays(-6)},
        {"Ведомости",   "Норникель сократит производство палладия на 8%",
         "Компания скорректировала план из-за ремонта на Октябрьском руднике.",
         "GMKN",   QDate::currentDate().addDays(-7)},
        {"Коммерсантъ", "Магнит открыл 200 магазинов в малых городах",
         "Ритейлер ускорил региональную экспансию. Ожидается 600+ новых точек за год.",
         "MGNT",   QDate::currentDate().addDays(-8)},
        {"Интерфакс",   "Роснефть получила лицензию на арктический шельф",
         "Выданы разрешения на геологоразведку в трёх блоках Карского моря.",
         "ROSN",   QDate::currentDate().addDays(-9)},
    };
}

static QString shortFmt(double v)
{
    bool neg = v < 0; v = qAbs(v);
    QString s;
    if      (v >= 1e9) s = QString::number(v/1e9,'f',2)+" млрд";
    else if (v >= 1e6) s = QString::number(v/1e6,'f',1)+" млн";
    else if (v >= 1e3) s = QString::number(v/1e3,'f',1)+" тыс";
    else               s = QString::number(v,'f',2);
    return (neg?"−":"+")+s;
}
static QString fmtRub(double v){ return shortFmt(v)+" ₽"; }

static QColor sectorColor(const QString &s)
{
    static const QMap<QString,QColor> m={
        {"Финансы",{79,110,247}},{"Энергетика",{245,158,11}},
        {"Нефть/газ",{249,115,22}},{"Технологии",{139,92,246}},
        {"Металлы",{107,114,128}},{"Сырьё",{251,191,36}},
        {"Валюта",{6,182,212}},{"Облигации",{52,211,153}},
        {"Ритейл",{236,72,153}},
    };
    return m.value(s,{75,85,99});
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowIcon(QIcon(":src/icons/avx1d-0ksyk.icns"));
    setWindowTitle("InvestPro");
    setMinimumSize(1280,800);
    resize(1500,920);
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_assets = generateAssets();
    m_news   = generateNews();
    setupUI();
    applyTheme();

    auto *t = new QTimer(this);
    connect(t,&QTimer::timeout,this,&MainWindow::tickerUpdate);
    t->start(3000);
}
MainWindow::~MainWindow(){}

void MainWindow::tickerUpdate()
{
    QRandomGenerator *rng = QRandomGenerator::global();
    for (auto &a : m_assets){
        double d=(rng->generateDouble()-0.499)*0.004*a.currentPrice;
        a.prevPrice=a.currentPrice;
        a.currentPrice+=d;
        a.profit=(a.currentPrice-a.avgPrice)*a.tickCount;
    }
    populateAssetList();
    if(m_pieChart) m_pieChart->setAssets(m_assets);
    if(m_tickerBar) m_tickerBar->setAssets(m_assets);
}

void MainWindow::setupUI()
{
    m_central=new QWidget(this);
    m_central->setObjectName("centralWidget");
    setCentralWidget(m_central);
    auto *root=new QVBoxLayout(m_central);
    root->setContentsMargins(12,12,12,12);
    root->setSpacing(6);

    m_tickerBar=new TickerBarWidget(m_assets);
    m_tickerBar->setFixedHeight(26);
    root->addWidget(m_tickerBar);

    buildHeader();
    root->addWidget(m_headerWidget);

    auto *split=new QSplitter(Qt::Horizontal);
    split->setObjectName("mainSplitter");
    split->setHandleWidth(4);
    buildLeftPanel();
    buildRightPanel();
    split->addWidget(m_leftPanel);
    split->addWidget(m_rightPanel);
    split->setStretchFactor(0,0);
    split->setStretchFactor(1,1);
    split->setSizes({330,1150});
    root->addWidget(split,1);
}

void MainWindow::buildHeader()
{
    m_headerWidget=new QWidget;
    m_headerWidget->setObjectName("headerWidget");
    m_headerWidget->setFixedHeight(74);
    auto *hl=new QHBoxLayout(m_headerWidget);
    hl->setContentsMargins(20,0,16,0);
    hl->setSpacing(0);

    auto *logo=new QLabel("📈 InvestPro");
    logo->setObjectName("logoLabel");
    hl->addWidget(logo);
    hl->addSpacing(24);

    auto *avatar=new QLabel("КР");
    avatar->setObjectName("avatarLabel");
    avatar->setFixedSize(48,48);
    avatar->setAlignment(Qt::AlignCenter);
    hl->addWidget(avatar);
    hl->addSpacing(10);

    auto *nameCol=new QVBoxLayout; nameCol->setSpacing(1);
    auto *nameL=new QLabel("Кирилл Рудаков"); nameL->setObjectName("accountLabel");
    auto *roleL=new QLabel("Премиум · Верифицирован ✓"); roleL->setObjectName("roleLabel");
    nameCol->addWidget(nameL); nameCol->addWidget(roleL);
    hl->addLayout(nameCol);
    hl->addSpacing(28);

    auto addSep=[&]{
        auto *s=new QFrame; s->setFrameShape(QFrame::VLine);
        s->setObjectName("hdrSep"); hl->addWidget(s); hl->addSpacing(24);
    };
    auto addStat=[&](const QString &icon,const QString &title,
                     const QString &val,const QString &obj){
        auto *box=new QVBoxLayout; box->setSpacing(1);
        auto *t=new QLabel(icon+"  "+title); t->setObjectName("statTitle");
        auto *v=new QLabel(val); v->setObjectName(obj);
        box->addWidget(t); box->addWidget(v);
        hl->addLayout(box); hl->addSpacing(24);
    };

    addSep();
    addStat("📈","Доходность","+14.73%","statGreen");
    addSep();
    addStat("🛡","Индекс риска","2.34 / 10","statBlue");
    addSep();
    addStat("💰","Стоимость портфеля","2 497 381 644 ₽","statBalance");
    addSep();
    addStat("📊","Позиций",QString::number(m_assets.size())+" активов","statBlue");
    hl->addStretch();

    auto *notif=new QPushButton("🔔"); notif->setObjectName("iconBtn");
    notif->setFixedSize(36,36);
    hl->addWidget(notif); hl->addSpacing(6);

    auto *closeBtn=new QPushButton("✕"); closeBtn->setObjectName("closeBtn");
    closeBtn->setFixedSize(36,36);
    connect(closeBtn,&QPushButton::clicked,this,&QWidget::close);
    hl->addWidget(closeBtn);
}

void MainWindow::buildLeftPanel()
{
    m_leftPanel=new QWidget;
    m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setMinimumWidth(300);
    m_leftPanel->setMaximumWidth(370);
    auto *ll=new QVBoxLayout(m_leftPanel);
    ll->setContentsMargins(8,10,6,8);
    ll->setSpacing(6);

    auto *searchBar=new QWidget; searchBar->setObjectName("searchBar");
    searchBar->setFixedHeight(34);
    auto *sbl=new QHBoxLayout(searchBar); sbl->setContentsMargins(10,0,10,0);
    auto *sicon=new QLabel("🔍"); sicon->setFixedWidth(18);
    auto *sedit=new QLineEdit; sedit->setObjectName("searchEdit");
    sedit->setPlaceholderText("Поиск актива..."); sedit->setFrame(false);
    sbl->addWidget(sicon); sbl->addWidget(sedit);
    ll->addWidget(searchBar);

    auto *chipsRow=new QHBoxLayout; chipsRow->setSpacing(4);
    for(const QString &lb:{"Все","Акции","Сырьё","Валюта"}){
        auto *chip=new QPushButton(lb); chip->setObjectName("chipBtn");
        chip->setFixedHeight(24); chipsRow->addWidget(chip);
    }
    chipsRow->addStretch();
    ll->addLayout(chipsRow);

    auto *pt=new QLabel("  АКТИВЫ  ·  "+QString::number(m_assets.size()));
    pt->setObjectName("panelTitle");
    ll->addWidget(pt);

    m_assetList=new QListWidget;
    m_assetList->setObjectName("assetList");
    m_assetList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_assetList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_assetList->setSpacing(3);
    populateAssetList();
    connect(m_assetList,&QListWidget::itemDoubleClicked,
            this,&MainWindow::onAssetDoubleClicked);
    ll->addWidget(m_assetList,1);

    double tp=0; for(const auto &a:m_assets) tp+=a.profit;
    auto *mini=new QWidget; mini->setObjectName("miniSummary"); mini->setFixedHeight(50);
    auto *ml=new QHBoxLayout(mini); ml->setContentsMargins(12,6,12,6);
    auto *ml1=new QLabel("Итого P/L:"); ml1->setObjectName("miniLabel");
    auto *ml2=new QLabel(fmtRub(tp));
    ml2->setObjectName(tp>=0?"miniValueGreen":"miniValueRed");
    ml->addWidget(ml1); ml->addStretch(); ml->addWidget(ml2);
    ll->addWidget(mini);
}

void MainWindow::buildRightPanel()
{
    m_rightPanel=new QWidget; m_rightPanel->setObjectName("rightPanel");
    auto *rl=new QVBoxLayout(m_rightPanel);
    rl->setContentsMargins(4,0,0,0); rl->setSpacing(6);
    buildMenuBar();
    rl->addWidget(m_menuBarWidget);
    m_pageStack=new QStackedWidget; m_pageStack->setObjectName("pageStack");
    buildHomePage();
    buildNewsPage();
    buildAnalyticsPage();
    buildCalendarPage();
    buildScreenerPage();
    buildPlaceholderPage("⚙  Настройки\n\nРаздел в разработке");
    rl->addWidget(m_pageStack,1);
}

void MainWindow::buildMenuBar()
{
    m_menuBarWidget=new QWidget; m_menuBarWidget->setObjectName("menuBarWidget");
    m_menuBarWidget->setFixedHeight(48);
    auto *ml=new QHBoxLayout(m_menuBarWidget);
    ml->setContentsMargins(12,6,12,6); ml->setSpacing(4);
    struct Btn{QString icon;QString text;int page;};
    QVector<Btn> btns={{"⌂","Главная",0},{"📰","Новости",1},
                       {"📊","Аналитика",2},{"📅","Календарь",3},{"🔭","Скринер",4}};
    for(const auto &b:btns){
        auto *btn=new QPushButton(b.icon+"  "+b.text);
        btn->setProperty("menuBtn",true); btn->setObjectName("menuBtn");
        btn->setCursor(Qt::PointingHandCursor);
        int pg=b.page;
        connect(btn,&QPushButton::clicked,[this,pg]{switchPage(pg);});
        ml->addWidget(btn);
    }
    ml->addStretch();
    auto *sep=new QFrame; sep->setFrameShape(QFrame::VLine);
    sep->setObjectName("menuSep"); ml->addWidget(sep);
    auto *settBtn=new QPushButton("⚙  Настройки");
    settBtn->setProperty("menuBtn",true); settBtn->setObjectName("menuBtn");
    connect(settBtn,&QPushButton::clicked,[this]{switchPage(5);});
    ml->addWidget(settBtn);
    m_themeBtn=new QPushButton("☀  Тема");
    m_themeBtn->setProperty("menuBtn",true); m_themeBtn->setObjectName("menuBtnTheme");
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_themeBtn,&QPushButton::clicked,this,&MainWindow::toggleTheme);
    ml->addWidget(m_themeBtn);
}

void MainWindow::buildHomePage()
{
    auto *page=new QWidget;
    auto *pl=new QHBoxLayout(page);
    pl->setContentsMargins(8,8,8,8); pl->setSpacing(10);

    auto *leftCol=new QWidget;
    auto *lcl=new QVBoxLayout(leftCol);
    lcl->setContentsMargins(0,0,0,0); lcl->setSpacing(8);

    m_pieChart=new PieChartWidget(m_assets);
    m_pieChart->setMinimumSize(340,340);
    lcl->addWidget(m_pieChart);

    auto *priCard=new QWidget; priCard->setObjectName("card");
    auto *prcl=new QVBoxLayout(priCard); prcl->setContentsMargins(14,10,14,10);
    auto *priTitle=new QLabel("🛡  Personal Risk Index"); priTitle->setObjectName("cardTitle");
    auto *priBar=new RiskGaugeWidget(2.34,10.0); priBar->setFixedHeight(68);
    auto *priDesc=new QLabel("Умеренный уровень риска (2.34/10). "
        "Защитные активы (ОФЗ, золото) — 38% портфеля.");
    priDesc->setObjectName("cardText"); priDesc->setWordWrap(true);
    prcl->addWidget(priTitle); prcl->addWidget(priBar); prcl->addWidget(priDesc);
    lcl->addWidget(priCard);
    lcl->addStretch();

    auto *rightCol=new QWidget;
    auto *rcl=new QVBoxLayout(rightCol);
    rcl->setContentsMargins(0,0,0,0); rcl->setSpacing(8);

    auto *kpiRow=new QHBoxLayout; kpiRow->setSpacing(8);
    struct KPI{QString icon;QString title;QString value;QString color;};
    QVector<KPI> kpis={
        {"💹","Прибыль сегодня","+287 430 ₽","#34d399"},
        {"📉","Макс. просадка","−3.8%","#f87171"},
        {"🔄","Оборот (30 дн.)","18.4 млрд ₽","#93c5fd"},
        {"⭐","Шарп / Сортино","1.42 / 1.87","#fbbf24"},
    };
    for(const auto &k:kpis){
        auto *card=new QWidget; card->setObjectName("kpiCard");
        auto *cl=new QVBoxLayout(card); cl->setContentsMargins(14,12,14,12); cl->setSpacing(3);
        auto *ti=new QLabel(k.icon+"  "+k.title); ti->setObjectName("kpiTitle");
        auto *va=new QLabel(k.value);
        va->setStyleSheet(QString("color:%1;font-size:16px;font-weight:800;").arg(k.color));
        cl->addWidget(ti); cl->addWidget(va);
        kpiRow->addWidget(card,1);
    }
    rcl->addLayout(kpiRow);

    auto *sumLabel=new QLabel("📋  Сводка портфеля"); sumLabel->setObjectName("sectionTitle");
    rcl->addWidget(sumLabel);

    auto *scroll=new QScrollArea;
    scroll->setWidgetResizable(true); scroll->setObjectName("summaryScroll");
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *sc=new QWidget;
    auto *scl=new QVBoxLayout(sc);
    scl->setContentsMargins(4,4,4,4); scl->setSpacing(5);
    for(const Asset &a:m_assets) scl->addWidget(new SummaryRowWidget(a));
    scl->addStretch();
    scroll->setWidget(sc);
    rcl->addWidget(scroll,1);

    pl->addWidget(leftCol,0);
    pl->addWidget(rightCol,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildNewsPage()
{
    auto *page=new QWidget;
    auto *pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto *title=new QLabel("📰  Новостная лента"); title->setObjectName("pageTitle");
    pl->addWidget(title);
    auto *scroll=new QScrollArea;
    scroll->setWidgetResizable(true); scroll->setObjectName("newsScroll");
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content=new QWidget;
    auto *cl=new QVBoxLayout(content);
    cl->setContentsMargins(4,4,4,4); cl->setSpacing(8);
    for(const NewsItem &n:m_news) cl->addWidget(new NewsCardWidget(n));
    cl->addStretch();
    scroll->setWidget(content);
    pl->addWidget(scroll,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildAnalyticsPage()
{
    auto *page=new QWidget;
    auto *pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto *title=new QLabel("📊  Аналитика портфеля"); title->setObjectName("pageTitle");
    pl->addWidget(title);

    auto *row1=new QHBoxLayout; row1->setSpacing(8);
    struct Metric{QString name;QString val;QString sub;QString color;QString icon;};
    QVector<Metric> metrics={
        {"Beta портфеля","0.87","vs MOEX","#93c5fd","β"},
        {"Alpha","+2.4%","годовых","#34d399","α"},
        {"VaR (95%)","−4.2%","дневной","#f87171","⚡"},
        {"Корреляция","0.61","между акт.","#fbbf24","∞"},
        {"Макс. дродаун","−8.3%","за 12 мес.","#f87171","↓"},
        {"Волатильность","12.4%","годовая","#fbbf24","~"},
    };
    for(const auto &m:metrics){
        auto *card=new QWidget; card->setObjectName("analyticsCard");
        auto *cl=new QVBoxLayout(card); cl->setContentsMargins(14,12,14,12);
        auto *ic=new QLabel(m.icon);
        ic->setStyleSheet(QString("color:%1;font-size:22px;font-weight:800;").arg(m.color));
        auto *nm=new QLabel(m.name); nm->setObjectName("analyticsCardTitle");
        auto *vl=new QLabel(m.val);
        vl->setStyleSheet(QString("color:%1;font-size:20px;font-weight:800;").arg(m.color));
        auto *sb=new QLabel(m.sub); sb->setObjectName("analyticsCardSub");
        cl->addWidget(ic); cl->addWidget(nm); cl->addWidget(vl); cl->addWidget(sb);
        row1->addWidget(card,1);
    }
    pl->addLayout(row1);

    auto *allocLabel=new QLabel("  Аллокация по секторам"); allocLabel->setObjectName("sectionTitle");
    pl->addWidget(allocLabel);
    auto *alloc=new AllocationBarWidget(m_assets);
    alloc->setMinimumHeight(220);
    pl->addWidget(alloc,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildCalendarPage()
{
    auto *page=new QWidget;
    auto *pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto *title=new QLabel("📅  Дивидендный календарь"); title->setObjectName("pageTitle");
    pl->addWidget(title);

    struct Div{QString tk;QString co;QString dt;QString am;QString yd;};
    QVector<Div> evs={
        {"LKOH","Лукойл","15.07.2025","1 100 ₽/акц","8.4%"},
        {"SBER","Сбербанк","23.07.2025","33.5 ₽/акц","6.1%"},
        {"MGNT","Магнит","02.08.2025","412 ₽/акц","5.2%"},
        {"GAZP","Газпром","10.08.2025","25.0 ₽/акц","4.8%"},
        {"ROSN","Роснефть","28.08.2025","38.6 ₽/акц","4.1%"},
        {"GMKN","НорНикель","15.09.2025","780 ₽/акц","3.9%"},
        {"OFZ238","ОФЗ 26238","20.09.2025","37.9 ₽/бум","6.0%"},
    };
    auto *scroll=new QScrollArea;
    scroll->setWidgetResizable(true); scroll->setObjectName("summaryScroll");
    auto *content=new QWidget;
    auto *cl=new QVBoxLayout(content);
    cl->setContentsMargins(4,4,4,4); cl->setSpacing(6);
    for(const auto &e:evs){
        auto *row=new QWidget; row->setObjectName("calRow"); row->setFixedHeight(56);
        auto *rl=new QHBoxLayout(row); rl->setContentsMargins(14,6,14,6);
        auto *badge=new QLabel(e.tk); badge->setAlignment(Qt::AlignCenter);
        badge->setFixedSize(70,32);
        badge->setStyleSheet("background:#4f6ef7;border-radius:8px;"
                             "color:white;font-weight:800;font-size:11px;");
        auto *co=new QLabel(e.co); co->setObjectName("calCompany");
        auto *dt=new QLabel("📅  "+e.dt); dt->setObjectName("calDate");
        auto *am=new QLabel(e.am); am->setObjectName("calAmount");
        auto *yd=new QLabel(e.yd); yd->setObjectName("calYield");
        rl->addWidget(badge); rl->addSpacing(12);
        rl->addWidget(co,2); rl->addWidget(dt,2); rl->addWidget(am,2); rl->addWidget(yd,1);
        cl->addWidget(row);
    }
    cl->addStretch();
    scroll->setWidget(content);
    pl->addWidget(scroll,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildScreenerPage()
{
    auto *page=new QWidget;
    auto *pl=new QVBoxLayout(page);
    pl->setContentsMargins(10,10,10,10); pl->setSpacing(8);
    auto *title=new QLabel("🔭  Скринер активов"); title->setObjectName("pageTitle");
    pl->addWidget(title);

    auto *fr=new QHBoxLayout; fr->setSpacing(8);
    for(const QString &f:{"P/E < 15","Div yield > 5%","Капитал > 500 млрд","ROE > 20%","Beta < 1.0"}){
        auto *chip=new QPushButton(f); chip->setObjectName("screenChip");
        chip->setCheckable(true); fr->addWidget(chip);
    }
    fr->addStretch();
    pl->addLayout(fr);

    struct SR{QString tk;QString nm;double pe;double dv;double roe;double bt;double cap;};
    QVector<SR> rows={
        {"SBER","Сбербанк",5.1,6.1,23.4,0.85,6420.0},
        {"GAZP","Газпром",3.2,4.8,11.2,0.72,3150.0},
        {"LKOH","Лукойл",6.4,8.4,19.6,0.91,5200.0},
        {"YNDX","Яндекс",28.7,0.0,12.1,1.41,1820.0},
        {"GMKN","НорНикель",8.9,3.9,31.2,0.78,2780.0},
        {"ROSN","Роснефть",4.6,4.1,14.3,0.88,4100.0},
        {"MGNT","Магнит",11.2,5.2,22.8,0.95,1640.0},
    };
    auto *scroll=new QScrollArea;
    scroll->setWidgetResizable(true); scroll->setObjectName("summaryScroll");
    auto *content=new QWidget;
    auto *cl=new QVBoxLayout(content);
    cl->setContentsMargins(4,4,4,4); cl->setSpacing(4);

    auto *hdrRow=new QWidget; hdrRow->setObjectName("tableHdrRow"); hdrRow->setFixedHeight(32);
    auto *hrl=new QHBoxLayout(hdrRow); hrl->setContentsMargins(14,0,14,0);
    for(const QString &h:{"Тикер","Название","P/E","Дивид.","ROE","Beta","Кап.(млрд)"}){
        auto *l=new QLabel(h); l->setObjectName("tableHdr");
        hrl->addWidget(l,h=="Название"?2:1);
    }
    cl->addWidget(hdrRow);

    for(const auto &r:rows){
        auto *row=new QWidget; row->setObjectName("calRow"); row->setFixedHeight(44);
        auto *rl=new QHBoxLayout(row); rl->setContentsMargins(14,6,14,6);
        auto col=[&](const QString &t,int s,const QString &style){
            auto *l=new QLabel(t); l->setStyleSheet(style); rl->addWidget(l,s);
        };
        col(r.tk,1,"color:#93c5fd;font-size:12px;font-weight:800;");
        col(r.nm,2,"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.pe,'f',1),1,r.pe<15?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.dv,'f',1)+"%",1,r.dv>5?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.roe,'f',1)+"%",1,r.roe>20?"color:#34d399;font-size:12px;":"color:#e8eaf6;font-size:12px;");
        col(QString::number(r.bt,'f',2),1,r.bt<1?"color:#34d399;font-size:12px;":"color:#fbbf24;font-size:12px;");
        col(QString::number(r.cap,'f',0),1,"color:#9ca3af;font-size:12px;");
        cl->addWidget(row);
    }
    cl->addStretch();
    scroll->setWidget(content);
    pl->addWidget(scroll,1);
    m_pageStack->addWidget(page);
}

void MainWindow::buildPlaceholderPage(const QString &msg)
{
    auto *page=new QWidget;
    auto *l=new QVBoxLayout(page);
    auto *lbl=new QLabel(msg); lbl->setObjectName("placeholderLabel");
    lbl->setAlignment(Qt::AlignCenter);
    l->addWidget(lbl);
    m_pageStack->addWidget(page);
}

void MainWindow::populateAssetList()
{
    m_assetList->clear();
    for(int i=0;i<m_assets.size();++i){
        const Asset &a=m_assets[i];
        bool up=a.currentPrice>=a.prevPrice;
        double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
        auto *item=new QListWidgetItem(m_assetList);
        item->setData(Qt::UserRole,i);
        auto *w=new AssetRowWidget(a,up,pct);
        item->setSizeHint(w->sizeHint());
        m_assetList->setItemWidget(item,w);
    }
}

void MainWindow::showAssetDetail(int index)
{
    if(index<0||index>=m_assets.size()) return;
    if(m_detailPage){
        m_pageStack->removeWidget(m_detailPage);
        delete m_detailPage;
        m_detailPage=nullptr;
    }
    const Asset &a=m_assets[index];
    bool up=a.currentPrice>=a.prevPrice;
    double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;

    m_detailPage=new QWidget;
    auto *dl=new QVBoxLayout(m_detailPage);
    dl->setContentsMargins(10,10,10,10); dl->setSpacing(8);

    auto *hdrCard=new QWidget; hdrCard->setObjectName("detailHdrCard");
    auto *hcl=new QHBoxLayout(hdrCard); hcl->setContentsMargins(16,12,16,12);

    auto *backBtn=new QPushButton("← Назад"); backBtn->setObjectName("backBtn");
    connect(backBtn,&QPushButton::clicked,[this]{switchPage(0);});

    auto *iconLbl=new QLabel(a.ticker.left(2));
    iconLbl->setFixedSize(50,50); iconLbl->setAlignment(Qt::AlignCenter);
    iconLbl->setStyleSheet(QString("background:%1;border-radius:25px;"
        "color:white;font-weight:800;font-size:14px;").arg(sectorColor(a.sector).name()));

    auto *nameBox=new QVBoxLayout;
    auto *nameLbl=new QLabel(a.name); nameLbl->setObjectName("detailName");
    auto *sectorLbl=new QLabel(a.sector+"  ·  "+a.ticker); sectorLbl->setObjectName("detailSector");
    nameBox->addWidget(nameLbl); nameBox->addWidget(sectorLbl);

    auto *priceBox=new QVBoxLayout; priceBox->setAlignment(Qt::AlignRight);
    auto *priceLbl=new QLabel(QString::number(a.currentPrice,'f',2)+" ₽");
    priceLbl->setObjectName(up?"detailPriceUp":"detailPriceDown");
    priceLbl->setAlignment(Qt::AlignRight);
    auto *chgLbl=new QLabel((up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%");
    chgLbl->setObjectName(up?"changeUp":"changeDown");
    chgLbl->setAlignment(Qt::AlignRight);
    priceBox->addWidget(priceLbl); priceBox->addWidget(chgLbl);

    hcl->addWidget(backBtn); hcl->addSpacing(14);
    hcl->addWidget(iconLbl); hcl->addSpacing(10);
    hcl->addLayout(nameBox); hcl->addStretch();
    hcl->addLayout(priceBox);
    dl->addWidget(hdrCard);
    dl->addWidget(buildKPIWidget(a));

    auto *chartLabel=new QLabel("  📈  График (японские свечи) — 120 дней");
    chartLabel->setObjectName("sectionTitle");
    dl->addWidget(chartLabel);

    auto *cc=new CandleChartWidget(a.candles);
    cc->setMinimumHeight(360);
    dl->addWidget(cc,1);

    m_pageStack->addWidget(m_detailPage);
    m_pageStack->setCurrentWidget(m_detailPage);
}

QWidget *MainWindow::buildKPIWidget(const Asset &a)
{
    bool up=a.currentPrice>=a.prevPrice;
    auto *w=new QWidget; w->setObjectName("kpiStrip");
    auto *l=new QHBoxLayout(w); l->setContentsMargins(0,0,0,0); l->setSpacing(6);
    struct KV{QString t;QString v;QString obj;};
    QVector<KV> items={
        {"Текущая цена",QString::number(a.currentPrice,'f',2)+"₽",up?"kpiValUp":"kpiValDown"},
        {"Кол-во",QString::number(a.tickCount)+" шт.","kpiVal"},
        {"Ср.цена покупки",QString::number(a.avgPrice,'f',2)+"₽","kpiVal"},
        {"Позиция",shortFmt(a.currentPrice*a.tickCount)+"₽","kpiVal"},
        {"P/L",fmtRub(a.profit),a.profit>=0?"kpiValUp":"kpiValDown"},
        {"PRI",QString::number(a.personalRisk,'f',1)+"/10","kpiVal"},
        {"Сектор",a.sector,"kpiVal"},
    };
    for(const auto &kv:items){
        auto *card=new QWidget; card->setObjectName("kpiCard2");
        auto *cl=new QVBoxLayout(card); cl->setContentsMargins(12,8,12,8); cl->setSpacing(2);
        auto *t=new QLabel(kv.t); t->setObjectName("kpiTitle");
        auto *v=new QLabel(kv.v); v->setObjectName(kv.obj);
        cl->addWidget(t); cl->addWidget(v);
        l->addWidget(card,1);
    }
    return w;
}

void MainWindow::switchPage(int index)
{
    if(index>=0&&index<m_pageStack->count())
        m_pageStack->setCurrentIndex(index);
}

void MainWindow::onAssetDoubleClicked(QListWidgetItem *item)
{
    if(!item) return;
    showAssetDetail(item->data(Qt::UserRole).toInt());
}

void MainWindow::toggleTheme()
{
    m_darkTheme=!m_darkTheme;
    m_themeBtn->setText(m_darkTheme?"☀  Тема":"🌙  Тема");
    applyTheme(); update();
}

void MainWindow::applyTheme(){ m_darkTheme?applyDarkTheme():applyLightTheme(); }

void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(R"(
#centralWidget{background:#080c14;border-radius:18px;}
#headerWidget{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0e1526,stop:1 #0b1120);
 border-radius:14px;border:1px solid #1a2340;}
#logoLabel{color:#4f6ef7;font-size:18px;font-weight:800;letter-spacing:1px;}
#avatarLabel{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4f6ef7,stop:1 #8b5cf6);
 border-radius:24px;color:white;font-weight:800;font-size:16px;}
#accountLabel{color:#f0f4ff;font-size:15px;font-weight:700;}
#roleLabel{color:#4f6ef7;font-size:10px;}
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
#chipBtn{background:#111828;border:1px solid #1a2340;color:#4b5563;
 border-radius:10px;padding:2px 10px;font-size:10px;}
#chipBtn:hover{background:#1a2a44;color:#e8eaf6;border-color:#4f6ef7;}
#panelTitle{color:#2a3555;font-size:9px;font-weight:700;letter-spacing:2px;padding:2px 6px;}
#assetList{background:transparent;border:none;}
#assetList::item{border-radius:10px;margin:1px 2px;}
#assetList::item:selected{background:#111828;border:1px solid #1a2340;}
#assetList::item:hover{background:#0e1624;}
#miniSummary{background:#111828;border-radius:10px;border:1px solid #1a2340;}
#miniLabel{color:#4b5563;font-size:11px;}
#miniValueGreen{color:#34d399;font-size:13px;font-weight:800;}
#miniValueRed{color:#f87171;font-size:13px;font-weight:800;}
#menuBarWidget{background:#0c1020;border-radius:12px;border:1px solid #1a2340;}
QPushButton[menuBtn="true"]{background:transparent;border:none;color:#4b5563;
 font-size:12px;padding:6px 14px;border-radius:8px;}
QPushButton[menuBtn="true"]:hover{background:#111828;color:#e8eaf6;}
QPushButton[menuBtn="true"]:pressed{background:#1a2a44;}
#menuBtnTheme{background:#111828;border:1px solid #1a2340;
 color:#fbbf24;font-size:12px;padding:6px 14px;border-radius:8px;}
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
QScrollBar:vertical{background:#0c1020;width:5px;border-radius:3px;}
QScrollBar::handle:vertical{background:#1a2340;border-radius:3px;min-height:24px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#summaryScroll,#newsScroll{background:transparent;border:none;}
#detailHdrCard{background:#111828;border-radius:12px;border:1px solid #1a2340;}
#backBtn{background:transparent;border:1px solid #1a2340;color:#4b5563;
 padding:7px 14px;border-radius:8px;font-size:12px;}
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
#screenChip{background:#111828;border:1px solid #1a2340;color:#4b5563;
 border-radius:10px;padding:4px 12px;font-size:11px;}
#screenChip:hover{background:#1a2a44;color:#e8eaf6;border-color:#4f6ef7;}
#screenChip:checked{background:#4f6ef720;color:#4f6ef7;border-color:#4f6ef7;}
)");
}

void MainWindow::applyLightTheme()
{
    qApp->setStyleSheet(R"(
#centralWidget{background:#eef2ff;border-radius:18px;}
#headerWidget{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #ffffff,stop:1 #f8faff);
 border-radius:14px;border:1px solid #d1d9f0;}
#logoLabel{color:#4f6ef7;font-size:18px;font-weight:800;letter-spacing:1px;}
#avatarLabel{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4f6ef7,stop:1 #8b5cf6);
 border-radius:24px;color:white;font-weight:800;font-size:16px;}
#accountLabel{color:#1e293b;font-size:15px;font-weight:700;}
#roleLabel{color:#4f6ef7;font-size:10px;}
#statTitle{color:#94a3b8;font-size:9px;letter-spacing:1px;}
#statGreen{color:#059669;font-size:19px;font-weight:900;}
#statBlue{color:#3b82f6;font-size:19px;font-weight:900;}
#statBalance{color:#1e293b;font-size:16px;font-weight:900;}
#iconBtn,#closeBtn{background:#f1f5f9;border:1px solid #d1d9f0;
 border-radius:10px;color:#64748b;font-size:13px;}
#iconBtn:hover{background:#e8eeff;color:#1e293b;}
#closeBtn:hover{background:#ef4444;color:white;border-color:#ef4444;}
#leftPanel{background:#ffffff;border-radius:14px;border:1px solid #d1d9f0;}
#searchBar{background:#f1f5f9;border-radius:10px;border:1px solid #d1d9f0;}
#searchEdit{background:transparent;color:#1e293b;font-size:12px;}
#chipBtn{background:#f1f5f9;border:1px solid #d1d9f0;color:#64748b;
 border-radius:10px;padding:2px 10px;font-size:10px;}
#chipBtn:hover{background:#eef2ff;color:#1e293b;border-color:#4f6ef7;}
#panelTitle{color:#94a3b8;font-size:9px;font-weight:700;letter-spacing:2px;padding:2px 6px;}
#assetList{background:transparent;border:none;}
#assetList::item{border-radius:10px;margin:1px 2px;}
#assetList::item:selected{background:#eef2ff;border:1px solid #d1d9f0;}
#assetList::item:hover{background:#f5f7ff;}
#miniSummary{background:#f1f5f9;border-radius:10px;border:1px solid #d1d9f0;}
#miniLabel{color:#64748b;font-size:11px;}
#miniValueGreen{color:#059669;font-size:13px;font-weight:800;}
#miniValueRed{color:#dc2626;font-size:13px;font-weight:800;}
#menuBarWidget{background:#ffffff;border-radius:12px;border:1px solid #d1d9f0;}
QPushButton[menuBtn="true"]{background:transparent;border:none;color:#64748b;
 font-size:12px;padding:6px 14px;border-radius:8px;}
QPushButton[menuBtn="true"]:hover{background:#eef2ff;color:#1e293b;}
#menuBtnTheme{background:#fefce8;border:1px solid #fde68a;
 color:#d97706;font-size:12px;padding:6px 14px;border-radius:8px;}
#menuBtnTheme:hover{background:#fef9c3;}
#pageStack{background:#ffffff;border-radius:14px;border:1px solid #d1d9f0;}
#card,#kpiCard,#analyticsCard{background:#f8faff;border-radius:12px;border:1px solid #d1d9f0;}
#cardTitle{color:#3b82f6;font-size:13px;font-weight:700;}
#cardText{color:#64748b;font-size:11px;}
#kpiTitle{color:#94a3b8;font-size:9px;letter-spacing:1px;}
#kpiCard2{background:#f8faff;border-radius:10px;border:1px solid #d1d9f0;}
#kpiVal{color:#1e293b;font-size:13px;font-weight:800;}
#kpiValUp{color:#059669;font-size:13px;font-weight:800;}
#kpiValDown{color:#dc2626;font-size:13px;font-weight:800;}
#analyticsCardTitle{color:#64748b;font-size:10px;}
#analyticsCardSub{color:#94a3b8;font-size:10px;}
#sectionTitle{color:#1e293b;font-size:14px;font-weight:800;padding:4px 8px;}
#pageTitle{color:#1e293b;font-size:17px;font-weight:900;padding:4px 8px;}
#placeholderLabel{color:#cbd5e1;font-size:28px;}
QScrollArea{background:transparent;border:none;}
QScrollBar:vertical{background:#f1f5f9;width:5px;border-radius:3px;}
QScrollBar::handle:vertical{background:#cbd5e1;border-radius:3px;min-height:24px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#detailHdrCard{background:#f8faff;border-radius:12px;border:1px solid #d1d9f0;}
#backBtn{background:transparent;border:1px solid #d1d9f0;color:#64748b;
 padding:7px 14px;border-radius:8px;font-size:12px;}
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
#screenChip{background:#f1f5f9;border:1px solid #d1d9f0;color:#64748b;
 border-radius:10px;padding:4px 12px;font-size:11px;}
#screenChip:hover{background:#eef2ff;color:#1e293b;border-color:#4f6ef7;}
#screenChip:checked{background:#eef2ff;color:#4f6ef7;border-color:#4f6ef7;}
)");
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(),18,18);
    p.setClipPath(path);
    QLinearGradient g(0,0,0,height());
    if(m_darkTheme){g.setColorAt(0,QColor("#080c14"));g.setColorAt(1,QColor("#050810"));}
    else{g.setColorAt(0,QColor("#eef2ff"));g.setColorAt(1,QColor("#e8edff"));}
    p.fillPath(path,g);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton&&e->pos().y()<80){
        m_dragging=true;
        m_dragPos=e->globalPosition().toPoint()-frameGeometry().topLeft();
    }
    QMainWindow::mousePressEvent(e);
}
void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(m_dragging&&(e->buttons()&Qt::LeftButton))
        move(e->globalPosition().toPoint()-m_dragPos);
    QMainWindow::mouseMoveEvent(e);
}
void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging=false; QMainWindow::mouseReleaseEvent(e);
}

AssetRowWidget::AssetRowWidget(const Asset &a,bool up,double pct,QWidget *parent)
    :QWidget(parent)
{
    setFixedHeight(64);
    auto *l=new QHBoxLayout(this);
    l->setContentsMargins(8,6,8,6); l->setSpacing(6);
    auto *icon=new QLabel(a.ticker.left(2));
    icon->setFixedSize(34,34); icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QString("background:%1;border-radius:17px;"
        "color:white;font-weight:800;font-size:10px;").arg(sectorColor(a.sector).name()));
    auto *center=new QVBoxLayout; center->setSpacing(2);
    auto *row1=new QHBoxLayout;
    auto *tl=new QLabel(a.ticker);
    tl->setStyleSheet("color:#e8eaf6;font-weight:800;font-size:13px;");
    auto *sl=new QLabel(a.sector);
    sl->setStyleSheet("color:#2a3555;font-size:9px;");
    row1->addWidget(tl); row1->addSpacing(4); row1->addWidget(sl); row1->addStretch();
    QString mini=QString("TC:%1 | CP:%2₽ | PL:%3 | PRI:%4")
        .arg(a.tickCount).arg(a.currentPrice,0,'f',1)
        .arg(a.profit>=0?"+"+shortFmt(a.profit):shortFmt(a.profit))
        .arg(a.personalRisk,0,'f',1);
    auto *ml=new QLabel(mini); ml->setStyleSheet("color:#2a3555;font-size:9px;");
    center->addLayout(row1); center->addWidget(ml);
    auto *right=new QVBoxLayout; right->setSpacing(1); right->setAlignment(Qt::AlignRight);
    QString clr=up?"#34d399":"#f87171";
    auto *priceLbl=new QLabel(QString::number(a.currentPrice,'f',1));
    priceLbl->setStyleSheet(QString("color:%1;font-size:13px;font-weight:800;").arg(clr));
    priceLbl->setAlignment(Qt::AlignRight);
    auto *pctLbl=new QLabel((up?"▲ +":"▼ ")+QString("%1%").arg(pct,0,'f',2));
    pctLbl->setStyleSheet(QString("color:%1;font-size:10px;font-weight:700;").arg(clr));
    pctLbl->setAlignment(Qt::AlignRight);
    right->addWidget(priceLbl); right->addWidget(pctLbl);
    l->addWidget(icon); l->addLayout(center,1); l->addLayout(right);
}

PieChartWidget::PieChartWidget(const QVector<Asset> &assets,QWidget *parent)
    :QWidget(parent),m_assets(assets)
{
    setMouseTracking(true);
    setMinimumSize(300,300);
    m_palette={{79,110,247},{139,92,246},{52,211,153},{248,159,49},
               {248,114,114},{6,182,212},{251,191,36},{236,72,153},
               {107,114,128},{34,211,238}};
}

void PieChartWidget::setAssets(const QVector<Asset> &assets){ m_assets=assets; update(); }

void PieChartWidget::paintEvent(QPaintEvent *)
{
    if(m_assets.isEmpty()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    double total=0;
    for(const auto &a:m_assets) total+=a.currentPrice*a.tickCount;
    if(total<=0) return;

    int cx=width()/2, cy=height()/2;
    int R=qMin(cx,cy)-28;
    int iR=R*52/100;

    double startAngle=-90.0*16;
    for(int i=0;i<m_assets.size();++i){
        double share=(m_assets[i].currentPrice*m_assets[i].tickCount)/total;
        int span=qRound(share*360*16);
        QColor col=m_palette[i%m_palette.size()];
        bool hov=(i==m_hovered);
        int offset=hov?10:0;
        double midAngle=(startAngle/16.0+span/32.0)*M_PI/180.0;
        int ox=hov?qRound(cos(midAngle)*offset):0;
        int oy=hov?qRound(sin(midAngle)*offset):0;
        if(hov){
            p.setBrush(QColor(col.red(),col.green(),col.blue(),40));
            p.setPen(Qt::NoPen);
            p.drawEllipse(cx+ox-R-10,cy+oy-R-10,(R+10)*2,(R+10)*2);
        }
        p.setBrush(hov?col.lighter(120):col);
        p.setPen(hov?QPen(Qt::white,2):Qt::NoPen);
        p.drawPie(cx-R+ox,cy-R+oy,R*2,R*2,(int)startAngle,span);
        startAngle+=span;
    }

    QRadialGradient hole(cx,cy,iR);
    hole.setColorAt(0,QColor("#131929"));
    hole.setColorAt(1,QColor("#0e1526"));
    p.setBrush(hole); p.setPen(Qt::NoPen);
    p.drawEllipse(cx-iR,cy-iR,iR*2,iR*2);

    if(m_hovered>=0&&m_hovered<m_assets.size()){
        const Asset &a=m_assets[m_hovered];
        double share=(a.currentPrice*a.tickCount)/total*100.0;
        bool up=a.currentPrice>=a.prevPrice;
        double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
        QColor ac=m_palette[m_hovered%m_palette.size()];
        p.setFont(QFont("Arial",13,QFont::Bold));
        p.setPen(ac);
        p.drawText(QRect(cx-60,cy-38,120,22),Qt::AlignCenter,a.ticker);
        p.setFont(QFont("Arial",8));
        p.setPen(QColor("#6b7280"));
        p.drawText(QRect(cx-60,cy-16,120,16),Qt::AlignCenter,a.sector);
        p.setFont(QFont("Arial",10,QFont::Bold));
        p.setPen(QColor("#e8eaf6"));
        p.drawText(QRect(cx-60,cy+2,120,18),Qt::AlignCenter,
                   QString::number(share,'f',1)+"% портфеля");
        p.setFont(QFont("Arial",9,QFont::Bold));
        p.setPen(up?QColor("#34d399"):QColor("#f87171"));
        p.drawText(QRect(cx-60,cy+22,120,16),Qt::AlignCenter,
                   (up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%");
    } else {
        p.setFont(QFont("Arial",11,QFont::Bold));
        p.setPen(QColor("#e8eaf6"));
        p.drawText(QRect(cx-70,cy-18,140,20),Qt::AlignCenter,"2 497 381 644 ₽");
        p.setFont(QFont("Arial",8));
        p.setPen(QColor("#4b5563"));
        p.drawText(QRect(cx-70,cy+4,140,16),Qt::AlignCenter,
                   "Портфель · "+QString::number(m_assets.size())+" активов");
    }

    int legendY=cy+R+12;
    if(legendY+m_assets.size()*17>height()-4) legendY=4;
    p.setFont(QFont("Arial",8));
    for(int i=0;i<m_assets.size();++i){
        int lx=(i<5)?6:width()/2+4;
        int ly=legendY+(i%5)*17;
        QColor c=m_palette[i%m_palette.size()];
        QPainterPath dot; dot.addRoundedRect(lx,ly+1,10,10,3,3);
        p.fillPath(dot,c);
        double share=(m_assets[i].currentPrice*m_assets[i].tickCount)/total*100.0;
        p.setPen(i==m_hovered?QColor("#e8eaf6"):QColor("#4b5563"));
        p.drawText(lx+14,ly,150,13,Qt::AlignLeft|Qt::AlignVCenter,
                   m_assets[i].ticker+"  "+QString::number(share,'f',1)+"%");
    }
}

void PieChartWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_assets.isEmpty()) return;
    double total=0;
    for(const auto &a:m_assets) total+=a.currentPrice*a.tickCount;
    if(total<=0) return;
    int cx=width()/2,cy=height()/2;
    int R=qMin(cx,cy)-28;
    int iR=R*52/100;
    QPointF pt=e->position();
    double dx=pt.x()-cx,dy=pt.y()-cy;
    double dist=sqrt(dx*dx+dy*dy);
    if(dist<iR||dist>R+12){
        if(m_hovered!=-1){m_hovered=-1;update();}
        return;
    }
    double angle=atan2(dy,dx)*180.0/M_PI+90.0;
    if(angle<0) angle+=360.0;
    double startAngle=0;
    int newHover=-1;
    for(int i=0;i<m_assets.size();++i){
        double share=(m_assets[i].currentPrice*m_assets[i].tickCount)/total;
        double span=share*360.0;
        if(angle>=startAngle&&angle<startAngle+span){newHover=i;break;}
        startAngle+=span;
    }
    if(newHover!=m_hovered){m_hovered=newHover;update();}
}

void PieChartWidget::leaveEvent(QEvent *)
{
    if(m_hovered!=-1){m_hovered=-1;update();}
}

RiskGaugeWidget::RiskGaugeWidget(double value,double max,QWidget *parent)
    :QWidget(parent),m_value(value),m_max(max){}

void RiskGaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w=width(),h=height();
    int barH=10,barY=h/2-barH/2-10;
    int bw=w-16,bx=8;
    QPainterPath bg; bg.addRoundedRect(bx,barY,bw,barH,5,5);
    p.fillPath(bg,QColor("#111828"));
    double ratio=qBound(0.0,m_value/m_max,1.0);
    int fillW=qRound(bw*ratio);
    if(fillW>0){
        QLinearGradient grad(bx,0,bx+bw,0);
        grad.setColorAt(0.0,QColor("#34d399"));
        grad.setColorAt(0.35,QColor("#fbbf24"));
        grad.setColorAt(0.7,QColor("#f97316"));
        grad.setColorAt(1.0,QColor("#ef4444"));
        QPainterPath fill; fill.addRoundedRect(bx,barY,fillW,barH,5,5);
        p.fillPath(fill,grad);
    }
    int mx=bx+qRound(bw*ratio);
    p.setBrush(Qt::white); p.setPen(QPen(QColor("#111828"),2));
    p.drawEllipse(mx-7,barY-3,14,barH+6);
    QStringList zones={"Мин","Низкий","Средний","Высокий","Экстрем"};
    QStringList colors={"#34d399","#86efac","#fbbf24","#f97316","#ef4444"};
    p.setFont(QFont("Arial",7));
    for(int i=0;i<=4;++i){
        double nx=bx+bw*i/4.0;
        p.setPen(QColor(colors[i]));
        p.drawText((int)nx-18,barY+barH+8,36,12,Qt::AlignCenter,zones[i]);
    }
    p.setFont(QFont("Arial",16,QFont::Bold));
    p.setPen(QColor("#e8eaf6"));
    p.drawText(0,0,w,barY-2,Qt::AlignCenter,
               QString::number(m_value,'f',2)+" / "+QString::number(m_max,'f',0));
}

AllocationBarWidget::AllocationBarWidget(const QVector<Asset> &assets,QWidget *parent)
    :QWidget(parent),m_assets(assets){}

void AllocationBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QMap<QString,double> sectors;
    double total=0;
    for(const auto &a:m_assets){double val=a.currentPrice*a.tickCount;sectors[a.sector]+=val;total+=val;}
    if(total<=0) return;
    int barH=26,barGap=10,labelW=110,valW=110;
    int barW=width()-labelW-valW-24;
    int x0=labelW+8,y=10;
    p.setFont(QFont("Arial",10,QFont::Bold));
    for(auto it=sectors.begin();it!=sectors.end();++it){
        double ratio=it.value()/total;
        int fillW=qRound(barW*ratio);
        p.setPen(QColor("#6b7280"));
        p.drawText(0,y,labelW,barH,Qt::AlignRight|Qt::AlignVCenter,it.key());
        QPainterPath bg; bg.addRoundedRect(x0,y+4,barW,barH-8,4,4);
        p.fillPath(bg,QColor("#111828"));
        if(fillW>6){
            QColor col=sectorColor(it.key());
            QLinearGradient grad(x0,0,x0+barW,0);
            grad.setColorAt(0,col.lighter(120)); grad.setColorAt(1,col.darker(130));
            QPainterPath fill; fill.addRoundedRect(x0,y+4,fillW,barH-8,4,4);
            p.fillPath(fill,grad);
        }
        p.setPen(QColor("#e8eaf6"));
        p.drawText(x0+barW+8,y,valW,barH,Qt::AlignLeft|Qt::AlignVCenter,
                   QString::number(ratio*100,'f',1)+"%  "+shortFmt(it.value())+"₽");
        y+=barH+barGap;
    }
}

CandleChartWidget::CandleChartWidget(const QVector<CandleData> &candles,QWidget *parent)
    :QWidget(parent),m_candles(candles)
{
    setMinimumSize(400,280);
    setMouseTracking(true);
    m_viewStart=qMax(0,candles.size()-100);
}

void CandleChartWidget::paintEvent(QPaintEvent *)
{
    if(m_candles.isEmpty()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath bgPath; bgPath.addRoundedRect(rect(),12,12);
    QLinearGradient bg(0,0,0,height());
    bg.setColorAt(0,QColor("#111828")); bg.setColorAt(1,QColor("#0c1020"));
    p.fillPath(bgPath,bg);

    const int PL=64,PR=14,PT=20,PB=36;
    int cw=width()-PL-PR,ch=height()-PT-PB;
    QVector<CandleData> vis=m_candles.mid(m_viewStart);
    if(vis.isEmpty()) return;

    double minP=1e18,maxP=-1e18;
    for(const auto &c:vis){minP=qMin(minP,c.low);maxP=qMax(maxP,c.high);}
    double range=maxP-minP;
    if(range<1e-9) range=1;
    auto py=[&](double price)->int{return PT+ch-(int)((price-minP)/range*ch);};

    p.setFont(QFont("Arial",7));
    for(int i=0;i<=6;++i){
        double price=minP+range*i/6.0;
        int y=py(price);
        p.setPen(QPen(QColor("#1a2340"),1,Qt::DotLine));
        p.drawLine(PL,y,PL+cw,y);
        p.setPen(QColor("#2a3555"));
        p.drawText(0,y-8,PL-4,16,Qt::AlignRight|Qt::AlignVCenter,QString::number(price,'f',1));
    }

    int n=vis.size();
    double candleW=(double)cw/n;
    double bodyW=qMax(1.5,candleW*0.68);
    for(int i=0;i<n;++i){
        const CandleData &c=vis[i];
        bool bull=c.close>=c.open;
        QColor body=bull?QColor("#34d399"):QColor("#f87171");
        QColor wick=bull?QColor("#22c47a"):QColor("#e05555");
        double cx2=PL+(i+0.5)*candleW;
        int openY=py(c.open),closeY=py(c.close);
        int highY=py(c.high),lowY=py(c.low);
        p.setPen(QPen(wick,1.2));
        p.drawLine((int)cx2,highY,(int)cx2,lowY);
        int bTop=qMin(openY,closeY);
        int bH=qMax(1,qAbs(closeY-openY));
        int bx=(int)(cx2-bodyW/2);
        if(bH>2){
            QPainterPath bp; bp.addRoundedRect(bx,bTop,(int)bodyW,bH,2,2);
            p.fillPath(bp,body);
        } else {
            p.fillRect(bx,bTop,(int)bodyW,qMax(2,bH),body);
        }
    }

    int step=qMax(1,n/8);
    p.setFont(QFont("Arial",7));
    for(int i=0;i<n;i+=step){
        double cx2=PL+(i+0.5)*candleW;
        p.setPen(QColor("#2a3555"));
        p.drawText((int)cx2-22,PT+ch+4,44,18,Qt::AlignCenter,vis[i].date.toString("dd.MM"));
    }

    if(m_hoverIdx>=0&&m_hoverIdx<n){
        double cx2=PL+(m_hoverIdx+0.5)*candleW;
        p.setPen(QPen(QColor("#4f6ef760"),1,Qt::DashLine));
        p.drawLine((int)cx2,PT,(int)cx2,PT+ch);
        const CandleData &hc=vis[m_hoverIdx];
        int closeY=py(hc.close);
        p.setPen(QPen(QColor("#4f6ef760"),1,Qt::DashLine));
        p.drawLine(PL,closeY,PL+cw,closeY);
        bool hBull=hc.close>=hc.open;
        QString tip=QString("  %1  O:%2  H:%3  L:%4  C:%5  V:%6K  ")
            .arg(hc.date.toString("dd.MM.yy"))
            .arg(hc.open,0,'f',1).arg(hc.high,0,'f',1)
            .arg(hc.low,0,'f',1).arg(hc.close,0,'f',1)
            .arg(hc.volume/1000);
        p.setFont(QFont("Arial",8,QFont::Bold));
        QFontMetrics fm(p.font());
        int tw=fm.horizontalAdvance(tip)+8;
        int tx=qBound(PL,(int)cx2-tw/2,PL+cw-tw);
        QRect tipRect(tx,PT,tw,18);
        QPainterPath tipBg; tipBg.addRoundedRect(tipRect,5,5);
        p.fillPath(tipBg,QColor("#1a2a44ee"));
        p.setPen(hBull?QColor("#34d399"):QColor("#f87171"));
        p.drawText(tipRect,Qt::AlignCenter,tip);
        QRect yMark(0,closeY-9,PL-2,18);
        p.fillRect(yMark,QColor("#4f6ef7"));
        p.setPen(Qt::white);
        p.drawText(yMark,Qt::AlignCenter,QString::number(hc.close,'f',1));
    }

    p.setPen(QPen(QColor("#1a2340"),1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(bgPath);
}

void CandleChartWidget::mouseMoveEvent(QMouseEvent *e)
{
    int n=m_candles.size()-m_viewStart;
    if(n<=0) return;
    const int PL=64,PR=14;
    int cw=width()-PL-PR;
    double cw2=(double)cw/n;
    if(cw2<=0) return;
    m_hoverIdx=qBound(0,(int)((e->pos().x()-PL)/cw2),n-1);
    update();
}

void CandleChartWidget::leaveEvent(QEvent *){m_hoverIdx=-1;update();}

TickerBarWidget::TickerBarWidget(const QVector<Asset> &assets,QWidget *parent)
    :QWidget(parent),m_assets(assets)
{
    setFixedHeight(26);
    m_timer=new QTimer(this);
    connect(m_timer,&QTimer::timeout,[this]{
        m_offset-=1;
        int totalW=m_assets.size()*180;
        if(totalW>0&&-m_offset>totalW) m_offset=0;
        update();
    });
    m_timer->start(16);
}

void TickerBarWidget::setAssets(const QVector<Asset> &a){m_assets=a;}

void TickerBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath bg; bg.addRoundedRect(rect(),8,8);
    p.fillPath(bg,QColor("#0c1020"));
    if(m_assets.isEmpty()) return;
    p.setFont(QFont("Arial",9,QFont::Bold));
    int x=m_offset;
    for(int rep=0;rep<3;++rep){
        for(const auto &a:m_assets){
            bool up=a.currentPrice>=a.prevPrice;
            double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
            p.setPen(QColor("#4f6ef7"));
            QString tk="  "+a.ticker+"  ";
            int tw=p.fontMetrics().horizontalAdvance(tk);
            p.drawText(x,0,tw,height(),Qt::AlignVCenter,tk);
            x+=tw;
            p.setPen(up?QColor("#34d399"):QColor("#f87171"));
            QString rest=QString("%1  %2%3%  ·")
                .arg(QString::number(a.currentPrice,'f',2))
                .arg(up?"▲+":"▼").arg(pct,0,'f',2);
            int rw=p.fontMetrics().horizontalAdvance(rest);
            p.drawText(x,0,rw,height(),Qt::AlignVCenter,rest);
            x+=rw+6;
        }
    }
}

SummaryRowWidget::SummaryRowWidget(const Asset &a,QWidget *parent):QWidget(parent)
{
    setFixedHeight(50); setObjectName("calRow");
    auto *l=new QHBoxLayout(this); l->setContentsMargins(12,6,12,6); l->setSpacing(0);
    bool up=a.currentPrice>=a.prevPrice;
    double pct=a.prevPrice>0?(a.currentPrice-a.prevPrice)/a.prevPrice*100.0:0.0;
    auto addCol=[&](const QString &t,int s,const QString &style,
                    Qt::Alignment align=Qt::AlignLeft|Qt::AlignVCenter){
        auto *lb=new QLabel(t); lb->setStyleSheet(style);
        lb->setAlignment(align); l->addWidget(lb,s);
    };
    addCol(a.ticker,1,"color:#93c5fd;font-size:11px;font-weight:800;");
    addCol(a.name,2,"color:#6b7280;font-size:11px;");
    addCol(QString::number(a.tickCount),1,"color:#e8eaf6;font-size:11px;",Qt::AlignCenter);
    addCol(QString::number(a.currentPrice,'f',1)+" ₽",2,
           QString("color:%1;font-size:12px;font-weight:700;").arg(up?"#34d399":"#f87171"),
           Qt::AlignRight|Qt::AlignVCenter);
    addCol((up?"▲ +":"▼ ")+QString::number(pct,'f',2)+"%",1,
           QString("color:%1;font-size:11px;font-weight:700;").arg(up?"#34d399":"#f87171"),
           Qt::AlignRight|Qt::AlignVCenter);
    addCol(fmtRub(a.profit),2,
           QString("color:%1;font-size:11px;").arg(a.profit>=0?"#34d399":"#f87171"),
           Qt::AlignRight|Qt::AlignVCenter);
}

NewsCardWidget::NewsCardWidget(const NewsItem &n,QWidget *parent):QWidget(parent)
{
    setObjectName("newsCard");
    setStyleSheet("QWidget#newsCard{background:#111828;border-radius:12px;"
                  "border-left:4px solid #4f6ef7;}");
    auto *l=new QVBoxLayout(this); l->setContentsMargins(16,10,16,10); l->setSpacing(5);
    auto *hdr=new QHBoxLayout;
    auto *src=new QLabel("📰  "+n.source);
    src->setStyleSheet("color:#4f6ef7;font-size:11px;font-weight:800;");
    auto *tick=new QLabel("["+n.relatedTicker+"]");
    tick->setStyleSheet("color:#34d399;font-size:10px;font-weight:700;");
    auto *date=new QLabel(n.date.toString("dd MMMM yyyy"));
    date->setStyleSheet("color:#2a3555;font-size:10px;");
    date->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    hdr->addWidget(src); hdr->addSpacing(8); hdr->addWidget(tick);
    hdr->addStretch(); hdr->addWidget(date);
    auto *title=new QLabel(n.title);
    title->setStyleSheet("color:#e8eaf6;font-size:13px;font-weight:700;");
    title->setWordWrap(true);
    auto *body=new QLabel(n.body);
    body->setStyleSheet("color:#4b5563;font-size:11px;");
    body->setWordWrap(true);
    l->addLayout(hdr); l->addWidget(title); l->addWidget(body);
}