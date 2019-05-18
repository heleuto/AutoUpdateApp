#include "widget.h"
#include "ui_widget.h"
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QMessageBox>
#include <QTimer>
#include <QDesktopWidget>

//此处定义建议和ui文件的值一样
QString Widget::mysql_ip = "192.168.0.133";
QString Widget::mysql_username ="root";
QString Widget::mysql_userpasswd = "123456";
QString Widget::mysql_databasename = "mydatabase";
QString Widget::WebService_ip = "192.168.0.133";
QString Widget::WebService_addr = "/myweb/WebService1.asmx";
quint32 Widget::mysql_port = 3306;
quint32 Widget::WebService_port = 80;
int     Widget::update_time = 3;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),isLocked(true),connectTimes(0)
{
    ui->setupUi(this);
    writeFactory();     //写入出厂配置

    lockStr = ui->unlockBtn->text();
    lockedBtns();
    this->setWindowTitle(tr("后台更新配置"));

    QRegExp regx("[0-9]+$");
    QValidator* validator = new QRegExpValidator(regx, ui->mysql_portLineEdit);
    ui->mysql_portLineEdit->setValidator(validator);

    QValidator* validator_2 = new QRegExpValidator(regx, ui->webservice_portLineEdit);
    ui->webservice_portLineEdit->setValidator(validator_2);

    icon = QIcon(":/img/Update.png");
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip(tr("单击显示主界面"));
    trayIcon->setIcon(icon);
    trayIcon->setVisible(true);
    showAction = new QAction(tr("显示主界面"),this);
    quitAction = new QAction(tr("退出"),this);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    connect(showAction,&QAction::triggered,this,&Widget::show);
    connect(quitAction,&QAction::triggered,qApp,&QApplication::quit);
    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIcon,&QSystemTrayIcon::activated,this,&Widget::myIconActivated);
    readProfiles();

    //开启更新线程,mysql不能跨线程连接

    sqlconnectName="recy_";//循环连接名，1-100
    QString name = sqlconnectName + QString::number(connectTimes);
    MyThread *upThread = new MyThread(name);//default
    connect(upThread,&MyThread::finished,upThread,&MyThread::deleteLater);
    connect(ui->pushButton_3,&QPushButton::clicked,upThread,&MyThread::quit);
    upThread->start();

}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_closeBtn_clicked()
{
    close();
}

void Widget::on_unlockBtn_clicked()
{
    if(isLocked)
    {
        isLocked = !isLocked;
        ui->unlockBtn->setText(tr("锁定配置"));
        ui->groupBox->setEnabled(true);
        ui->groupBox_2->setEnabled(true);
        ui->groupBox_3->setEnabled(true);
        ui->factorySetBtn->setEnabled(true);
    }else{
        isLocked = !isLocked;
        ui->unlockBtn->setText(lockStr);
        ui->groupBox->setEnabled(false);
        ui->groupBox_2->setEnabled(false);
        ui->groupBox_3->setEnabled(false);
        ui->factorySetBtn->setEnabled(false);
    }
}

void Widget::myIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        this->show();
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    default:
        break;
    }
}

