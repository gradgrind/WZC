#include "tt_grid.h"
#include "readxml.h"
#include "fetdata.h"
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

void TT_Grid::test_setup(void (*func)(DBData *, TT_Grid *))
{
    setup_func = func;
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
        DBData dbdata(fetdata.nodes);
        setup_func(&dbdata, this);

        // Make lesson lists for the courses.
        for (int lid : dbdata.Tables["LESSONS"]) {
            auto ldata = dbdata.Nodes[lid].DATA;
            dbdata.course_lessons[ldata["COURSE"].toInt()].append(lid);
        }

        // Make course lists for all classes and teachers.

        // Start with an analysis of the divisions and their groups/subgroups
        // for all classes. The results of this are used by the calls to
        // course_divisions.
        class_divisions(dbdata);

        // Collect the group tile information for each course,
        // add the course-ids to the lists for the classes and teachers.
        for (int course_id : dbdata.Tables["COURSES"]) {
            auto cdata = dbdata.Nodes[course_id].DATA;
            auto groups = cdata["STUDENTS"].toArray();
            auto llist = course_divisions(dbdata, groups);

            dbdata.course_tileinfo[course_id] = llist;
            auto tlist = cdata.value("TEACHERS").toArray();
            for (const auto &t : tlist) {
                int tid = t.toInt();
                dbdata.teacher_courses[tid].append(course_id);
            }
            for (auto iter = llist.cbegin(), end = llist.cend();
                    iter != end; ++iter) {
                int cl = iter.key();
                dbdata.class_courses[cl].append(course_id);
            }

            // Print the item
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
            QString subject = dbdata.get_tag(cdata.value("SUBJECT").toInt());
            QStringList teachers;
            for (const auto &t : tlist) {
                teachers.append(dbdata.get_tag(t.toInt()));
            }
            qDebug() << "COURSE TILES" << subject << teachers.join(",")
                     << groups << "->" << ll.join(",");
        }

        auto dbpath = fileName.section(".", 0, -2) + ".sqlite";
        dbdata.save(dbpath);
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
