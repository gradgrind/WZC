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

    // Set up event handlers
    scene->set_click_handler([this](
            const QList<QGraphicsItem *> items, int keymod) {
        handle_click(items, keymod);
    });
    scene->set_context_menu_handler([this](
            const QList<QGraphicsItem *> items) {
        handle_context_menu(items);
    });
    hover_handler = [this](HoverRectItem* gitem, bool enter){
        handle_hover(gitem, enter);
    };
}

void TT_Grid::handle_click(QList<QGraphicsItem *> items, int keymod)
{
    Tile *tile = nullptr;
    int cellx = -100, celly = -100;
    for (const auto item : items) {
        Tile *t = qgraphicsitem_cast<Tile *>(item);
        if (t) {
            tile = t;
            continue;
        }
        Cell * cell = qgraphicsitem_cast<Cell *>(item);
        if (cell) {
            cellx = cell->cellx;
            celly = cell->celly;
            break;
        }
        // Ignore everything else
    }

    //TODO: What to do with keymod?
    if (click_handler) click_handler(cellx, celly, tile);
}

void TT_Grid::handle_context_menu(QList<QGraphicsItem *> items)
{
    Tile *tile = nullptr;
    int cellx = -100, celly = -100;
    for (const auto item : items) {
        Tile *t = qgraphicsitem_cast<Tile *>(item);
        if (t) {
            tile = t;
            continue;
        }
        Cell * cell = qgraphicsitem_cast<Cell *>(item);
        if (cell) {
            cellx = cell->cellx;
            celly = cell->celly;
            break;
        }
        // Ignore everything else
        }

    QString tiledata;
    if (tile) {
        tiledata = QString("[%1|%2]").arg(tile->tag).arg(tile->lid);
    }
    qDebug() << "CONTEXT MENU:" << cellx << celly
             << tiledata;

}

void TT_Grid::handle_hover(HoverRectItem* gitem, bool enter){
    Tile *tile = qgraphicsitem_cast<Tile *>(gitem);
    /*
    if (enter) {
        qDebug() << "ENTER" << tile->lid;
    } else {
        qDebug() << "EXIT" << tile->lid;
    }
    */
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
    // Add the tile-selection rectangle
    selection_rect = new QGraphicsRectItem();
    selection_rect->setPen(
        QPen(QBrush(QColor("#FF" + SELECTIONCOLOUR)), GRIDLINEWIDTH));
    scene->addItem(selection_rect);
    selection_rect->setZValue(10);
    selection_rect->hide();
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

void TT_Grid::setClickHandler(
    std::function<void (int day, int hour, Tile *tile)> handler)
{
    click_handler = handler;
}

void TT_Grid::select_tile(Tile *tile) {
    if (tile) {
        selection_rect->setRect(tile->rect());
        selection_rect->setPos(tile->pos());
        selection_rect->show();
    } else {
        selection_rect->hide();
    }
}

void TT_Grid::setCellOK(int day, int hour)
{
    auto cell = cols[day+1][hour+1];
    cell->setBrush(OKBRUSH);
    ok_cells.append(cell);
}

void TT_Grid::clearCellOK()
{
    QBrush b;
    for (const auto &cell : ok_cells) {
        cell->setBrush(b);
    }
    ok_cells.clear();
}


Tile::Tile(
    TT_Grid *grid,
    QJsonObject data,
    int lesson_id) : Chip()
{
    grid->scene->addItem(this);

    if (grid->hover_handler) {
        setHoverHandler(grid->hover_handler);
            //void (* handler)(QGraphicsRectItem*, bool))

    }

    lid = lesson_id;
    tag = data.value("TAG").toString();
    length = data.value("LENGTH").toInt(1);
    divs = data.value("DIVS").toInt(1);
    div0 = data.value("DIV0").toInt(0);
    ndivs = data.value("NDIVS").toInt(1);
    middle = data.value("TEXT").toString();
    tl = data.value("TL").toString();
    tr = data.value("TR").toString();
    bl = data.value("BL").toString();
    br = data.value("BR").toString();

    QString bg = data.value("BACKGROUND").toString("FFFFFF");
    set_background(bg);
    QJsonObject settings = grid->settings;
    set_border(settings.value(
                           "TILE_BORDER_WIDTH").toDouble(TILE_BORDER_WIDTH),
               settings.value(
                           "TILE_BORDER_COLOUR").toString(TILE_BORDER_COLOUR)
               );

    config_text(
        settings.value("TEXT_SIZE").toDouble(),
        settings.value("TEXT_BOLD").toBool(TEXT_BOLD),
        settings.value("TEXT_ALIGN").toInt(TEXT_ALIGN),
        settings.value("TEXT_COLOUR").toString());
    set_subtext_size(settings.value("SUBTEXT_SIZE").toDouble());
}

void Tile::place(qreal x, qreal y, qreal w, qreal h)
{
    /* The QGraphicsItem method "setPos" takes "float" coordinates,
     * either as setPos(x, y) or as setPos(QPointF). It sets the position
     * of the item in parent coordinates. For items with no parent, scene
     * coordinates are used.
     * The position of the item describes its origin (local coordinate
     * (0, 0)) in parent coordinates.
     * The size of the tile is set by means of the width and height values.
     * If no change of size is desired, just call the "setPos" method.
     */
    setRect(0, 0, w, h);
    setPos(x, y);
    // Handle the text field placement and potential shrinking
    set_text(middle);
    set_toptext(tl, tr);
    set_bottomtext(bl, br);
}
