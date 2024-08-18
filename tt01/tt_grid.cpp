#include "tt_grid.h"

TT_Grid::TT_Grid(QGraphicsView *view, QStringList days, QStringList hours) {
    canvas = new Canvas(view);
    scene = canvas->scene;

    QList<Chip *> hheaders;
    qreal y = 0.0;
    Chip *c = new Chip(VHEADERWIDTH, HHEADERHEIGHT);
    c->set_border(2.0, GRIDLINECOLOUR);
    hheaders.append(c);
    scene->addItem(c);
    c->setPos(-VHEADERWIDTH, -HHEADERHEIGHT);
    for(const QString &hour : std::as_const(hours)) {
        Chip *c = new Chip(VHEADERWIDTH, HOUR_HEIGHT);
        c->set_border(2.0, GRIDLINECOLOUR);
        c->set_text(hour);
        hheaders.append(c);
        scene->addItem(c);
        c->setPos(-VHEADERWIDTH, y);
        y += HOUR_HEIGHT;
    }
    cols.append(hheaders);
    qreal x = 0.0;
    for(const QString &day : std::as_const(days)) {
        QList<Chip *> rows;
        y = 0.0;
        Chip *c = new Chip(DAY_WIDTH, HHEADERHEIGHT);
        c->set_border(2.0, GRIDLINECOLOUR);
        scene->addItem(c);
        c->set_text(day);
        rows.append(c);
        c->setPos(x, -HHEADERHEIGHT);
        for(const QString &hour : std::as_const(hours)) {
            c = new Chip(DAY_WIDTH, HOUR_HEIGHT);
            c->set_border(2.0, GRIDLINECOLOUR);
            rows.append(c);
            scene->addItem(c);
            c->setPos(x, y);
            y += HOUR_HEIGHT;
        }
        cols.append(rows);
        x += DAY_WIDTH;
    }
}
