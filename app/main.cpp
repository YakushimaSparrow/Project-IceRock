#include <QApplication>
#include <QFont>
#include <QProcessEnvironment>
#include "mainwindow.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Arial", 10));

    // -------------------------------------------------------
    // Токен Tinkoff Sandbox передаётся через аргумент в CLion:
    //   Run → Edit Configurations → Program arguments:
    //   t.ВашТокен
    //
    // Или через переменную окружения TINKOFF_SANDBOX_TOKEN.
    // Без токена — режим симуляции (без сети).
    // -------------------------------------------------------
    QString token;
    if (argc > 1)
        token = QString::fromUtf8(argv[1]);
    else
        token = QProcessEnvironment::systemEnvironment()
                    .value("TINKOFF_SANDBOX_TOKEN");

    MainWindow w(token);
    w.show();
    return app.exec();
}
