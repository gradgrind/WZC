#ifndef READFET_H
#define READFET_H

#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>

QMap<QString, QList<QVariant>> readFet(QString fetxml);

void readFet_test();

#endif // READFET_H
