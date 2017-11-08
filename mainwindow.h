#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CachedSqlTableModel.h"

namespace Ui {
class MainWindow;
}

class QSqlQueryModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QSqlTableModel* model;
    CachedSqlTableModel testModel;
};

#endif // MAINWINDOW_H
