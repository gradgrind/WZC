#include "canvas.h"
#include "chip.h"
#include <QMenu>

const int minutesPerHour = 60;
const qreal CHIP_MARGIN = 1.5;
const qreal CHIP_SPACER = 10.0;

// Unit conversions
const qreal MM2PT = 2.83464549;
const qreal PT2MM = 0.3527778;

// *******************

void on_hover(QGraphicsRectItem* item, bool enter)
{
    qDebug() << (enter ? "ENTER" : "EXIT");
}


// Canvas: This is the "view" widget for the canvas – though it is not itself
// a graphical element! The QGraphicsView handled here is passed in as a
// parameter.
// The actual canvas is implemented as a "scene".
Canvas::Canvas(QGraphicsView *gview) : QObject()
{
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

    scene = new Scene();
    view->setScene(scene);
    //self.items = {}

    /*
    //-- Testing code:
    QGraphicsRectItem *r1 = new QGraphicsRectItem(20, 50, 300, 10);
    scene->addItem(r1);
    scene->addRect(QRectF(200, 300, 100, 100), QPen(Qt::black), QBrush(Qt::red));
    Chip *chip1 = new Chip(200, 40);
    scene->addItem(chip1);
    chip1->setPos(-50, 300);
    chip1->setHoverHandler(on_hover);
    chip1->set_background("f0f000");
    chip1->set_border(3, "ff0000");
    chip1->config_text(14, true, 0, "00E0FF");
    chip1->set_subtext_size(10);
    chip1->set_text("Central");
    chip1->set_toptext("TL", "An extremely long entry which will need shrinking");
    chip1->set_bottomtext("BL", "BR");
    Chip *chip2 = new Chip(40,100);
    scene->addItem(chip2);
    chip2->setPos(-20, 250);
    chip2->set_background("e0e0ff");
    chip2->set_border(0);
    chip2->set_text("Middle");
    */
}

int Canvas::pt2px(int pt) {
    return int(ldpi * pt / 72.0 + 0.5);
}

qreal Canvas::px2mm(int px) {
    return px * 25.4 / ldpi;
}

/*
 * void Canvas::context_1() {
    qDebug() << "-> context_1";
}
*/

// *******************

Scene::Scene() : QGraphicsScene() {
    make_context_menu();
}

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

        QList<QGraphicsItem *> allitems = items(point);

        qDebug() << "Items" << keymod
                 << " @ " << point << " : " << allitems;

        if (click_handler) {
            click_handler(allitems);
        }


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

void Scene::make_context_menu() {
    context_menu = new QMenu();
    QAction *action = context_menu->addAction("I am context Action 1");
    //connect(action, &QAction::triggered, &Canvas::context_1);
    connect(action, &QAction::triggered, [=](bool b) {qDebug() << "-> lambda";});
}

void Scene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    auto point = event->scenePos();
    auto gitems = items(point);
    // Choose the topmost reacting item
    QMenu *cm = context_menu;
    for(auto gitem : gitems) {
        Chip *gtitem = qgraphicsitem_cast<Chip *>(gitem);
        if (gtitem) {
            qDebug() << "Chip";
            //return;
            // Get the context menu from the item.
            // What info do the handlers need? Item (the scene is available
            // from the item, if it is passed as pointer)?
            cm = gtitem->context_menu;
        }
    }
    if (cm) {
        cm->exec(event->screenPos());
    } else {
        qDebug() << "Context Menu";
    }
}

//TODO: At present I am just using this for testing
void Scene::set_click_handler(std::function<void (const QList<QGraphicsItem *>)> handler)
{
    click_handler = handler;
}
