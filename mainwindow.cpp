#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parametricsvgitem/parametricsvgitem.h"

#include <QDoubleSpinBox>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(new QGraphicsScene());

    m_psvg = new ParametricSvgItem("sample.svg");

    if(m_psvg->isError()){
        foreach(auto message, m_psvg->errors()){
            QMessageBox msgBox;
            msgBox.setText(message);
            msgBox.setWindowTitle("JS");
            msgBox.exec();
        }
    }

    ui->graphicsView->scene()->addItem(m_psvg);

    ui->tableWidget->setHorizontalHeaderLabels({ "Name", "Value" });
    fillTable(m_psvg);

    QObject::connect(ui->tableWidget, &QTableWidget::itemChanged,
                                this, &MainWindow::changeValue);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeValue(QTableWidgetItem *item)
{
    int row = item->row();
    if(row < 0)
        return;
    //QVariant paramValue = item->data(Qt::UserRole);
    //paramValue.type()
    QTableWidgetItem *nameItem = ui->tableWidget->item(row, 0);
    QString paramName = nameItem->text();

    QString valueAsText = item->text();

     if(m_psvg->parameterType(paramName) == QVariant::Invalid)
         return;

    if(m_psvg->parameterType(paramName) == QVariant::Double){
        m_psvg->updateByParameter(paramName, valueAsText.toDouble());
    }else{
        m_psvg->updateByParameter(paramName, valueAsText);
    }


}

void MainWindow::fillTable(ParametricSvgItem *psvg)
{
    ui->tableWidget->setSortingEnabled(false);
    ui->tableWidget->setRowCount(psvg->parametersCount());
    int row = 0;
    foreach (auto name, psvg->parameterNames()) {
        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        auto flags = nameItem->flags();
        nameItem->setFlags(flags & (~Qt::ItemIsEditable));
        ui->tableWidget->setItem(row, 0, nameItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem(psvg->parameterValue(name).toString());
        valueItem->setData(Qt::UserRole, psvg->parameterValue(name));
        ui->tableWidget->setItem(row, 1, valueItem);

        ++row;
    }

    ui->tableWidget->setSortingEnabled(true);
}
