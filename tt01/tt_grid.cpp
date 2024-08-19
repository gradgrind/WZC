#include "tt_grid.h"

TT_Grid::TT_Grid(
    QGraphicsView *view,
    QStringList days,
    QStringList hours,
    QList<int> breaks)
{
    canvas = new Canvas(view);
    scene = canvas->scene;
    daylist = days;
    hourlist = hours;
    breaklist = breaks;
    setup_grid();

    //TODO: use lambda?
    scene->set_click_handler([this](const QList<QGraphicsItem *> items) {
        test(this, items);
    });
}

//[&] (const std::string& message)

void TT_Grid::test(TT_Grid *grid, QList<QGraphicsItem *> items)
{
    grid->scene->clear();
    grid->setup_grid();
}

void TT_Grid::setup_grid()
{
    cols.clear();

    QList<Chip *> hheaders;
    qreal y = 0.0;
    Chip *c = new Chip(VHEADERWIDTH, HHEADERHEIGHT);
    c->set_border(2.0, GRIDLINECOLOUR);
    hheaders.append(c);
    scene->addItem(c);
    c->setPos(-VHEADERWIDTH, -HHEADERHEIGHT);
    for(const QString &hour : std::as_const(hourlist)) {
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
    for(const QString &day : std::as_const(daylist)) {
        QList<Chip *> rows;
        y = 0.0;
        Chip *c = new Chip(DAY_WIDTH, HHEADERHEIGHT);
        c->set_border(2.0, GRIDLINECOLOUR);
        scene->addItem(c);
        c->set_text(day);
        rows.append(c);
        c->setPos(x, -HHEADERHEIGHT);
        for(const QString &hour : std::as_const(hourlist)) {
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
    // Add emphasis on breaks
    int x0 = -VHEADERWIDTH;
    int x1 = daylist.length() * DAY_WIDTH;
    if (re_colour.match(BREAKLINECOLOUR).hasMatch()) {
        QPen pen(QBrush(QColor("#FF" + BREAKLINECOLOUR)), GRIDLINEWIDTH);
        for(const int &b : std::as_const(breaklist)) {
            int y = b * HOUR_HEIGHT;
            QGraphicsLineItem *l = new QGraphicsLineItem(x0, y, x1, y);
            l->setPen(pen);
            scene->addItem(l);
        }
    }
}

TT_Grid::~TT_Grid()
{
    delete canvas;
    // TODO: more?
}
