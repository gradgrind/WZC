#include "chip.h"
#include <QBrush>
#include <QPen>
#include <QFont>

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

/* Colour the background, which is initially transparent.
 * Colours must be provided as "RRGGBB" strings (case insensitive).
 * The chip becomes opaque.
*/
void Chip::set_background(QString colour)
{
    if (re_colour.match(colour).hasMatch()) {
        setBrush(QBrush(QColor("#FF" + colour)));
    } else {
        qFatal("Invalid background colour: %s", qUtf8Printable(colour));
    }
}

/* Set the border width and colour, which is initially black with width = 1.
 * Colours must be provided as "RRGGBB" strings (case insensitive).
*/
void Chip::set_border(qreal width, QString colour) {
    if (width > 0.01) {
        if (re_colour.match(colour).hasMatch()) {
            setPen(QPen(QBrush(QColor("#FF" + colour)), width));
        } else {
            qFatal("Invalid border colour: %s", qUtf8Printable(colour));
        }
    } else {
        setPen(Qt::PenStyle::NoPen);
    }
}

/* Set all the text items within the chip.
 * This also caters for rewriting the chip's text.
*/
void Chip::set_text(QString middle, int m_align,
                    QString tl, QString tr,
                    QString bl, QString br,
                    qreal m_size, bool m_bold, qreal c_size
                    ) {
    m_item = set_item(m_item, middle, m_size, m_bold);
    tl_item = set_item(tl_item, tl, c_size);
    tr_item = set_item(tr_item, tr, c_size);
    bl_item = set_item(bl_item, bl, c_size);
    br_item = set_item(br_item, br, c_size);

//TODO: manage sizes and place items
}

QGraphicsSimpleTextItem *Chip::set_item(
    QGraphicsSimpleTextItem *t_item,
    QString itext,
    int isize,
    bool ibold)
{
    if (itext.isEmpty()) {
        if (t_item) {
            scene()->removeItem(t_item);
            delete t_item;
        }
        return nullptr;
    } else {
        if (!t_item) {
            t_item = new QGraphicsSimpleTextItem(this);
        }
        t_item->setText(itext);
        QFont f;
        if (isize > 0.01) {
            f.setPointSizeF(isize);
        }
        f.setBold(ibold);
        t_item->setFont(f);
        return t_item;
    }
}
