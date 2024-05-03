#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QChar>
#include <QDataStream>
#include <QTimer>
#include <QSet>
#include <QQueue>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server();
    QTcpSocket *socket;
    void messageAnnouncement(quint16 userId, QString message);
    void sendNameToUser(quint16 userId);
    void sendIdToUser(quint16 userId);

private:
    QSet<quint16> freeIds;
    QMap<quint16, QTcpSocket*> user_sockets;
    QMap<quint16, QString> user_names;
    QSet<quint16> user_confirmed;
    QByteArray data;
    quint16 nextBlockSize;
    const quint16 serverId = quint16(10001);
    QQueue<quint16> usersToDeleteQueue, pingPongAskQueue, pingPongCheckQueue;
    QVector<QByteArray> chatStory;

    void setNameFromUser(quint16 id, QDataStream& in);
    void setMessageFromUser(quint16 id, QDataStream& in);
    void disconnectUser(quint16 id);
    void deleteUser();
    void pingPongAsk();
    void pingPongCheckAns();
    void pingPongConfirmUser(quint16 id);

public slots:
    void incomingConnection(qintptr socketDescriptor);
    void slotReadyRead();
};

#endif // SERVER_H
