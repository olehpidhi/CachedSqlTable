#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QDebug>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QSqlField>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/home/king/Projects/DBPlayground/history.db");
    if (!db.open())
    {
        qDebug() << "failed opening db";
    } else {
        testModel.setTableName("historyTable");
        testModel.setDatabase(db);
        testModel.select();
        qDebug() << testModel.rowCount();
        auto r = testModel.record();
        r.setValue("entityId", "123456.0.0");
        r.setValue("dataType", 13);
        r.setValue("dataSource", 1);
        r.setValue("time", QVariant::fromValue<long>(1509450467357));
        r.setValue("value", 12435);
        r.setValue("sent", 0);
        connect(&testModel, &CachedSqlTableModel::rowsInserted, this, [](int first, int last){
            qDebug() << "inserted" << first << last;

        });
        testModel.insertRecord(r);
        qDebug() << testModel.rowCount();
        ui->tableView->setModel(&testModel);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
