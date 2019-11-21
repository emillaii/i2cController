#include <QCoreApplication>
#include "i2ccontrol.h"
#include <QTextStream>
#include <QObject>
#include <cserver.h>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qInfo("Input argc: %d", argc);
    int ret = 0;
    int vcm_pos = 0;
    if (argc > 1) {
        vcm_pos = atoi(argv[1]);
        qInfo("VCM move to pos: %d", vcm_pos);
    }
    QTextStream cin(stdin,  QIODevice::ReadOnly);
    QTextStream cout(stdout,  QIODevice::WriteOnly);
    QTextStream cerr(stderr,  QIODevice::WriteOnly);
    i2cControl control;
//    control.vcm_move(vcm_pos);
//    qInfo("End of the vcm move");
//    CServer s;
//    s.setI2CControl(&control);
//    if (!s.init("localserver-test")){
//        // 初使化失败, 说明已经有一个在运行了
//        return 1;
//    }
//    return a.exec();
//------Debug use
    control.openDevice();
    control.vcm_init();
    qInfo("vcm_move Done");
    while(true) {
        QString cmd;
        qInfo("Waiting user command----");
        qInfo("q : quit the program");
        qInfo("m : move the vcm");

        cin>>cmd;
        qInfo("Input cmd: %s", cmd.toStdString().c_str());
        if (cmd == "q") { break; }
        else if (cmd == "m") {
            qInfo("Input the vcm position");
            cin>>cmd;
            control.vcm_move(cmd.toInt(0));
            qInfo("move complete 1");
        } else if (cmd == "s") {
            qInfo("Scan");
            control.readi2c();
        }
    }
//
    return ret;
}
