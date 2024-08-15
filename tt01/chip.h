#ifndef CHIP_H
#define CHIP_H

#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

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

    QMenu *context_menu = nullptr;
};

#endif // CHIP_H
