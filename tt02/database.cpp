#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
//#include <QElapsedTimer>

DBData::DBData(QMap<int, QJsonObject> node_map) : Nodes {node_map}
{
    for (auto i = Nodes.cbegin(), end = Nodes.cend(); i != end; ++i) {
        Tables[i.value().value("TYPE").toString()].append(i.key());
    }
    for (int d : Tables.value("DAYS")) {
        days[d] = Nodes.value(d).value("X").toInt();
    }
    for (int h : Tables.value("HOURS")) {
        hours[h] = Nodes.value(h).value("X").toInt();
    }
}

QString DBData::get_tag(int id)
{
    return Nodes.value(id).value("ID").toString();
}

void DBData::load(QString path)
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
    QSqlQuery query("select Id, DATA from NODES");
    Nodes.clear();
    while (query.next()) {
        int i = query.value(0).toInt();
        QJsonParseError err;
        auto json = QJsonDocument::fromJson(
            query.value(1).toByteArray(), &err);
        if (json.isNull()) {
            qFatal() << "Bad JSON:" << err.errorString() << "in node" << i;
        } else if (!json.isObject()) {
            qFatal() << "Invalid JSON in node" << i;
        }
        Nodes[i] = json.object();
    }
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
               "DATA text)");
    query.prepare("insert into NODES (Id, DATA) "
                  "values (?, ?)");
    for (auto i = Nodes.cbegin(), end = Nodes.cend(); i != end; ++i) {
        query.bindValue(0, i.key());
        query.bindValue(1, QJsonDocument(i.value())
            .toJson(QJsonDocument::JsonFormat::Compact));
        query.exec();
    }
    db.commit();
}

int time2mins(QString time)
{
    auto hm = time.split(":");
    bool ok;
    int h = hm[0].toInt(&ok);
    if (!ok || h < 0 || h >= 24) return -1;
    int m = hm[1].toInt(&ok);
    if (!ok || m < 0 || m >= 60) return -1;
    return h * 60 + m;
}
