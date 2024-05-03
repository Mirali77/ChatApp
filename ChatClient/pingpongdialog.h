#ifndef PINGPONGDIALOG_H
#define PINGPONGDIALOG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class PingPongDialog;
}

class PingPongDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PingPongDialog(QWidget *parent = nullptr, quint16 seconds_left = 0);
    ~PingPongDialog();

private:
    Ui::PingPongDialog *ui;
    quint16 seconds;

signals:
    void confirmSignal();
    void noConfirmSignal();

private slots:
    void decrease();
    void on_confirmButton_clicked();
};

#endif // PINGPONGDIALOG_H
