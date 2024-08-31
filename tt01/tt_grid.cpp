#include "tt_grid.h"

Cell::Cell(int x, int y) : Chip(), cellx{x}, celly{y} {}

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

    //TODO: do something useful ...
    scene->set_click_handler([this](const QList<QGraphicsItem *> items) {
        test(items);
    });
}

//TODO: remove
void TT_Grid::test(QList<QGraphicsItem *> items)
{
    qDebug() << "CLICKED:" << items;
}

void TT_Grid::setup_grid()
{
    cols.clear();

    QList<Cell *> hheaders;
    qreal y = 0.0;
    Cell *c = new Cell(-1, -1);
    c->set_size(VHEADERWIDTH, HHEADERHEIGHT);
    c->set_border(2.0, GRIDLINECOLOUR);
    hheaders.append(c);
    scene->addItem(c);
    c->setPos(-VHEADERWIDTH, -HHEADERHEIGHT);
    int yi = 0;
    for(const QString &hour : std::as_const(hourlist)) {
        Cell *c = new Cell(-1, yi);
        c->set_size(VHEADERWIDTH, HOUR_HEIGHT);
        c->set_border(2.0, GRIDLINECOLOUR);
        c->set_text(hour);
        hheaders.append(c);
        scene->addItem(c);
        c->setPos(-VHEADERWIDTH, y);
        y += HOUR_HEIGHT;
        yi++;
    }
    cols.append(hheaders);
    qreal x = 0.0;
    int xi = 0;
    for(const QString &day : std::as_const(daylist)) {
        QList<Cell *> rows;
        y = 0.0;
        Cell *c = new Cell(xi, -1);
        c->set_size(DAY_WIDTH, HHEADERHEIGHT);
        c->set_border(2.0, GRIDLINECOLOUR);
        scene->addItem(c);
        c->set_text(day);
        rows.append(c);
        c->setPos(x, -HHEADERHEIGHT);
        int yi = 0;
        for(const QString &hour : std::as_const(hourlist)) {
            c = new Cell(xi, yi);
            c->set_size(DAY_WIDTH, HOUR_HEIGHT);
            c->set_border(2.0, GRIDLINECOLOUR);
            rows.append(c);
            scene->addItem(c);
            c->setPos(x, y);
            y += HOUR_HEIGHT;
            yi++;
        }
        cols.append(rows);
        x += DAY_WIDTH;
        xi++;
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
    Cell *cell = cols[col + 1][row + 1];
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
