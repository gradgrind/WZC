#include "chip.h"

HoverRectItem::HoverRectItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent) {}

void HoverRectItem::setHoverHandler(void (* handler)(QGraphicsRectItem*, bool))
{
    hover_handler = handler;
    if (handler) {
        setAcceptHoverEvents(true);
    } else {
        setAcceptHoverEvents(false);
    }
}

void HoverRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    hover_handler(this, true);
}

void HoverRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    hover_handler(this, false);
}

// *******************

/** A rectangular box with border colour, border width and background colour.
 *  The default fill is none (transparent), the default pen is a black
 *  line with width = 1 (the width of a <QPen> can be set to an <int> or
 *  a <float>).
 *  The item's coordinate system starts at (0, 0), fixed by passing
 *  this origin to the <QGraphicsRectItem> constructor.
 *  The box is then moved to the desired location using method "place".
 *  It can have a vertically centred simple text item, which can be aligned
 *  horizontally left, centre or right, also a simple text item in each
 *  of the four corners:
 *      "tl" – top left     "tr" – top right
 *      "bl" – bottom left  "br" – bottom right
 *  The font and colour of the centred text can be set separately from
    those of the corners.
*/
Chip::Chip(qreal width, qreal height) : HoverRectItem() {
    setRect(0.0, 0.0, width, height);
}

// setPos(x, y); built in


