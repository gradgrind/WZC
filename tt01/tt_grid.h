#ifndef TT_GRID_H
#define TT_GRID_H

#include <QApplication>
#include "canvas.h"
#include "chip.h"
#include <QStringList>

class TT_Grid; // forward declaration – needed in Tile

class Tile : public Chip
{
public:
    enum { Type = UserType + 3 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    Tile(TT_Grid *grid, QJsonObject data);

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

class TT_Grid
{
public:
    TT_Grid(
        QGraphicsView *view,
        QStringList days,
        QStringList hours,
        QList<int> breaks);
    ~TT_Grid();

    void place_tile(Tile *tile, int col, int row);

    void test(QList<QGraphicsItem *> items);

    Canvas *canvas;
    Scene *scene;
    QStringList daylist;
    QStringList hourlist;
    QList<int> breaklist;

    void setup_grid();
    QList<QList<Chip *>> cols;

    qreal DAY_WIDTH = 140.0;
    qreal HOUR_HEIGHT = 60.0;
    qreal VHEADERWIDTH = 80.0;
    qreal HHEADERHEIGHT = 40.0;
    qreal GRIDLINEWIDTH = 2.0;
    QString GRIDLINECOLOUR = "EDAB9A";
    QString BREAKLINECOLOUR = "404080";
    qreal FONT_CENTRE_SIZE = 12.0;
    qreal FONT_CORNER_SIZE = 8.0;

    QJsonObject settings;
};

#endif // TT_GRID_H

