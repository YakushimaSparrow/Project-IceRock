#include <iostream>
#include <vector>
#include <cassert>
#include <QApplication>
#include "mainwindow.h"
#include "R.hpp"
#include "logger.hpp"

int main(int argc, char *argv[]){

    QApplication app(argc, argv);

    assert(1 == calculateMaxDragdown({1, 2, 3}));

    Logger::getInstance("logs/logbook.txt").log("Test", LoggerLevel::INFO);
    Logger::getInstance().log("Test2", LoggerLevel::INFO);

    app.setWindowIcon(QIcon("src/icons/avx1d-0ksyk.icns"));
    MainWindow w;
    w.showMaximized();
    return app.exec();
}