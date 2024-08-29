#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonDocument>
//#include <QElapsedTimer>

QString get_tag(QList<DBNode> nodes, int index)
{
    return nodes.value(index).DATA.value("ID").toString();
}

void save_data(QString path, QList<DBNode> nodes)
{
    //QElapsedTimer timer;
    //timer.start();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();
    db.transaction(); // using a transaction makes a dramatic speed difference
    QSqlQuery query;
    query.exec("drop table if exists NODES");
    query.exec("create table NODES "
               "(Id integer primary key, "
               "DB_TABLE text, "
               "DATA text)");
    query.prepare("INSERT INTO NODES (Id, DB_TABLE, DATA) "
                  "VALUES (?, ?, ?)");
    for (const auto &node : nodes) {
        query.bindValue(0, node.Id);
        query.bindValue(1, node.DB_TABLE);
        query.bindValue(2, QJsonDocument(node.DATA)
            .toJson(QJsonDocument::JsonFormat::Compact));
        query.exec();
    }
    db.commit();
    //qDebug() << "DB saving took" << timer.elapsed() << "milliseconds";
}

