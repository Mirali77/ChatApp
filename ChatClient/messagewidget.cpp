#include "messagewidget.h"
#include "ui_messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent, QString sender, QString message) :
    QWidget(parent),
    ui(new Ui::MessageWidget)
{
    ui->setupUi(this);
    ui->senderLabel->setText(sender + ":");
    ui->messageLabel->setText(message);
}

MessageWidget::~MessageWidget()
{
    delete ui;
}
