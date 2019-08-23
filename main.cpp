#include "mainwindow.h"
#include <QApplication>
#include <QtWebView/QtWebView>
#include<QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWebView::initialize();

    MainWindow w;
    w.showMaximized();
    //w.show();


    qDebug() << "Main Window Show";

    return a.exec();
}
