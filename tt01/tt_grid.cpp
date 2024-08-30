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

    scene->set_click_handler([this](const QList<QGraphicsItem *> items) {
        test(items);
    });
}

void TT_Grid::test_setup(void (*func)(DBData *, TT_Grid *))
{
    setup_func = func;
}

void TT_Grid::test(QList<QGraphicsItem *> items)
{
    scene->clear();
    setup_grid();
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

void TT_Grid::place_tile(Tile *tile, int col, int row)
{
    Chip *cell = cols[col + 1][row + 1];
    QRectF r = cell->rect();
    qreal cellw = r.width() - 2*GRIDLINEWIDTH;
    QPointF p = cell->pos();
    qreal x0 = p.x();
    qreal y0 = p.y();
    qreal h = r.height();
    if (tile->length > 1) {
        cell = cols[col + 1][row + tile->length];
        h = cell->pos().y() + cell->rect().height() - y0;
    }
    // Calculate width
    qreal w = cellw * tile->divs / tile->ndivs;
    qreal dx = cellw * tile->div0 / tile->ndivs;
    tile->place(
        x0 + GRIDLINEWIDTH + dx, y0 + GRIDLINEWIDTH,
        w, h - 2*GRIDLINEWIDTH);
}
