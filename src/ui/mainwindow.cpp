#include "mainwindow.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPainter>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("RiskAlyzer");
    setupUI();
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *root = new QHBoxLayout(central);

    setupPieChart();
    setupLineChart();
    setupRiskIndicator();

    QVBoxLayout *chartsLayout = new QVBoxLayout();
    chartsLayout->addWidget(pieChartView);
    chartsLayout->addWidget(lineChartView);

    QVBoxLayout *riskLayout = new QVBoxLayout();
    riskLayout->addWidget(new QLabel("Portfolio Risk"));
    riskLayout->addWidget(riskLabel);
    riskLayout->addWidget(riskBar);
    riskLayout->addStretch();

    root->addLayout(chartsLayout, 3);
    root->addLayout(riskLayout, 1);
}

void MainWindow::setupPieChart()
{
    QPieSeries *series = new QPieSeries();
    series->append("Stocks", 65);
    series->append("Bonds", 25);
    series->append("Cash", 10);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Portfolio Allocation");
    chart->setBackgroundBrush(QBrush(QColor("#1e1e1e")));
    chart->legend()->setLabelColor(Qt::lightGray);

    pieChartView = new QChartView(chart);
    pieChartView->setRenderHint(QPainter::Antialiasing);
    pieChartView->setMaximumHeight(350);
}

void MainWindow::setupLineChart()
{
    QLineSeries *series = new QLineSeries();
    series->append(0, 100);
    series->append(1, 102);
    series->append(2, 101);
    series->append(3, 105);
    series->append(4, 108);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Portfolio Price");
    chart->setBackgroundBrush(QBrush(QColor("#1e1e1e")));
    chart->legend()->hide();

    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    lineChartView = new QChartView(chart);
    lineChartView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::setupRiskIndicator()
{
    int risk = 72;

    QString color = risk < 40 ? "#2ecc71" :
                    risk < 70 ? "#f1c40f" :
                                "#e74c3c";

    riskLabel = new QLabel(QString::number(risk) + " %");
    riskLabel->setAlignment(Qt::AlignCenter);
    riskLabel->setStyleSheet(
        "QLabel {"
        "border: 2px solid " + color + ";"
        "color: " + color + ";"
        "font-size: 26px;"
        "font-weight: bold;"
        "padding: 10px;"
        "}"
    );

    riskBar = new QProgressBar();
    riskBar->setRange(0, 100);
    riskBar->setValue(risk);
    riskBar->setTextVisible(false);
}