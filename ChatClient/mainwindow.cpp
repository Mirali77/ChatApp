#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->chatBrowser->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    ui->chatBrowser->verticalScrollBar()->setSingleStep(10);
    ui->chatBrowser->verticalScrollBar()->setPageStep(20);
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
                if (socket->bytesAvailable() < 4) {
                    qDebug() << "Available bytes less than 4";
                    break;
                }
                in >> nextBlockSize;
                qDebug() << "Next blocks size = " << nextBlockSize;
            }
            if (socket->bytesAvailable() < nextBlockSize) {
                qDebug() << "Data is not fully loaded";
                //continue;
                break;
            }
            quint16 type;
            in >> type;
            // 0 - set name from server
            // 1 - set message announcement from server
            // 2 - set id from server
            // 3 - ping pong check from server
            // 4 - set picture announcement from server
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
                case 4:
                    setPictureFromServer(in);
                    break;
            }
            nextBlockSize = 0;
        }
    }
    else {
        qDebug() << "Reading error";
    }
}

void MainWindow::setPictureFromServer(QDataStream& in) {
    QImage pic; QString sender;
    in >> sender >> pic;
    PictureWidget* pictureItem = new PictureWidget(nullptr, sender, pic);
    QListWidgetItem* item = new QListWidgetItem;
    ui->chatBrowser->addItem(item);
    item->setSizeHint(QSize(0, 300));
    ui->chatBrowser->setItemWidget(item, pictureItem);
    ui->chatBrowser->scrollToBottom();
}

void MainWindow::sendConfirmationToServer() {
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << id << quint16(3);
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
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
    MessageWidget* messageItem = new MessageWidget(nullptr, name, message);
    QListWidgetItem* item = new QListWidgetItem;
    ui->chatBrowser->addItem(item);
    item->setSizeHint(QSize(0, 80));
    ui->chatBrowser->setItemWidget(item, messageItem);
    ui->chatBrowser->scrollToBottom();
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
    out << quint32(0) << id << quint16(0) << name;
    qDebug() << id;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
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
    out << quint32(0) << id << quint16(1) << message;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
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
    out << quint32(0) << id << quint16(2);
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
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

void MainWindow::on_sendPictureButton_clicked()
{
    QString path;
    path = QFileDialog::getOpenFileName(this, "Выбрать файл", "/home",
                       "All files (*.*);; JPEG Image (*.jpg) ;; PNG Image (*.png);");
    QImage pic(path);
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << quint32(0) << id << quint16(4) << pic;
    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));
    socket->write(data);
}
