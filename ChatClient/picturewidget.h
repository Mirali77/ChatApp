#ifndef PICTUREWIDGET_H
#define PICTUREWIDGET_H

#include <QWidget>
#include <QImage>
#include <QDebug>

namespace Ui {
class PictureWidget;
}

class PictureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PictureWidget(QWidget *parent, QString sender, QImage pic);
    ~PictureWidget();

private:
    Ui::PictureWidget *ui;
};

#endif // PICTUREWIDGET_H
