#-------------------------------------------------
#
# Project created by QtCreator 2023-12-03T14:17:15
#
#-------------------------------------------------

QT       += core gui svg qml xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 file_copies

COPIES += samplesvg

samplesvg.files = $$files(sample.svg)
samplesvg.path = $$OUT_PWD

TARGET = parametricsvg
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    parametricsvgitem/parametricsvgitem.cpp

HEADERS  += mainwindow.h \
    parametricsvgitem/parametricsvgitem.h

FORMS    += mainwindow.ui
