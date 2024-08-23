#include "readfet.h"
#include <QList>
#include <QXmlStreamReader>
#include <qdebug.h>

const QString TESTXML(
R"(<?xml version="1.0" encoding="UTF-8"?>
<Draw>
    <Input>
        <Column title="A"/>
        <Column title="B"/>
        <Column title="C"/>
        <Column title="D">
            <item id="0">Bayer Leverkusen &amp;</item>
            <item id="1">Benfica</item>
            <item id="2">Villareal</item>
            <item id="3"/>
        </Column>
    </Input>
</Draw>
)");

/* Use a QVariantMap to store the nodes, saving only strings,
 * arrays and objects.
 */

struct xmlnode {
    QStringView name;
    QXmlStreamAttributes attributes;
    QString text;
    QList<xmlnode> children;
};

ReadFet::ReadFet(QString fetxml)
{
    // This is not a general, comprehensive XML reader ...
    QList<xmlnode> stack;
    xmlnode node;
    QXmlStreamReader xml(fetxml);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (!node.text.isEmpty()) {
                qFatal() << "XML-Element" << node.name
                         << "... new element, but text not empty"
                         << "@" << xml.lineNumber();
            }
            // Push current level
            stack.append(node);
            node = xmlnode{
                .name = xml.name(),
                .attributes = xml.attributes()
            };
            qDebug() << "Start" << xml.name();
        } else if (xml.isEndElement()) {
            qDebug() << "End" << xml.name();
            xmlnode node0 = node;
            node = stack.takeLast();
            node.children.append(node0);
        } else if (xml.isCharacters()) {
            if (xml.isWhitespace()) {
                continue;
            }
            if (node.children.length() != 0) {
                qFatal() << "XML-Element" << node.name
                         << "... new text, but has element(s) too"
                         << "@" << xml.lineNumber();
            }
            qDebug() << "Text" << xml.text();
            node.text += xml.text().toString();
        } else if (xml.isStartDocument() or xml.isEndDocument()) {
            continue;
        } else {
            qFatal() << "Unexpected XML:" << xml.tokenString()
                     << "@" << xml.lineNumber();
        }
    }
    if (xml.hasError()) {
        qFatal() << "XML error: " << xml.errorString()
                 << "@" << xml.lineNumber();
    }
}


void ReadFet::test()
{
    ReadFet r(TESTXML);
}
