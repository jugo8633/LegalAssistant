#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWebEngineWidgets/QWebEngineView>
#include <QUrl>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    webview=new QWebEngineView(this);
    //webview->load(QUrl("http://www.google.com"));
    //webview->load(QUrl("file:///github/LegalAssistant/page/example/index.htm"));
    webview->setUrl(QUrl::fromLocalFile("/github/LegalAssistant/page/example/index.htm"));
    webview->resize(800,600);
    webview->show();
    setCentralWidget(webview);

//    player = new QMediaPlayer;
//    playlist = new QMediaPlaylist(player);
//    playlist->addMedia(QUrl::fromLocalFile("/github/LegalAssistant/page/example/hop.ogg"));
//    videoWidget = new QVideoWidget(this);
//    player->setVideoOutput(videoWidget);
//    videoWidget->show();
//    playlist->setCurrentIndex(0);
//    player->play();
//    setCentralWidget(videoWidget);

}

MainWindow::~MainWindow()
{
    delete webview;
    delete ui;
}
