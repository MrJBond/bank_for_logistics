QT       += core  sql pdf charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/QtRptProject/QtRPT/QtRPT.pri)

SOURCES += \
    AI/chatbot.cpp \
    AI/loanrecommender.cpp \
    AI/networkmanager.cpp \
    auth/usersession.cpp \
    db/accountrepository.cpp \
    db/loanrepository.cpp \
    db/transactionrepository.cpp \
    entities/account.cpp \
    entities/loan.cpp \
    entities/transaction.cpp \
    service/abstractservice.cpp \
    service/accountservice.cpp \
    service/clientservice.cpp \
    db/abstractrepository.cpp \
    db/clientrepository.cpp \
    db/dbconnector.cpp \
    entities/client.cpp \
    entities/entity.cpp \
    main.cpp \
    mainwindow.cpp \
    service/loanservice.cpp \
    service/transactionservice.cpp

HEADERS += \
    AI/chatbot.h \
    AI/loanrecommender.h \
    AI/networkmanager.h \
    auth/usersession.h \
    db/accountrepository.h \
    db/loanrepository.h \
    db/transactionrepository.h \
    entities/account.h \
    entities/loan.h \
    entities/transaction.h \
    service/abstractservice.h \
    service/accountservice.h \
    service/clientservice.h \
    db/abstractrepository.h \
    db/clientrepository.h \
    db/dbconnector.h \
    entities/client.h \
    entities/entity.h \
    mainwindow.h \
    service/loanservice.h \
    service/transactionservice.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES += \
    AI/train_bot.py \
    AI/loan_recommender.py
