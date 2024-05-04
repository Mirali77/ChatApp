#include "picturewidget.h"
#include "ui_picturewidget.h"

PictureWidget::PictureWidget(QWidget *parent, QString sender, QImage pic) :
    QWidget(parent),
    ui(new Ui::PictureWidget)
{
    ui->setupUi(this);
    ui->senderLabel->setText(sender + ":");
    QImage scaledPic = pic.scaled(qMin(440, pic.width()), qMin(250, pic.height()), Qt::IgnoreAspectRatio);
    ui->pictureLabel->setPixmap(QPixmap::fromImage(scaledPic, Qt::AutoColor));
}

PictureWidget::~PictureWidget()
{
    delete ui;
}
