#include "CachedSqlTableModel.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QRegExp>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <memory>


QStringList fieldNamesFromRecord(const QSqlRecord &record)
{
    QStringList fieldNames;
    for (int i = 0; i < record.count(); ++i)
    {
        fieldNames.push_back(record.fieldName(i));
    }
    return fieldNames;
}

QVariantList fieldValuesFromRecord(const QSqlRecord &record)
{
    QVariantList fieldValues;

    for(int i = 0; i < record.count(); ++i)
    {
        fieldValues << record.value(i);
    }
    return fieldValues;
}

CachedSqlTableModel::CachedSqlTableModel(QObject *parent, QSqlDatabase db):
    QAbstractTableModel(parent),
    m_db(db)
{

}

void CachedSqlTableModel::insertRecord(const QSqlRecord &record)
{
    int lastIndex = static_cast<int>(m_cache.size());
    beginInsertRows(QModelIndex(), lastIndex, lastIndex + 1);
    m_cache.push_back(record);
    endInsertRows();
    if (commitBeginRow == -1)
    {
        commitBeginRow = static_cast<int>(m_cache.size() - 1);
    }

    if (editStrategy == QSqlTableModel::OnRowChange ||
            editStrategy == QSqlTableModel::OnFieldChange)
    {
        submitAll();
    }

}

void CachedSqlTableModel::submitAll()
{
    auto inserterFuture = QtConcurrent::run(this, &CachedSqlTableModel::insertPendingRows);
    std::shared_ptr<QFutureWatcher<bool>> inserterWatcher(new QFutureWatcher<bool>());
    connect(inserterWatcher.get(), &QFutureWatcher<bool>::finished, this, [this, inserterFuture](){
        if (inserterFuture.result())
        {
            emit rowsInserted(commitBeginRow, static_cast<int>(m_cache.size() - 1));
        }
        else
        {
            emit insertionFailed();
        }
    });
}

void CachedSqlTableModel::select()
{
    QSqlQuery selectQuery(m_db);
    QString insertQueryString = QString("SELECT * FROM %1").arg(tableName);
    if (!selectQuery.prepare(insertQueryString))
    {
        m_lastError = selectQuery.lastError();
    }
    selectQuery.setForwardOnly(true);
    selectQuery.exec();

    if (!selectQuery.isActive())
    {
        return;
    }

    m_templateRecord = selectQuery.record();
    m_columnCount = m_templateRecord.count();

    while (selectQuery.next())
    {
        m_cache.push_back(selectQuery.record());
    }

}

QVariant CachedSqlTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_cache.size()
            || role != Qt::DisplayRole)
    {
        return QVariant();
    }
    return m_cache[static_cast<size_t>(index.row())].value(index.column());
}

int CachedSqlTableModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(m_cache.size());
}

int CachedSqlTableModel::columnCount(const QModelIndex &) const
{
    return m_columnCount;
}

bool CachedSqlTableModel::insertPendingRows()
{
    QSqlQuery insertQuery(m_db);
    QString insertQueryString = QString("INSERT INTO '%1' VALUES").arg(tableName);

    for (size_t i = static_cast<size_t>(commitBeginRow); i < m_cache.size(); ++i)
    {
        auto &record = m_cache[i];
        insertQueryString.append("(");
//        for (int i = 0; i < columnCount(); ++i)
//        {
//            insertQueryString.append(record.value(i));
//        }
    }

    if (!insertQuery.prepare(insertQueryString))
    {
        m_lastError = insertQuery.lastError();
        qDebug() << m_lastError;
    }
    bool execResult = insertQuery.execBatch();
    if (!execResult)
    {
        m_lastError = insertQuery.lastError();
        qDebug() << m_lastError << insertQuery.executedQuery();
    }
    return execResult;
}

QSqlError CachedSqlTableModel::lastError() const
{
    return m_lastError;
}

QSqlRecord CachedSqlTableModel::record() const
{
    return m_templateRecord;
}

QSqlDatabase CachedSqlTableModel::database() const
{
    return m_db;
}

void CachedSqlTableModel::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
}

QString CachedSqlTableModel::getTableName() const
{
    return tableName;
}

void CachedSqlTableModel::setTableName(const QString &value)
{
    tableName = value;
}
