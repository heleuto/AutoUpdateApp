#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QIcon>
#include <QSystemTrayIcon>
#include "mythread.h"

class QAction;
class QMenu;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_closeBtn_clicked();

    void on_unlockBtn_clicked();

    void myIconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_factorySetBtn_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

    void setOkBtnEnable();
    void showDbOpenState(bool);
public:
    static QString mysql_ip;
    static quint32 mysql_port;
    static QString mysql_username;
    static QString mysql_userpasswd;
    static QString mysql_databasename;

    static QString WebService_ip;
    static quint32 WebService_port;
    static QString WebService_addr;

    static int update_time;//更新时间,默认3点
private:
    Ui::Widget *ui;
    bool isLocked;
    QString lockStr;

    QSystemTrayIcon *trayIcon;
    QIcon icon;
    QAction *showAction, *quitAction;
    QMenu *trayIconMenu;
    void readProfiles();
    void writeProfiles();
    void writeDefaultProfiles();

    //MyThread *upThread;
    void writeFactory();
    int readFactory();
    void lockedBtns();
    QString sqlconnectName;
    int connectTimes;
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
};

#endif // WIDGET_H
