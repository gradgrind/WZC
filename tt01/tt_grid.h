#ifndef TT_GRID_H
#define TT_GRID_H

#include <QApplication>
#include "canvas.h"
#include "chip.h"
#include <QStringList>

class TT_Grid
{
public:
    TT_Grid(
        QGraphicsView *view,
        QStringList days,
        QStringList hours,
        QList<int> breaks);
    ~TT_Grid();

    static void test(TT_Grid * grid, QList<QGraphicsItem *> items);

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
};

#endif // TT_GRID_H