void Widget::readProfiles()
{
    QString dirStr = QApplication::applicationDirPath();
    QString configIni="/config.ini";
    QString configIniWhole = dirStr +configIni;

    QFile fileToRead(configIniWhole);

    if(!fileToRead.exists())
    {
        writeDefaultProfiles();//写入默认配置
        return;
    }

    QSettings set(configIniWhole,QSettings::IniFormat);

    QString sql_ip = set.value("SYSTEM/mysql_ip").toString();
    QString sql_username = set.value("SYSTEM/mysql_username").toString();
    QString sql_databasename = set.value("SYSTEM/mysql_databasename").toString() ;
    quint32 sql_prot = set.value("SYSTEM/mysql_port").toUInt();
    QString sql_passwd = set.value("SYSTEM/mysql_userpasswd").toString();
    QString myServer_ip = set.value("SYSTEM/WebService_ip").toString();
    quint32 myServer_port = set.value("SYSTEM/WebService_port").toUInt();
    QString myServer_addr = set.value("SYSTEM/WebService_addr").toString();
    qint32 up_time = set.value("SYSTEM/update_time").toInt() ;

    if(sql_ip.isEmpty())
    {
        set.setValue("SYSTEM/mysql_ip",mysql_ip);
        ui->mysql_ipLineEdit->setText(mysql_ip);
    }else{
        ui->mysql_ipLineEdit->setText(sql_ip);
        mysql_ip = sql_ip;
    }

    if(sql_username.isEmpty())
    {
        set.setValue("SYSTEM/mysql_username",mysql_username);
        ui->mysql_usernameLineEdit->setText(mysql_username);
    }else{
        ui->mysql_usernameLineEdit->setText(sql_username);
        mysql_username =sql_username;
    }

    if(sql_passwd.isEmpty())
    {
        set.setValue("SYSTEM/mysql_userpasswd",mysql_userpasswd);
        ui->mysql_passwdLineEdit->setText(mysql_userpasswd);
    }else{
        ui->mysql_passwdLineEdit->setText(sql_passwd);
        mysql_userpasswd = sql_passwd;
    }

    if(sql_databasename.isEmpty())
    {
        set.setValue("SYSTEM/mysql_databasename",mysql_databasename);
        ui->mysql_databasenameLineEdit->setText(mysql_databasename);
    }else{
        ui->mysql_databasenameLineEdit->setText(sql_databasename);
        mysql_databasename = sql_databasename;
    }

    if(myServer_ip.isEmpty())
    {        
        set.setValue("SYSTEM/WebService_ip",WebService_ip);
        ui->webservice_ipLineEdit->setText(WebService_ip);
    }else{
        ui->webservice_ipLineEdit->setText(myServer_ip);
        WebService_ip =myServer_ip;
    }

    if(myServer_addr.isEmpty())
    {        
        set.setValue("SYSTEM/WebService_addr",WebService_addr);
        ui->webservice_addrLineEdit->setText(WebService_addr);
    }else{
        ui->webservice_addrLineEdit->setText(myServer_addr);
        WebService_addr = myServer_addr;
    }

    if(set.value("SYSTEM/mysql_port").toString().isEmpty())
    {
        set.setValue("SYSTEM/mysql_port",mysql_port);
        ui->mysql_portLineEdit->setText(QString::number( mysql_port));
    }else{
        ui->mysql_portLineEdit->setText(QString::number( sql_prot));
        mysql_port = sql_prot;
    }

    if(set.value("SYSTEM/WebService_port").toString().isEmpty())
    {

        set.setValue("SYSTEM/WebService_port",WebService_port);
        ui->webservice_portLineEdit->setText(QString::number(WebService_port));
    }else{
        ui->webservice_portLineEdit->setText(QString::number(myServer_port));
        WebService_port = myServer_port;
    }

    if(update_time < 0 || update_time > 6 || set.value("SYSTEM/update_time").toString().isEmpty())
    {
        ui->spinBox->setValue(update_time);
    }else{
        ui->spinBox->setValue(up_time);
        update_time = up_time;
    }
}

void Widget::writeProfiles()
{
    qDebug()<<"writing config";
    QString dirStr = QApplication::applicationDirPath();
    QString configIni="/config.ini";
    QString configIniWhole = dirStr +configIni;
    QSettings set(configIniWhole,QSettings::IniFormat);

    set.setValue("SYSTEM/mysql_ip",ui->mysql_ipLineEdit->text());
    set.setValue("SYSTEM/mysql_username",ui->mysql_usernameLineEdit->text());
    set.setValue("SYSTEM/mysql_userpasswd",ui->mysql_passwdLineEdit->text());
    set.setValue("SYSTEM/mysql_databasename",ui->mysql_databasenameLineEdit->text());
    set.setValue("SYSTEM/WebService_ip",ui->webservice_ipLineEdit->text());
    set.setValue("SYSTEM/WebService_addr",ui->webservice_addrLineEdit->text());

    set.setValue("SYSTEM/mysql_port",ui->mysql_portLineEdit->text());
    set.setValue("SYSTEM/WebService_port",ui->webservice_portLineEdit->text());
    set.setValue("SYSTEM/update_time",ui->spinBox->value());
}

//此函数只能在没有inf文件时调用一次，有inf文件后不允许调用
void Widget::writeDefaultProfiles()
{
    QString dirStr = QApplication::applicationDirPath();
    QString configIni="/config.ini";
    QString configIniWhole = dirStr +configIni;
    QSettings set(configIniWhole,QSettings::IniFormat);

    set.setValue("SYSTEM/mysql_ip",ui->mysql_ipLineEdit->text());
    set.setValue("SYSTEM/mysql_username",ui->mysql_usernameLineEdit->text());
    set.setValue("SYSTEM/mysql_userpasswd",ui->mysql_passwdLineEdit->text());
    set.setValue("SYSTEM/mysql_databasename",ui->mysql_databasenameLineEdit->text());
    set.setValue("SYSTEM/WebService_ip",ui->webservice_ipLineEdit->text());
    set.setValue("SYSTEM/WebService_addr",ui->webservice_addrLineEdit->text());

    set.setValue("SYSTEM/mysql_port",ui->mysql_portLineEdit->text());
    set.setValue("SYSTEM/WebService_port",ui->webservice_portLineEdit->text());
    set.setValue("SYSTEM/update_time",ui->spinBox->value());

    readProfiles();//重新载入
}

