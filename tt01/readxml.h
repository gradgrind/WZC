#ifndef READXML_H
#define READXML_H

#include <QString>
#include <QVariant>
#include <QList>
#include <QXmlStreamReader>
#include <QMap>

struct XMLNode {
    QString name;
    QXmlStreamAttributes attributes;
    QList<QVariant> children;
};

Q_DECLARE_METATYPE(XMLNode)

XMLNode readXMLTree(QString xmlin);

QStringList printXMLNode(XMLNode node, QString pre = "");

void readxml_test();

#endif // READXML_H
