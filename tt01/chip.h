#ifndef CHIP_H
#define CHIP_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QRegularExpression>
#include <QJsonObject>

// *******************

class HoverRectItem : public QGraphicsRectItem
{
public:
    enum { Type = UserType + 1 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    HoverRectItem(QGraphicsItem *parent = nullptr);
    void setHoverHandler(void (* handler)(QGraphicsRectItem*, bool));

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void (* hover_handler)(QGraphicsRectItem*, bool);
};

// *******************

class Chip : public HoverRectItem
{
public:
    enum { Type = UserType + 2 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    Chip(qreal width, qreal height);

    void set_background(QString colour);
    void set_border(qreal width, QString colour = "");
    void set_text(QJsonObject jsonobj);
    QMenu *context_menu = nullptr;

private:
    void setitem(
        QGraphicsSimpleTextItem *&t_item,
        QJsonObject jsonobj,
        QString itext,
        QString isize,
        QString ibold = "");
    QGraphicsSimpleTextItem *m_item = nullptr;
    QGraphicsSimpleTextItem *tl_item = nullptr;
    QGraphicsSimpleTextItem *tr_item = nullptr;
    QGraphicsSimpleTextItem *bl_item = nullptr;
    QGraphicsSimpleTextItem *br_item = nullptr;
    void place_pair(
        QGraphicsSimpleTextItem *l,
        QGraphicsSimpleTextItem *r,
        bool top);

    //QFont central;
    //QFont corner;
};

static QRegularExpression re_colour("^[0-9a-fA-F]{6}$");

#endif // CHIP_H
