#include "tt_grid.h"
#include "readxml.h"
#include "fetdata.h"
#include "database.h"
#include <QFileDialog>
#include <iostream>

#include <QJsonArray>
#include "lessontiles.h"

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

void TT_Grid::test(QList<QGraphicsItem *> items)
{
    scene->clear();
    setup_grid();

    auto fileName = QFileDialog::getOpenFileName(nullptr,
        ("Open XML file"), "", ("XML Files (*.xml *.fet)"));
    QFile data(fileName);
    if (data.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream indat(&data);
        QString tdat = indat.readAll();
        XMLNode xml = readXMLTree(tdat);

        auto fetdata = fetData(xml);

        class_divisions(fetdata);
        for (int course_id : fetdata.course_list) {
            auto cdata = fetdata.nodes[course_id].DATA;
            auto groups = cdata["STUDENTS"].toArray();
            auto llist = course_divisions(fetdata, groups);
            QStringList ll;
            for (auto iter = llist.cbegin(), end = llist.cend();
                 iter != end; ++iter) {
                int cl = iter.key();
                for (const auto &llf : iter.value()) {
                    ll.append(QString("%1(%2:%3:%4:%5)")
                                  .arg(cl)
                                  .arg(llf.offset)
                                  .arg(llf.fraction)
                                  .arg(llf.total)
                                  .arg(llf.groups.join(",")));
                }
            }
            qDebug() << "COURSE TILES" << cdata["SUBJECT"]
                     << groups << "->" << ll.join(",");
        }

        auto dbpath = fileName.section(".", 0, -2) + ".sqlite";
        save_data(dbpath, fetdata.nodes);
        qDebug() << "Saved data to" << dbpath;

        return;

        for (const auto &l : printXMLNode(xml)) {
            std::cout << qPrintable(l) << std::endl;
        }

    }
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


Tile::Tile(TT_Grid *grid, QJsonObject data) : Chip()
{
    grid->scene->addItem(this);
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
