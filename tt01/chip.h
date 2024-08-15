#ifndef CHIP_H
#define CHIP_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QRegularExpression>

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
    void set_text(QString middle = "", int m_align = 0,
        QString tl = "", QString tr = "",
        QString bl = "", QString br = "",
        qreal m_size = 0, bool m_bold = true, qreal c_size = 0
    );
    QMenu *context_menu = nullptr;

private:
    QGraphicsSimpleTextItem *set_item(
        QGraphicsSimpleTextItem *item,
        QString itext,
        int isize,
        bool ibold = false);
    QGraphicsSimpleTextItem *m_item = nullptr;
    QGraphicsSimpleTextItem *tl_item = nullptr;
    QGraphicsSimpleTextItem *tr_item = nullptr;
    QGraphicsSimpleTextItem *bl_item = nullptr;
    QGraphicsSimpleTextItem *br_item = nullptr;

    //QFont central;
    //QFont corner;
};

static QRegularExpression re_colour("^[0-9a-fA-F]{6}$");

#endif // CHIP_H
