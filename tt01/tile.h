#ifndef TILE_H
#define TILE_H

#include "chip.h"

class Tile : public Chip
{
public:
    enum { Type = UserType + 4 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    Tile(QGraphicsScene *scene, QJsonObject settings, QJsonObject data);

    void place(qreal x, qreal y, qreal w, qreal h);

    QString tag;
    int length;
    int divs;
    int div0;
    int ndivs;
    QString middle;
    QString tl;
    QString tr;
    QString bl;
    QString br;

    const qreal TILE_BORDER_WIDTH = 1.0;
    const bool TEXT_BOLD = true;
    const int TEXT_ALIGN = 0; // centred
};

#endif // TILE_H
