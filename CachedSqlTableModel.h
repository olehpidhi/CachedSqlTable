#ifndef CACHEDSQLTABLEMODEL_H
#define CACHEDSQLTABLEMODEL_H

#include <QAbstractTableModel>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QFuture>

class CachedSqlTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CachedSqlTableModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());

    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void insertRecord(const QSqlRecord& record);
    void setRecord(int row, const QSqlRecord & record);

    QString getTableName() const;
    void setTableName(const QString &value);

    QSqlDatabase database() const;
    void setDatabase(const QSqlDatabase &db);

    QSqlError lastError() const;

    QSqlRecord record() const;

    QString getFilter() const;
    void setFilter(const QString &filter);

public slots:
    void submitAll();
    void select();

signals:
    void rowsInserted(int first, int last);
    void insertionFailed();

private:
    bool insertPendingRows();
    void generateInsertValues(QString &insertQueryString);

private:
    std::vector<QSqlRecord> m_cache;
    QSqlDatabase m_db;
    QString tableName;
    QString m_filter;
    int commitBeginRow = -1;
    QSqlTableModel::EditStrategy editStrategy = QSqlTableModel::OnRowChange;
    int m_columnCount = 0;
    QSqlError m_lastError;
    QSqlRecord m_templateRecord;
};

#endif // CACHEDSQLTABLEMODEL_H
