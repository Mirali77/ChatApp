#include "pingpongdialog.h"
#include "ui_pingpongdialog.h"

PingPongDialog::PingPongDialog(QWidget *parent, quint16 seconds_left) :
    QDialog(parent),
    ui(new Ui::PingPongDialog)
{
    ui->setupUi(this);
    seconds = seconds_left;
    ui->timerDisplay->display(seconds);
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(decrease()));
    timer->start(1000);
}

PingPongDialog::~PingPongDialog()
{
    delete ui;
}

void PingPongDialog::decrease() {
    seconds--;
    if (seconds == 0) {
        emit noConfirmSignal();
        this->close();
    }
    else ui->timerDisplay->display(seconds);
}

void PingPongDialog::on_confirmButton_clicked()
{
    emit confirmSignal();
    this->close();
}
