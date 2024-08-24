#include "readfet.h"
#include <QXmlStreamReader>
#include <qdebug.h>

struct child {
    QString key;
    QVariant val;
};

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

struct xmlnode {
    QString name;
    QXmlStreamAttributes attributes;
    QString text;
    QList<child> children;
};

//TODO: What about having ALL nodes as QMap<QString, QVariant> – also
// the text ones (and the empty ones)? That way I could include
// attributes, say as a mapping with key "__attributes__" and value
// QMap<QString, QString>. Text could be with key "__text__".

QMap<QString, QList<QVariant>> readFet(QString fetxml)
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
                .name = xml.name().toString(),
                .attributes = xml.attributes()
            };
            //qDebug() << "Start" << xml.name();
        } else if (xml.isEndElement()) {
            //qDebug() << "End" << xml.name();
            auto name = node.name;
            auto attr = node.attributes;
            QVariant v;
            if (!node.text.isEmpty()) {
                auto t = node.text;
                //qDebug() << " ++++ t:" << t;
                v.setValue(t);
            } else if (!node.children.isEmpty()) {
                QMap<QString, QList<QVariant>> cmap;
                for (const auto &c : node.children) {
                    auto vlp = cmap.value(c.key);
                    vlp.append(c.val);
                    cmap.insert(c.key, vlp);
                }
                v.setValue(cmap);
            }
            for (const auto& a : attr) {
                qDebug() << " ++" << name << ":" << a.name() << a.value();
            }
            //qDebug() << " ++" << name << ":" << attr;
            node = stack.takeLast();
            node.children.append({name, v});
        } else if (xml.isCharacters()) {
            if (xml.isWhitespace()) {
                continue;
            }
            if (!node.children.isEmpty()) {
                qFatal() << "XML-Element" << node.name
                         << "... new text, but has element(s) too"
                         << "@" << xml.lineNumber();
            }
            if (!node.text.isEmpty()) {
                qFatal() << "XML-Element" << node.name
                         << "... new text, but has text already"
                         << "@" << xml.lineNumber();
            }
            node.text = xml.text().toString();
            //qDebug() << "Text" << node.text;
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
    return node.children[0].val
        .value<QMap<QString, QList<QVariant>>>();
}


void readFet_test()
{
    auto fet = readFet(TESTXML);
    auto c1v = fet["Input"];
    for (const auto &cv : c1v) {
        auto cvm = cv.value<QMap<QString, QList<QVariant>>>();
        for (auto j = cvm.cbegin(), end = cvm.cend(); j != end; ++j) {
            qDebug() << "$$$" << j.key() << ":" << j.value();
        }
    }
}
