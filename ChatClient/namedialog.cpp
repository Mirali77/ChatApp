#include "namedialog.h"
#include "ui_namedialog.h"

NameDialog::NameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NameDialog)
{
    ui->setupUi(this);
}

NameDialog::~NameDialog()
{
    delete ui;
}

void NameDialog::on_buttonBox_rejected()
{
    this->close();
}


void NameDialog::on_buttonBox_accepted()
{
    QString name = ui->nameEdit->text();
    emit nameChanged(name);
}
