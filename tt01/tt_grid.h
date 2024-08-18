#ifndef TT_GRID_H
#define TT_GRID_H

#include <QApplication>
#include "canvas.h"
#include "chip.h"
#include <QStringList>

class TT_Grid
{
public:
    TT_Grid(QGraphicsView *view, QStringList days, QStringList hours);

    Canvas *canvas;
    Scene *scene;
    QList<QList<Chip *>> cols;


    qreal DAY_WIDTH = 140.0;
    qreal HOUR_HEIGHT = 60.0;
    qreal VHEADERWIDTH = 80.0;
    qreal HHEADERHEIGHT = 40.0;
    qreal GRIDLINEWIDTH = 1.0;
    QString GRIDLINECOLOUR = "b0b0b0";
    qreal FONT_CENTRE_SIZE = 12.0;
    qreal FONT_CORNER_SIZE = 8.0;
};

#endif // TT_GRID_H