//写入出厂配置，配置内容为ui文件的原始输入数据
//必须先调用此函数再读取Inf配置文件
void Widget::writeFactory()
{
    //判断出厂配置文件是否存在，不存在，则创建
    QString factoryData="factory.dat";
    QFile file(factoryData);
    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QDataStream out(&file);
        QString sql_ip = ui->mysql_ipLineEdit->text();
        QString sql_username = ui->mysql_usernameLineEdit->text();
        QString sql_databasename = ui->mysql_databasenameLineEdit->text();
        quint32 sql_prot = ui->mysql_portLineEdit->text().toUInt();
        QString sql_passwd = ui->mysql_passwdLineEdit->text();
        QString myServer_ip = ui->webservice_ipLineEdit->text();
        quint32 myServer_port = ui->webservice_portLineEdit->text().toUInt();
        QString myServer_addr = ui->webservice_addrLineEdit->text();
        qint32 up_time =  ui->spinBox->value();
        out << sql_ip << sql_username << sql_databasename << sql_prot << sql_passwd << myServer_ip
            << myServer_port << myServer_addr << up_time;
    }
}

//返回值
//-1：文件不存在
//0：正常读取
//因不做判断，不许保证读取时和写入时的位置一一对应,先导出到局部变量，确认修改后，再写入成员变量
int Widget::readFactory()
{
    QString factoryData="factory.dat";
    QFile file(factoryData);
    if(file.exists())
    {
        file.open(QIODevice::ReadOnly);
        QDataStream in(&file);    // read the data serialized from the file

        QString sql_ip ;
        QString sql_username ;
        QString sql_databasename ;
        quint32 sql_prot ;
        QString sql_passwd ;
        QString myServer_ip;
        quint32 myServer_port ;
        QString myServer_addr ;
        qint32 up_time;

        in >> sql_ip >> sql_username >> sql_databasename >> sql_prot >> sql_passwd
                >> myServer_ip >> myServer_port >> myServer_addr >> up_time;

        ui->mysql_ipLineEdit->setText(sql_ip);
        ui->mysql_usernameLineEdit->setText(sql_username);
        ui->mysql_databasenameLineEdit->setText(sql_databasename);
        ui->mysql_portLineEdit->setText(QString::number( sql_prot));
        ui->mysql_passwdLineEdit->setText(sql_passwd);
        ui->webservice_ipLineEdit->setText(myServer_ip);
        ui->webservice_portLineEdit->setText(QString::number(myServer_port));
        ui->webservice_addrLineEdit->setText(myServer_addr);
        ui->spinBox->setValue(up_time);
    }else{
        return -1;
    }
    return 0;
}

void Widget::lockedBtns()
{
    isLocked = true;
    ui->groupBox->setEnabled(false);
    ui->groupBox_2->setEnabled(false);
    ui->groupBox_3->setEnabled(false);
    ui->unlockBtn->setText(lockStr);
}

void Widget::closeEvent(QCloseEvent *event)
{
    if(trayIcon->isVisible())
    {
        hide();
    }
    event->ignore();
}

void Widget::showEvent(QShowEvent *event)
{
    readProfiles();//hua还原为上次配置
    lockedBtns();
}

//还原出厂设置
void Widget::on_factorySetBtn_clicked()
{
    int ret = QMessageBox::warning(this,"警告","所有配置将会被还原为出厂配置",QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes)   readFactory();
}

