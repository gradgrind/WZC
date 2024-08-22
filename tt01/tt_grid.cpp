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

//[&] (const std::string& message)

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


Tile::Tile(TT_Grid *grid, QJsonObject data) : Chip()
{
    grid->scene->addItem(this);
    tag = data.value("TAG").toString();
    length = data.value("LENGTH").toInt();
    divs = data.value("DIVS").toInt();
    div0 = data.value("DIV0").toInt();
    ndivs = data.value("NDIVS").toInt();
    middle = data.value("TEXT").toString();
    tl = data.value("TL").toString();
    tr = data.value("TR").toString();
    bl = data.value("BL").toString();
    br = data.value("BR").toString();

    QString bg = data.value("BACKGROUND").toString("FFFFFF");
    set_background(bg);
    QJsonObject settings = grid->settings;
    set_border(settings.value(
        "TILE_BORDER_WIDTH").toDouble(TILE_BORDER_WIDTH));

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
     * The size of the tile can be changed by supplying new width and
     * height values.
     * If no change of size is desired, just call the "setPos" method.
     */
    setRect(0, 0, w, h);
    setPos(x, y);
    // Handle the text field placement and potential shrinking
    set_text(middle);
    set_toptext(tl, tr);
    set_bottomtext(bl, br);
}
