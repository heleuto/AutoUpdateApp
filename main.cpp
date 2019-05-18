#include "widget.h"
#include <QApplication>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSystemSemaphore ss("MyObject",1,QSystemSemaphore::Open);
    ss.acquire();
    QSharedMemory mem("MySystemObject");
    if(!mem.create(1))
    {
        QMessageBox::information(0,QObject::tr("警告"),QObject::tr("程序已经运行"));
        ss.release();
        return 0;
    }
    ss.release();
    a.setQuitOnLastWindowClosed(false);//避免程序因Dialog关闭而退出
    Widget w;
    w.show();

    return a.exec();
}