//撤销修改，返回上次点击确定时的配置,配置信息从inf文件读取
void Widget::on_pushButton_4_clicked()
{
    int  ret =QMessageBox::information(this,"温馨提示!","还原为上次保存的配置!\r\n本次未保存的配置将丢失!\r\n",QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes)    readProfiles();
}
//确认修改，撤销修改以这次为准,并重新启动升级线程,将此次配置写入inf文件
void Widget::on_pushButton_3_clicked()
{
    QString sql_ip = ui->mysql_ipLineEdit->text();
    QString sql_username = ui->mysql_usernameLineEdit->text();
    QString sql_databasename = ui->mysql_databasenameLineEdit->text();

    QString sql_port_str = ui->mysql_portLineEdit->text();
    quint32 sql_prot = ui->mysql_portLineEdit->text().toUInt();

    QString sql_passwd = ui->mysql_passwdLineEdit->text();
    QString myServer_ip = ui->webservice_ipLineEdit->text();

    QString myServer_port_str = ui->webservice_portLineEdit->text();
    quint32 myServer_port = ui->webservice_portLineEdit->text().toUInt();

    QString myServer_addr = ui->webservice_addrLineEdit->text();

    if(sql_ip.isEmpty() || sql_username.isEmpty() || sql_databasename.isEmpty() || sql_port_str.isEmpty() || sql_passwd.isEmpty()
            || myServer_ip.isEmpty() || myServer_port_str.isEmpty() || myServer_addr.isEmpty())
    {
        QMessageBox::critical(this,tr("错误提示"),tr("输入错误!\r\n内容不能为空！\r\n请检查输入的内容后\r\n重新输入"),QMessageBox::Ok);
        return;
    }

    //判断IP地址输入是否有误
    QRegExp rx2("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
    if( !rx2.exactMatch(sql_ip) )
    {
        QMessageBox::critical(this,tr("错误提示"),tr("MySQL数据库IP填写错误!\r\n请重新输入\r\n"),QMessageBox::Ok);
        return;
    }
    if( !rx2.exactMatch(myServer_ip) )
    {
        QMessageBox::critical(this,tr("错误提示"),tr("WebService的IP填写错误!\r\n请重新输入\r\n"),QMessageBox::Ok);
        return;
    }
    ui->pushButton_3->setEnabled(false);//防止重复点击
    mysql_ip = sql_ip;
    mysql_port = sql_prot;
    mysql_username = sql_username;
    mysql_userpasswd = sql_passwd;
    mysql_databasename = sql_databasename;

    WebService_ip = myServer_ip;
    WebService_port = myServer_port;
    WebService_addr = myServer_addr;
    update_time = ui->spinBox->value();

    qDebug()<<"mysql_ip"<<mysql_ip;
    qDebug()<<"mysql_port"<<mysql_port;
    qDebug()<<"mysql_username"<<mysql_username;
    qDebug()<<"mysql_userpasswd"<<mysql_userpasswd;
    qDebug()<<"mysql_databasename"<<mysql_databasename;
    qDebug()<<"WebService_ip"<<WebService_ip;
    qDebug()<<"WebService_port"<<WebService_port;
    qDebug()<<"WebService_addr"<<WebService_addr;
    qDebug()<<"update_time"<<update_time;

    writeProfiles();

    connectTimes ++;
    if(connectTimes > 100)
    {
        connectTimes = 0;
    }

    // 每次链接采用新连接名，除非用户能1秒按确认按钮100次
    QString name = sqlconnectName + QString::number(connectTimes);
    MyThread *upThread = new MyThread(name);//重新创建数据库连接
    connect(upThread,&MyThread::finished,upThread,&MyThread::deleteLater);
    connect(ui->pushButton_3,&QPushButton::clicked,upThread,&MyThread::quit);
    connect(upThread,&MyThread::dbOpenState,this,&Widget::showDbOpenState);
    connect(upThread,&MyThread::started,this,&Widget::setOkBtnEnable);//防止重复点击

    QMessageBox msg;
    msg.setWindowTitle("提示");
    msg.setText("后台升级程序正在启动\r\n请稍后!\r\n");
    QTimer::singleShot(3000,&msg,&QMessageBox::close);
    connect(upThread,&QThread::started,&msg,&QMessageBox::close);

    upThread->start(); //重启线程
    msg.exec();
}

void Widget::setOkBtnEnable()
{
    ui->pushButton_3->setEnabled(true);
}

void Widget::showDbOpenState(bool ret)
{
    QDesktopWidget* desktop =qApp->desktop();
    if(ret)
    {
        QMessageBox msg;
        msg.setWindowTitle("提示");
        msg.setInformativeText(tr("数据库打开成功\r\n数据库正在准备升级!\r\n"));
        msg.setStandardButtons(QMessageBox::Ok);
        msg.move((desktop->width() /*- msg.width()*/)/2,(desktop->height() /*- msg.height()*/)/2);
        //msg.setGeometry(0,0,480,360);
        QTimer::singleShot(5000,&msg,&QMessageBox::close);
        msg.exec();
    }else{
        QMessageBox msg;
        msg.setWindowTitle("警告");
        msg.setText(tr("数据库打开失败!\r\n"));
        msg.setStandardButtons(QMessageBox::Ok);
        //msg.setGeometry(0,0,480,360);
        msg.move((desktop->width() - msg.width())/2,(desktop->height() - msg.height())/2);
        QTimer::singleShot(5000,&msg,&QMessageBox::close);
        msg.exec();
    }
}
