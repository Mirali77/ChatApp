#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>

namespace Ui {
class MessageWidget;
}

class MessageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MessageWidget(QWidget *parent, QString sender, QString message);
    ~MessageWidget();

private:
    Ui::MessageWidget *ui;
};

#endif // MESSAGEWIDGET_H
