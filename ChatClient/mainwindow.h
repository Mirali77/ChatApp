#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "namedialog.h"
#include <QTimer>
#include <QMessageBox>
#include "pingpongdialog.h"
#include <QFileDialog>
#include <QBuffer>
#include "messagewidget.h"
#include "picturewidget.h"
#include <QScrollBar>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectionButton_clicked();

    void on_changeNameButton_clicked();

    void on_sendMessageButton_clicked();

    void on_disconnectButton_clicked();

    void on_messageEdit_returnPressed();

    void on_sendPictureButton_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket* socket;
    QByteArray data;
    quint32 nextBlockSize;
    NameDialog* nameDialog;
    quint16 id;
    bool connected;

    void setNameFromServer(QDataStream& in);
    void setMessageFromServer(QDataStream& in);
    void setIdFromServer(QDataStream& in);
    void sendMessageToServer(QString message);
    void pingPongAskFromServer();
    void sendDisconnectMessageToServer();
    void disconnectFromServer();
    void setPictureFromServer(QDataStream& in);

public slots:
    void slotReadyRead();
    void nameChangedSlot(QString name);
    void sendConfirmationToServer();
};
#endif // MAINWINDOW_H
