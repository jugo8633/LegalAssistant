#-------------------------------------------------
#
# Project created by QtCreator 2019-08-22T16:12:03
#
#-------------------------------------------------

QT       += core gui webview webenginewidgets multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LegalAssistant
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        Debug/moc_mainwindow.cpp \
        logHandler/LogHandler.cpp \
        main.cpp \
        mainwindow.cpp \
        messageHandler/CMessageHandler.cpp \
        objectHandler/CObject.cpp \
        objectHandler/CThread.cpp \
        objectHandler/CTimer.cpp \
        objectHandler/Handler.cpp \
        socketHandler/CATcpClient.cpp \
        socketHandler/CATcpServer.cpp \
        socketHandler/CSocket.cpp \
        socketHandler/CSocketClient.cpp \
        socketHandler/CSocketServer.cpp

HEADERS += \
        Debug/moc_predefs.h \
        Debug/ui_mainwindow.h \
        global_inc/ICallback.h \
        global_inc/common.h \
        global_inc/container.h \
        global_inc/dataType.h \
        global_inc/event.h \
        global_inc/packet.h \
        global_inc/utf8.h \
        global_inc/utility.h \
        logHandler/LogHandler.h \
        mainwindow.h \
        messageHandler/CMessageHandler.h \
        objectHandler/CObject.h \
        objectHandler/CThread.h \
        objectHandler/CTimer.h \
        objectHandler/Handler.h \
        socketHandler/CATcpClient.h \
        socketHandler/CATcpServer.h \
        socketHandler/CSocket.h \
        socketHandler/CSocketClient.h \
        socketHandler/CSocketServer.h \
        socketHandler/IReceiver.h \
        ui_mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    page/example/hop.avi \
    page/example/hop.gif \
    page/example/hop.mp4 \
    page/example/hop.ogg \
    page/example/index.htm \
    page/example/poster.png \
    page/night-club/images/content.gif \
    page/night-club/images/dj.jpg \
    page/night-club/images/footer.gif \
    page/night-club/images/header.gif \
    page/night-club/images/logo.gif \
    page/night-club/images/menu.gif \
    page/night-club/images/middle.gif \
    page/night-club/images/middle2.gif \
    page/night-club/images/photo.jpg \
    page/night-club/images/welcome.gif \
    page/night-club/index.html \
    page/night-club/style.css
