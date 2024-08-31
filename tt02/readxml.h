#ifndef READXML_H
#define READXML_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QMap>

struct XMLNode {
    QString name;
    QMap<QString, QString> attributes;
    QList<QVariant> children;
};

Q_DECLARE_METATYPE(XMLNode)

XMLNode readXMLTree(QString xmlin);

QStringList printXMLNode(XMLNode node, QString pre = "");

void readxml_test(bool show = true);

#endif // READXML_H
