#include "CachedSqlTableModel.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QRegExp>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QSqlDriver>
#include <QSqlField>
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
    QtConcurrent::run(this, &CachedSqlTableModel::insertPendingRows);
}

void CachedSqlTableModel::select()
{
    QSqlQuery selectQuery(m_db);
    auto sqlDriver = m_db.driver();
    QString insertQueryString = QString("SELECT * FROM %1 %2").arg(
                sqlDriver->escapeIdentifier(tableName, QSqlDriver::TableName),
                m_filter);
    insertQueryString.trimmed();

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
    beginResetModel();
    m_cache.clear();
    while (selectQuery.next())
    {
        m_cache.push_back(selectQuery.record());
    }
    endResetModel();
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

void CachedSqlTableModel::generateInsertValues(QString &insertQueryString)
{
    auto sqlDriver = m_db.driver();
    for (size_t i = static_cast<size_t>(commitBeginRow); i < m_cache.size(); ++i)
    {
        auto &record = m_cache[i];
        insertQueryString.append('(');
        for (int i = 0; i < columnCount() - 1; ++i)
        {
            insertQueryString.append(sqlDriver->formatValue(record.field(i)));
            insertQueryString.append(',');
        }
        insertQueryString.append(record.value(columnCount() - 1).toString());
        insertQueryString.append("),");
    }
    insertQueryString.truncate(insertQueryString.length() - 1);
}

QString CachedSqlTableModel::getFilter() const
{
    return m_filter;
}

void CachedSqlTableModel::setFilter(const QString &filter)
{
    m_filter = QString("WHERE %1").arg(filter);
}

bool CachedSqlTableModel::insertPendingRows()
{
    QSqlQuery insertQuery(m_db);
    auto sqlDriver = m_db.driver();
    QString insertQueryString = QString("INSERT INTO %1 VALUES").arg(
                sqlDriver->escapeIdentifier(tableName, QSqlDriver::TableName));

    generateInsertValues(insertQueryString);

    if (!insertQuery.prepare(insertQueryString))
    {
        m_lastError = insertQuery.lastError();
    }
    bool execResult = insertQuery.exec();

    if (!execResult)
    {
        m_lastError = insertQuery.lastError();
        emit insertionFailed();
    }
    else
    {
        emit rowsInserted(commitBeginRow, static_cast<int>(m_cache.size() - 1));
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
