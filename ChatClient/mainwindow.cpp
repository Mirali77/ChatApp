#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    nextBlockSize = 0;
    connected = false;
    id = -1;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_connectionButton_clicked()
{
    if (connected) {
        QMessageBox::critical(this, "Ошибка", "Вы уже подключены к серверу");
        return;
    }
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, [&] { connected = true;});
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    ui->chatBrowser->clear();
    socket->connectToHost("192.168.0.104", 2323);
    connected = true;
}

void MainWindow::slotReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_12);
    if (in.status() == QDataStream::Ok) {
        qDebug() << "Ready read";
        while (true) {
            if (!nextBlockSize) {
                if (socket->bytesAvailable() < 2) {
                    qDebug() << "Available bytes less than 2";
                    break;
                }
                in >> nextBlockSize;
                qDebug() << "Next blocks size = " << nextBlockSize;
            }
            if (socket->bytesAvailable() < nextBlockSize) {
                qDebug() << "Data is not fully loaded";
                break;
            }
            quint16 type;
            in >> type;
            // 0 - set name from server
            // 1 - set message announcement from server
            // 2 - set id from server
            // 3 - ping pong check from server
            qDebug() << type;
            switch (type) {
                case 0:
                    setNameFromServer(in);
                    break;
                case 1:
                    setMessageFromServer(in);
                    break;
                case 2:
                    setIdFromServer(in);
                    break;
                case 3:
                    pingPongAskFromServer();
                    break;
            }
            nextBlockSize = 0;
        }
    }
    else {
        qDebug() << "Reading error";
    }
}

void MainWindow::sendConfirmationToServer() {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint16(0) << id << quint16(3);
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    socket->write(data);
}

void MainWindow::pingPongAskFromServer() {
    PingPongDialog pingPongDialog(this, 18);
    connect(&pingPongDialog, &PingPongDialog::confirmSignal, this, &MainWindow::sendConfirmationToServer);
    connect(&pingPongDialog, &PingPongDialog::noConfirmSignal, this, [&]{
        sendDisconnectMessageToServer();
        disconnectFromServer();
    });
    pingPongDialog.setModal(true);
    pingPongDialog.exec();
}

void MainWindow::setNameFromServer(QDataStream &in) {
    QString name;
    in >> name;
    ui->nameLabel->setText(name);
}

void MainWindow::setMessageFromServer(QDataStream &in) {
    QString name, message;
    in >> name >> message;
    ui->chatBrowser->append(name + ":\n" + message + "\n");
}

void MainWindow::setIdFromServer(QDataStream& in) {
    quint16 my_id;
    in >> my_id;
    id = my_id;
}

void MainWindow::nameChangedSlot(QString name) {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint16(0) << id << quint16(0) << name;
    qDebug() << id;
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    socket->write(data);
}

void MainWindow::on_changeNameButton_clicked()
{
    if (!connected) {
        QMessageBox::critical(this, "Ошибка", "Вы не подключены к серверу");
        return;
    }
    nameDialog = new NameDialog(this);
    connect(nameDialog, &NameDialog::nameChanged, this, &MainWindow::nameChangedSlot);
    nameDialog->show();
}

void MainWindow::sendMessageToServer(QString message) {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint16(0) << id << quint16(1) << message;
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    socket->write(data);
}

void MainWindow::on_sendMessageButton_clicked()
{
    QString message = ui->messageEdit->text();
    if (message == "") {
        qDebug() << "Message is empty";
        return;
    }
    ui->messageEdit->setText("");
    sendMessageToServer(message);
}

void MainWindow::sendDisconnectMessageToServer() {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint16(0) << id << quint16(2);
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    socket->write(data);
}

void MainWindow::disconnectFromServer() {
    connected = false;
    ui->nameLabel->setText("");
    QTimer::singleShot(1000, socket, [&] { socket->disconnect(); });
}

void MainWindow::on_disconnectButton_clicked()
{
    sendDisconnectMessageToServer();
    disconnectFromServer();
}

void MainWindow::on_messageEdit_returnPressed()
{
    QString message = ui->messageEdit->text();
    if (message == "") {
        qDebug() << "Message is empty";
        return;
    }
    ui->messageEdit->setText("");
    sendMessageToServer(message);
}
