#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QChartView;
class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUI();
    void setupPieChart();
    void setupLineChart();
    void setupRiskIndicator();

    QChartView *pieChartView;
    QChartView *lineChartView;
    QLabel *riskLabel;
    QProgressBar *riskBar;
};

#endif // MAINWINDOW_H