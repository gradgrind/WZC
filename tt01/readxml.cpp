#include "readxml.h"
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


XMLNode readXMLTree(QString xmlin)
{
    QList<XMLNode> stack;
    XMLNode node;
    QXmlStreamReader xml(xmlin);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            // Push current level
            stack.append(node);
            node = XMLNode{
                .name = xml.name().toString(),
                .attributes = xml.attributes()
            };
        } else if (xml.isEndElement()) {
            //qDebug() << "End" << xml.name();
            auto node0 = node;
            node = stack.takeLast();
            QVariant v;
            v.setValue(node0);
            node.children.append(v);
        } else if (xml.isCharacters()) {
            if (xml.isWhitespace()) {
                continue;
            }
            node.children.append(xml.text().toString());
        } else if (
            xml.isStartDocument()
            or xml.isEndDocument()
            or xml.isComment()) {
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
    return node.children[0].value<XMLNode>();
}

QStringList printXMLNode(XMLNode node, QString pre)
{
    QStringList slist;
    slist.append(pre + node.name);
    pre += "  ";
    for (const QXmlStreamAttribute &a : node.attributes) {
        slist.append(pre + "@ "
                       + a.qualifiedName().toString()
                       + ": " + a.value().toString());
    }
    for (const QVariant &v : node.children) {
        if (v.canConvert<QString>()) {
            slist.append(pre + v.toString());
        } else {
            slist += printXMLNode(v.value<XMLNode>(), pre);
        }
    }
    return slist;
}

void readxml_test()
{
    auto xml = readXMLTree(TESTXML);
    for (const auto &l : printXMLNode(xml)) {
        qDebug() << l;
    }
}
