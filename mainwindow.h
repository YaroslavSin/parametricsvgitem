#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ParametricSvgItem;
class QTableWidgetItem;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void changeValue(QTableWidgetItem *item);

private:
    Ui::MainWindow *ui;
    void fillTable(ParametricSvgItem *psvg);
    ParametricSvgItem *m_psvg;
};

#endif // MAINWINDOW_H
