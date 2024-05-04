#include "server.h"

Server::Server()
{
    if (this->listen(QHostAddress::Any, 2323)) qDebug() << "Succesfully connected";
    else qDebug() << "Connection failed";
    for (quint16 i = 1; i <= quint16(10000); i++) freeIds.push_back(i);
    nextBlockSize = 0;
}

void Server::incomingConnection(qintptr socketDescriptor) {
    socket = new QTcpSocket;
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    quint16 id = freeIds.front();
    freeIds.pop_front();
    user_sockets[id] = socket;
    user_names[id] = "Пользователь" + QString::number(id);
    qDebug() << "Client" << id << "connected";
    sendIdToUser(id);
    sendNameToUser(id);

    for(auto& byteData: chatStory) {
        QDataStream in(&byteData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_12);
        quint16 type;
        QString sender;
        in >> type >> sender;
        // 0 - message
        // 1 - picture
        if (type == 0) {
            QString message;
            in >> message;
            data.clear();
            QDataStream out(&data, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_12);
            out << quint32(0) << quint16(1) << sender << message;
            out.device()->seek(0);
            out << quint32(data.size() - sizeof(quint32));
            user_sockets[id]->write(data);
        }
        else {
            QImage pic;
            in >> pic;
            data.clear();
            QDataStream out(&data, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_12);
            out << quint32(0) << quint16(4) << sender << pic;
            out.device()->seek(0);
            out << quint32(data.size() - sizeof(quint32));
            user_sockets[id]->write(data);
        }
    }

    messageAnnouncement(serverId, user_names[id] + " подключился(лась)");
    pingPongAskQueue.push_back(id);
    QTimer::singleShot(60 * 1000, [&]{ pingPongAsk(); });
}

void Server::pingPongAsk() {
    if (pingPongAskQueue.empty()) {
        qDebug() << "Error: can't get id";
        return;
    }
    quint16 id = pingPongAskQueue.front();
    pingPongAskQueue.pop_front();
    if (!user_sockets.contains(id)) {
        return;
    }
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << quint16(3);
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    user_sockets[id]->write(data);

    pingPongCheckQueue.push_back(id);
    QTimer::singleShot(20 * 1000, [&]{ pingPongCheckAns();});
}

void Server::pingPongCheckAns() {
    if (pingPongCheckQueue.empty()) {
        qDebug() << "Error: can't get id";
        return;
    }
    quint16 id = pingPongCheckQueue.front();
    pingPongCheckQueue.pop_front();
    if (!user_sockets.contains(id)) {
        return;
    }
    if (user_confirmed.contains(id)) {
        user_confirmed.remove(id);
        pingPongAskQueue.push_back(id);
        QTimer::singleShot(60 * 1000, [&]{ pingPongAsk(); });
        return;
    }
    else if (user_sockets.contains(id)) disconnectUser(id);
}

void Server::pingPongConfirmUser(quint16 id) {
    user_confirmed.insert(id);
}

void Server::slotReadyRead() {
    socket = (QTcpSocket*)sender();
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_12);
    if (in.status() == QDataStream::Ok) {
        qDebug() << "Reading ";
        while (true) {
            if (!nextBlockSize) {
                if (socket->bytesAvailable() < 4) {
                    qDebug() << "Available bytes less than 4";
                    break;
                }
                in >> nextBlockSize;
                qDebug() << "Next blocks size = " << nextBlockSize;
            }
            if (socket->bytesAvailable() < nextBlockSize) {
                qDebug() << "Data is not fully loaded";
                break;
                //continue;
                //break;
            }
            quint16 id, type;
            in >> id >> type;
            // 0 - set user name
            // 1 - set message from user
            // 2 - disconnect user
            // 3 - ping pong confirmation from user
            // 4 - set picture from user
            switch (type) {
                case 0:
                    setNameFromUser(id, in);
                    break;
                case 1:
                    setMessageFromUser(id, in);
                    break;
                case 2:
                    disconnectUser(id);
                    break;
                case 3:
                    pingPongConfirmUser(id);
                    break;
                case 4:
                    setPictureFromUser(id, in);
                    break;
            }
            nextBlockSize = 0;
        }
    }
    else {
        qDebug() << "DataStream error";
    }
}

void Server::setPictureFromUser(quint16 id, QDataStream& in) {
    QImage pic;
    in >> pic;
    pictureAnnouncement(id, pic);
}

void Server::setNameFromUser(quint16 id, QDataStream &in) {
    QString name, old_name;
    old_name = user_names[id];
    in >> name;
    user_names[id] = name;
    sendNameToUser(id);
    messageAnnouncement(serverId, old_name + " поменял(а) своё имя на " + name);
}

void Server::setMessageFromUser(quint16 id, QDataStream& in) {
    QString message;
    in >> message;
    messageAnnouncement(id, message);
}

void Server::deleteUser() {
    if (usersToDeleteQueue.empty()) {
        qDebug() << "Error: no users to delete";
        return;
    }
    quint16 id = usersToDeleteQueue.front();
    qDebug() << id;
    usersToDeleteQueue.pop_front();
    freeIds.push_back(id);
    user_names.remove(id);
    user_sockets[id]->disconnect();
    user_sockets.remove(id);
}

void Server::disconnectUser(quint16 id) {
    messageAnnouncement(serverId, user_names[id] + " отключился(лась)\n");
    usersToDeleteQueue.push_back(id);
    QTimer::singleShot(1000, [&] { deleteUser(); });
}

void Server::messageAnnouncement(quint16 senderId, QString message) {
    data.clear();
    QDataStream pout(&data, QIODevice::WriteOnly);
    pout.setVersion(QDataStream::Qt_5_12);
    pout << quint16(0) << (senderId == serverId? "Сервер" : user_names[senderId]) << message;
    chatStory.push_back(data);

    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << quint16(1) << (senderId == serverId? "Сервер" : user_names[senderId]) << message;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    for (auto it = user_sockets.cbegin(); it != user_sockets.cend(); it++) {
        it.value()->write(data);
    }
}

void Server::pictureAnnouncement(quint16 senderId, QImage pic) {
    data.clear();
    QDataStream pout(&data, QIODevice::WriteOnly);
    pout.setVersion(QDataStream::Qt_5_12);
    pout << quint16(1) << (senderId == serverId? "Сервер" : user_names[senderId]) << pic;
    chatStory.push_back(data);

    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << quint16(4) << (senderId == serverId? "Сервер" : user_names[senderId]) << pic;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    for (auto it = user_sockets.cbegin(); it != user_sockets.cend(); it++) {
        it.value()->write(data);
    }
}

void Server::sendNameToUser(quint16 userId) {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << quint16(0) << user_names[userId];
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    user_sockets[userId]->write(data);
}

void Server::sendIdToUser(quint16 userId) {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << quint16(2) << userId;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    user_sockets[userId]->write(data);
}
