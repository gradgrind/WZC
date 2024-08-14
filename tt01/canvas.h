#ifndef CANVAS_H
#define CANVAS_H

#include <QApplication>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QEvent>

// Sizes in points
const QSizeF A4(841.995, 595.35);
const QSizeF A3(1190.7, 841.995);

class Scene : public QGraphicsScene
{
    Q_OBJECT

public:
    Scene();
    QMenu *context_menu;
    void make_context_menu();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
};

// *******************

class HoverRectItem : public QGraphicsRectItem
{
public:
    HoverRectItem(void (* handler)(QGraphicsRectItem*, bool),
        QGraphicsItem *parent = nullptr);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void (* hover_handler)(QGraphicsRectItem*, bool);
};

// *******************

class Canvas : QObject
{
    Q_OBJECT

public:
    Canvas(QGraphicsView *gview);
    virtual ~Canvas () = default;

    QGraphicsView *view;
    int ldpi, pdpi;

    int pt2px(int pt);
    qreal px2mm(int px);

private:
    Scene *scene;

//private slots:
//    static void context_1();
};

// *******************

class Chip : public QGraphicsRectItem
{
public:
    enum { Type = UserType + 1 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

};

#endif // CANVAS_H
