#include "canvas.h"
#include <QGraphicsSceneMouseEvent>

const int minutesPerHour = 60;
const qreal CHIP_MARGIN = 1.5;
const qreal CHIP_SPACER = 10.0;

// Unit conversions
const qreal MM2PT = 2.83464549;
const qreal PT2MM = 0.3527778;

// *******************

HoverRectItem::HoverRectItem(void (* handler)(QGraphicsRectItem*, bool),
        QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    hover_handler = handler;
    if (handler) {
        setAcceptHoverEvents(true);
    }
}

void HoverRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    hover_handler(this, true);
}

void HoverRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    hover_handler(this, false);
}

// *******************

// Canvas: This is the "view" widget for the canvas – though it is not itself
// a graphical element! The QGraphicsView handled here is passed in as a
// parameter.
// The actual canvas is implemented as a "scene".
Canvas::Canvas(QGraphicsView *gview) {
    view = gview;
// Change update mode: The default, MinimalViewportUpdate, seems
// to cause artefacts to be left, i.e. it updates too little.
// Also BoundingRectViewportUpdate seems not to be 100% effective.
// view->setViewportUpdateMode(
//     QGraphicsView::ViewportUpdateMode::BoundingRectViewportUpdate
// )
    view->setViewportUpdateMode(
        QGraphicsView::ViewportUpdateMode::FullViewportUpdate
    );
// view->setRenderHints(
//     QPainter::RenderHint::Antialiasing
//     | QPainter::RenderHint::SmoothPixmapTransform
// )
// view->setRenderHints(QPainter::RenderHint::TextAntialiasing)
    view->setRenderHints(QPainter::RenderHint::Antialiasing);

    ldpi = view->logicalDpiX();
    pdpi = view->physicalDpiX();
    // Scaling the scene by pdpi/ldpi should display the correct size ...

    Scene *scene = new Scene();
    view->setScene(scene);
    //self.items = {}

    //make_context_menu()

    //-- Testing code:
    QGraphicsRectItem *r1 = new QGraphicsRectItem(20, 50, 300, 10);
    view->scene()->addItem(r1);
    view->scene()->addRect(QRectF(200, 300, 100, 100), QPen(Qt::black), QBrush(Qt::red));
}

int Canvas::pt2px(int pt) {
    return int(ldpi * pt / 72.0 + 0.5);
}

qreal Canvas::px2mm(int px) {
    return px * 25.4 / ldpi;
}

// *******************

Scene::Scene() : QGraphicsScene() {}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::MouseButton::LeftButton) {
        int kbdmods = qApp->keyboardModifiers();
        int keymod = 0;
        // Note that Ctrl-click is for context menu on OSX ...
        // Note that Alt-click is intercepted by some window managers on
        // Linux ... In that case Ctrl-Alt-click might work.
        if (kbdmods & Qt::KeyboardModifier::AltModifier) {
            keymod = 4;
        } else {
            if (kbdmods & Qt::KeyboardModifier::ShiftModifier) {
                keymod = 1;
            }
            if (kbdmods & Qt::KeyboardModifier::ControlModifier) {
                keymod += 2;
            }
        }
        QPointF point = event->scenePos();
    //QList<QGraphicsItem *> allitems = items(point);
        qDebug() << "Items" << keymod
                 << " @ " << point << " : " << items(point);

/*
                cell = None
                tiles = []
                item0 = None
                for item in items:
                    try:
                        cell = item.tag
                        item0 = item
                    except AttributeError:
                        tiles.append(item)
                for tile in tiles:
                    # Give all tiles at this point a chance to react, starting
                    # with the topmost. An item can break the chain by
                    # returning a false value.
                    try:
                        if not tile.leftclick():
                            return
                    except AttributeError:
                        pass
                if item0:
                    print(f"Left press{shift}{ctrl}{alt} @ {cell}")
                    if shift:
#???
                        self.place_tile("T2", cell)
                    if alt:
                        self.select_cell(cell)

*/

    }
}

void Scene::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    qDebug() << "Context Menu";
}

