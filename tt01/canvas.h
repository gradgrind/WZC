#ifndef CANVAS_H
#define CANVAS_H

#include <QApplication>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
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

    Scene *scene;
};

#endif // CANVAS_H
