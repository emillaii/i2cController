#ifndef CSERVER_H
#define CSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>
#include <i2ccontrol.h>
#include <QJsonDocument>
#include <QJsonObject>
class CServer: public QObject
{
    Q_OBJECT
public:
     CServer(): m_server(nullptr)
     {
     }

    ~CServer()
    {
        if (m_server) delete m_server;
    }
    int init(const QString & servername)
    {
        // 如果已经有一个实例在运行了就返回0
        if (isServerRun(servername)) {
            return 0;
        }
        m_server = new QLocalServer;
        // 先移除原来存在的,如果不移除那么如果
        // servername已经存在就会listen失败
        QLocalServer::removeServer(servername);
        // 进行监听
        m_server->listen(servername);
        connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
        return 1;
    }
    void setI2CControl(i2cControl *control) {
        this->m_control = control;
    }
private slots:
    // 有新的连接来了
    void newConnection()
    {
        qInfo("New Connection");
        if(m_socket!=nullptr)
        {
            m_socket->close();
            m_socket->flush();
        }
        m_socket = m_server->nextPendingConnection();
        connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    }
    // 可以读数据了
    void readyRead()
    {
        // 取得是哪个localsocket可以读数据了
        QLocalSocket *local = static_cast<QLocalSocket *>(sender());
        if (!local)
            return;
        QTextStream in(local);
        QString     readMsg;
        // 读出数据
        readMsg = in.readAll();
        qInfo(readMsg.toStdString().c_str());

        QJsonDocument jsonDocument = QJsonDocument::fromJson(readMsg.toUtf8().data());
        if( jsonDocument.isNull() ){
            qDebug()<< "===> please check the string "<< readMsg.toUtf8().data();
        }
        QJsonObject jsonObject = jsonDocument.object();

        QString cmd = jsonObject["cmd"].toString();
        int delay = jsonObject["delay"].toInt(0);
        int pos = jsonObject["pos"].toInt(0);
        qInfo("Input cmd: %s", cmd.toStdString().c_str());
        qInfo("Input delay: %d",delay);
        qInfo("Input pos: %d", pos);
        if(cmd == "init")
        {
            if(m_control->openDevice())
            {
                int ret = 0;
                ret = m_control->vcm_init();
                if(ret == 100)
                {
                    sendMessage("init success!");
                }
                else
                {
                    sendMessage(QString("init fail,error code %1!").arg(ret));
                    m_control->closeDevice();
                }
            }
            else
            {
                sendMessage("open i2c fail!");
            }
        }
        else if(cmd == "move")
        {
            int ret = 0;
            ret = m_control->vcm_move(pos);
            if(ret == 0)
            {
                sendMessage("move success!");
            }
            else
            {
                sendMessage(QString("move fail,error code %1!").arg(ret));
                m_control->closeDevice();
            }
        }
        else {
            sendMessage("cmd error!");
        }
        // 发送收到数据信号
//        emit newMessage(readMsg);
    }
private:
    QLocalServer *m_server;
    i2cControl *m_control;
    QLocalSocket *m_socket = nullptr;
    // 判断是否有一个同名的服务器在运行
    int isServerRun(const QString & servername)
    {
        // 用一个localsocket去连一下,如果能连上就说明
        // 有一个在运行了
        QLocalSocket ls;
        ls.connectToServer(servername);
        if (ls.waitForConnected(1000)){
            // 说明已经在运行了
            ls.disconnectFromServer();
            ls.close();
            return 1;
        }
        return 0;
    }

    void sendMessage(const QString &msg)
    {
        m_socket->write(msg.toStdString().c_str());
        m_socket->flush();
        m_socket->waitForBytesWritten();
        qDebug()<<"send message "<<msg;
    }
signals:
    void newMessage(const QString &msg);
};


#endif // CSERVER_H
