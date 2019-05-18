#include "mythread.h"
#include "widget.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

MyThread::MyThread(QString name,QObject *parent):QThread (parent),mStop(false)/*,selectTimer(NULL),db(NULL)*/,updated(false)
{
    if(name.isEmpty())
    {
        connectName = "qt_sql_default_connection";//采用默认连接名
    }else{
        connectName = name;
    }
    alivenum =0;//心跳倒计时
}

void MyThread::run()
{
    bool ret = databaseInit();

    emit dbOpenState(ret);
    if(!ret)    return;

    QTimer *selectTimer = new QTimer();
    connect(selectTimer,&QTimer::timeout,this,&MyThread::keepAlive);
    connect(this,&QThread::finished,selectTimer,&QObject::deleteLater);
    connect(&http, SIGNAL(responseReady()), this, SLOT(getResponse()));
    connect(this,SIGNAL(updateTmpFininshed()),this,SLOT(checkAndUpdateDatabase()));
    selectTimer->start(60 * 1000);//one minute
    exec();

    CloseDB();//必须在此处移除数据库
}

bool MyThread::databaseInit()
{
    bool ret = false;
    QSqlDatabase db;
    if(db.contains(connectName))
    {
        db =QSqlDatabase::database(connectName);
    }else {
        db =QSqlDatabase::addDatabase("QMYSQL",connectName);
        db.setDatabaseName(Widget::mysql_databasename);
        db.setHostName(Widget::mysql_ip);
        db.setUserName(Widget::mysql_username);
        db.setPassword(Widget::mysql_userpasswd);
        db.setPort(Widget::mysql_port);
    }

    ret = db.open();

    return ret;
}

//保持心跳,15分钟一次心跳,再判断是否需要升级
void MyThread::keepAlive()
{
    alivenum++;
    if(alivenum > 14)
    {
        alivenum =0;
        QSqlDatabase db = QSqlDatabase::database(connectName);
        QSqlQuery mQuery(db);
        if(!mQuery.exec("select 1"))
        {
            if(databaseInit())
            {
                mQuery= QSqlQuery(db);
            }
        }
        time = QTime::currentTime();
        if((time.hour() == 2 ))
        {
            if(updated) return;
            //开始更新
            QtSoapMessage request;
            dateTime = QDateTime::currentDateTime();

            //country自定义数据库查询语句
            QString country ="";
            request.setMethod("reader", "http://tempuri.org/");
            request.addMethodArgument("ssql", "", country);

            http.setHost(Widget::WebService_addr,(int)Widget::WebService_port);
            http.setAction("http://tempuri.org/reader");
            http.submitRequest(request, "/myweb/WebService1.asmx");
        }else{
            updated = false;
        }
    }
}

void MyThread::getResponse()
{
    //正式
    QSqlDatabase db = QSqlDatabase::database(connectName);
    QString sql_str = QString("select 1");
    QSqlQuery test_sql_query(sql_str,db);

    test_sql_query.prepare(sql_str);
    if (!test_sql_query.isActive()) {
        if(databaseInit())       test_sql_query = QSqlQuery(db);
    }

    QString result_sql = QString("INSERT INTO mlog (start_time, finish_time, update_result,remarks) "
                                 "VALUES (?, ?, ? , ?)");

    const QtSoapMessage &message = http.getResponse();

    if (message.isFault())
    {
        qDebug("Error: %s", message.faultString().value().toString().toLatin1().constData());
        qDebug()<<tr("网页服务器连接失败！");
        test_sql_query.clear();
        test_sql_query.prepare(result_sql);
        test_sql_query.addBindValue(dateTime.toString("yyyy-MM-dd hh:mm:ss"));
        test_sql_query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        test_sql_query.addBindValue(3);
        test_sql_query.addBindValue(tr("网页服务器连接失败"));
        test_sql_query.exec();
        //emit updateErrorCode(3);
        return;
    }

    qDebug()<<tr("网页服务器连接成功！");

    //自己解析xml
    QString strXML = message.toXmlString();
    //qDebug() << strXML;

    QDomDocument doc;
    doc.setContent(strXML);



    updated = true;
    QSqlQuery sql_query(db);
    QString insert_sql;

    //删除原表
    insert_sql = "delete from tmp";
    sql_query.prepare(insert_sql);
    if(!sql_query.exec())
    {
        qDebug() << "Error: Fail to delete tmp table." << sql_query.lastError();
    }
    else
    {
        qDebug() << "tmp Table delete!";
    }

    QString null_str="";

    QSqlDatabase::database(connectName).transaction();

    //插入自己补全

    if(QSqlDatabase::database(connectName).commit())
    {
        emit updateTmpFininshed();
        qDebug()<<tr("用户数据库更新临时表格成功！");
    }else{
        QSqlDatabase::database(connectName).rollback();
        qDebug()<<tr("用户数据库更新临时表格失败！");
        return;
    }
}

void MyThread::checkAndUpdateDatabase()
{
    QSqlDatabase db = QSqlDatabase::database(connectName);
    QString result_sql = QString("INSERT INTO mlog (start_time, finish_time, update_result,remarks) "
                                 "VALUES (?, ?, ? , ?)");
    QSqlQuery chkQuery("select count(*) from tmp",db);
    chkQuery.exec();
    if(chkQuery.next())
    {
        int chkNum = chkQuery.value(0).toInt();
        qDebug()<<"chkNum:"<<chkNum;
        if(chkNum > 1)
        {
            //判断零时表是否是空的，再更新目标表
        }else{
            qDebug()<<"Error Table:tmp_person";
            chkQuery.clear();
            chkQuery.prepare(result_sql);
            chkQuery.addBindValue(dateTime.toString("yyyy-MM-dd hh:mm:ss"));
            chkQuery.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            chkQuery.addBindValue(2);
            chkQuery.addBindValue(tr("接收长度出错"));
            chkQuery.exec();
            //emit updateErrorCode(2);
        }
    }else{
        qDebug()<<tr("用户数据库临时表格无数据！");
        chkQuery.clear();
        chkQuery.prepare(result_sql);
        chkQuery.addBindValue(dateTime.toString("yyyy-MM-dd hh:mm:ss"));
        chkQuery.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        chkQuery.addBindValue(5);
        chkQuery.addBindValue(tr("创建用户数据临时表格失败"));
        chkQuery.exec();
        //emit updateErrorCode(5);
        return;
    }
}

void MyThread::CloseDB()
{
    if(QSqlDatabase::contains(connectName))
    {
        {
            QSqlDatabase db = QSqlDatabase::database(connectName);
            if(db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(connectName);
    }
}
