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
 *  The box is then moved to the desired location using method "setPos".
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

const qreal CHIP_MARGIN = 1.5;

/* Set all the text items within the chip.
 * This also caters for rewriting the chip's text.
*/
//TODO: Unfortunately C++ doesn't support passing parameters by name,
// so I use JSON ... which will be needed later anyway.
void Chip::set_text(QJsonObject jsonobj)
{
    setitem(m_item, jsonobj, "middle", "middle_size", "middle_bold");
    setitem(tl_item, jsonobj, "tl", "corner_size");
    setitem(tr_item, jsonobj, "tr", "corner_size");
    setitem(bl_item, jsonobj, "bl", "corner_size");
    setitem(br_item, jsonobj, "br", "corner_size");

//TODO: manage sizes and place items
    // Get chip dimensions
    QRectF bbr = rect();
    qreal hr = bbr.height();
    qreal wr = bbr.width();
    // Central item ("middle")
    if (m_item) {
        // Reset scale
        m_item->setScale(1);
        QRectF bb = m_item->boundingRect();
        qreal h = bb.height();
        qreal w = bb.width();
        qreal scale = (wr - CHIP_MARGIN * 2) * 0.9 / w;
        if (scale < 1.0) {
            m_item->setScale(scale);
            w *= scale;
            h *= scale;
        }
        // Deal with alignment
        QJsonValue item_j = jsonobj.value("middle_align");
        QString text_a;
        if (item_j != QJsonValue::Undefined) {
            text_a = item_j.toString();
        }
        qreal x;
        if (text_a == "l") {
            x = CHIP_MARGIN;
        } else if (text_a == "r") {
            x = wr - CHIP_MARGIN - w;
        } else {
            x = (wr - w) / 2;
        }
        m_item->setPos(x, (hr - h) / 2);
    }
    // Top items
    place_pair(tl_item, tr_item, true);
    // Bottom items
    place_pair(bl_item, br_item, false);
}

void Chip::setitem(
    QGraphicsSimpleTextItem *&t_item,
    QJsonObject jsonobj,
    QString itext,
    QString isize,
    QString ibold)
{
    QJsonValue item_j = jsonobj.value(itext);
    QString text_i;
    if (item_j != QJsonValue::Undefined) {
        text_i = item_j.toString();
    }
    if (text_i.isEmpty()) {
        if (t_item) {
            scene()->removeItem(t_item);
            delete t_item;
            t_item = nullptr;
        }
        return;
    }
    if (!t_item) {
        t_item = new QGraphicsSimpleTextItem(this);
    }
    t_item->setText(text_i);
    item_j = jsonobj.value(isize);
    QFont f;
    qreal size_i = 0.0;
    if (item_j != QJsonValue::Undefined) {
        size_i = item_j.toDouble();
    }
    if (size_i > 0.01) {
        f.setPointSizeF(size_i);
    }
    bool bold_i = false;
    if (!ibold.isEmpty()) {
        item_j = jsonobj.value(ibold);
        if (item_j != QJsonValue::Undefined) {
            bold_i = item_j.toBool();
        }
    }
    f.setBold(bold_i);
    t_item->setFont(f);
}

void Chip::place_pair(
    QGraphicsSimpleTextItem *l,
    QGraphicsSimpleTextItem *r,
    bool top)
{
    qreal w0 = rect().width() - CHIP_MARGIN * 2;
    qreal w_l = 0.0;
    if (l) {
        // Reset scale
        w_l= l->boundingRect().width();
    }
    qreal w_r = 0.0;
    if (r) {
        // Reset scale
        w_r= r->boundingRect().width();
    }
    // Get scales
    qreal s_l = 1.0;
    qreal s_r = 1.0;
    if ((w_l + w_r) / w0 > 0.8) {
        // Need some shrinking
        if (w_l / w0 < 0.25) {
            // Only shrink r
            s_r = (w0 - w_l) * 0.8 / w_r;
        } else if (w_r / w0 < 0.25) {
            // Only shrink l
            s_l = (w0 - w_r) * 0.8 / w_l;
        } else {
            // Shrink both
            s_l = w0 * 0.8 / (w_l + w_r);
            s_r = s_l;
        }
    }
    // Place items
    if (l) {
        l->setScale(s_l);
        if (top) {
            l->setPos(CHIP_MARGIN, CHIP_MARGIN);
        } else {
            qreal h0 = rect().height() - CHIP_MARGIN;
            l->setPos(CHIP_MARGIN, h0 - l->boundingRect().height());
        }
    }
    if (r) {
        r->setScale(s_r);
        w_r *= s_r;
        if (top) {
            r->setPos(w0 + CHIP_MARGIN - w_r, CHIP_MARGIN);
        } else {
            qreal h0 = rect().height() - CHIP_MARGIN;
            r->setPos(w0 + CHIP_MARGIN - w_r, h0 - r->boundingRect().height());
        }
    }
}
