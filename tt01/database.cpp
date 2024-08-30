#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
//#include <QElapsedTimer>

DBData::DBData(QList<DBNode> node_list) : Nodes {node_list}
{
    for (const auto & node : Nodes) {
        Tables[node.DB_TABLE].append(node.Id);
    }
    for (int d : Tables.value("DAYS")) {
        days[d] = Nodes.value(d).DATA.value("X").toInt();
    }
    for (int h : Tables.value("HOURS")) {
        hours[h] = Nodes.value(h).DATA.value("X").toInt();
    }
}

QString DBData::get_tag(int index)
{
    return Nodes.value(index).DATA.value("ID").toString();
}

void DBData::save(QString path)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid()) {
        db.close();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName(path);
    if (!db.open()) {
        qFatal() << "Opening database failed:" << path
                 << "\n Error:" << db.lastError();
    }
    db_path = path;
    save();
}

void DBData::save()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid()) {
        qFatal() << "Saving data failed – no database open";
    }
    db.transaction(); // using a transaction makes a dramatic speed difference
    QSqlQuery query;
    query.exec("drop table if exists NODES");
    query.exec("create table NODES "
               "(Id integer primary key, "
               "DB_TABLE text, "
               "DATA text)");
    query.prepare("insert into NODES (Id, DB_TABLE, DATA) "
                  "values (?, ?, ?)");
    for (const auto &node : Nodes) {
        query.bindValue(0, node.Id);
        query.bindValue(1, node.DB_TABLE);
        query.bindValue(2, QJsonDocument(node.DATA)
                               .toJson(QJsonDocument::JsonFormat::Compact));
        query.exec();
    }
    db.commit();
}
