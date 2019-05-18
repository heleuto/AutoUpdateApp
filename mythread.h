#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QSqlDatabase>
#include "include/qtsoap.h"

//用于更新mysql数据库
class MyThread : public QThread
{
    Q_OBJECT
public:
    MyThread(QString name="", QObject *parent =nullptr);
    volatile bool mStop;
protected:
    void run() override;
private:
    bool databaseInit();
    void CloseDB();
signals:
    void updateTmpFininshed();
    void dbOpenState(bool);
private slots:
    void keepAlive();
    void getResponse();
    void checkAndUpdateDatabase();
public slots:

private:
    QTime time;
    QDateTime dateTime;
    QtSoapHttpTransport http;
    QString connectName;
    int alivenum;
    bool updated;
};

#endif // MYTHREAD_H
