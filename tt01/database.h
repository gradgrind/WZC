#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QList>
#include <QJsonObject>

struct DBNode {
    int Id;
    QString DB_TABLE;
    QJsonObject DATA;
};

QString get_tag(QList<DBNode> nodes, int index);

void save_data(QString path, QList<DBNode> nodes);

#endif // DATABASE_H
