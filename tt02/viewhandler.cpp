#include "viewhandler.h"
#include "readxml.h"
#include "fetdata.h"
#include "lessontiles.h"
#include "showclass.h"
#include "showteacher.h"
#include "showroom.h"
#include "localconstraints.h"
#include <QBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QFileDialog>
#include <QJsonArray>

const int BREAK_MINS = 10;

ViewHandler::ViewHandler(QGraphicsView *gview) : QWidget(), view{gview}
{
    auto box1 = new QVBoxLayout(this);
    auto load_file = new QPushButton("Datei öffnen");
    box1->addWidget(load_file);
    connect(
        load_file, &QPushButton::clicked,
        this, &ViewHandler::handle_load_file);
    viewtype = new QWidget();
    rb_class = new QRadioButton("Klasse");
    rb_teacher = new QRadioButton("Lehrer(in)");
    rb_room = new QRadioButton("Raum");
    auto box2 = new QVBoxLayout(viewtype);
    box1->addWidget(viewtype);
    box2->addWidget(rb_class);
    box2->addWidget(rb_teacher);
    box2->addWidget(rb_room);
    // The selection controls should be disabled until a file has
    // been loaded.
    viewtype->setEnabled(false);
    choice = new QListWidget();
    box1->addWidget(choice);
    connect(
        rb_class, &QRadioButton::clicked,
        this, &ViewHandler::handle_rb_class);
    connect(
        rb_teacher, &QRadioButton::clicked,
        this, &ViewHandler::handle_rb_teacher);
    connect(
        rb_room, &QRadioButton::clicked,
        this, &ViewHandler::handle_rb_room);
    connect(
        choice, &QListWidget::currentRowChanged,
        this, &ViewHandler::handle_item_chosen);
}

ViewHandler::~ViewHandler()
{
    if (dbdata) delete dbdata;
    if (grid) delete grid;
    if (basic_constraints) delete basic_constraints;
}

//TODO: A second call results in a crash!
void ViewHandler::handle_load_file()
{
    auto fileName = QFileDialog::getOpenFileName(nullptr,
        ("Open XML file"), "", ("XML Files (*.xml *.fet)"));
    QFile data(fileName);
    if (!data.open(QFile::ReadOnly | QIODevice::Text)) return;

    QTextStream indat(&data);
    QString tdat = indat.readAll();
    XMLNode xml = readXMLTree(tdat);

    auto fetdata = fetData(xml);
    if (dbdata) delete dbdata;
    dbdata = new DBData(fetdata.nodes);

    // Make lesson lists for the courses.
    for (int lid : dbdata->Tables["LESSONS"]) {
        auto ldata = dbdata->Nodes[lid].DATA;
        dbdata->course_lessons[ldata["COURSE"].toInt()].append(lid);
    }
    QStringList dlist;
    for (int d : dbdata->Tables["DAYS"]) {
        auto node = dbdata->Nodes.value(d).DATA;
        auto day = node.value("NAME").toString();
        if (day.isEmpty()) {
            day = node.value("ID").toString();
        }
        dlist.append(day);
    }
    QStringList hlist;
    QList<int> breaks;
    // Determine the breaks by looking at the start and end times of
    // the periods
    int t0 = 0;
    int i = 0;
    for (int h : dbdata->Tables["HOURS"]) {
        hlist.append(dbdata->get_tag(h));
        auto node = dbdata->Nodes.value(h).DATA;
        int t1 = time2mins(node.value("START_TIME").toString());
        int t2 = time2mins(node.value("END_TIME").toString());
        if (t1 >= 0 and t2 >= 0) {
            if (i != 0 && (t1 - t0 >= BREAK_MINS)) {
                    breaks.append(i);
            }
            t0 = t2;
        }
        i++;
    }
    if (grid) delete grid;
    grid = new TT_Grid(view, dlist, hlist, breaks);

    // Make course lists for all classes and teachers.

    // Start with an analysis of the divisions and their groups/subgroups
    // for all classes. The results of this are used by the calls to
    // course_divisions.
    class_divisions(dbdata);

    // Collect the group tile information for each course,
    // add the course-ids to the lists for the classes and teachers.
    for (int course_id : dbdata->Tables["COURSES"]) {
        auto cdata = dbdata->Nodes[course_id].DATA;
        auto groups = cdata["STUDENTS"].toArray();
        auto llist = course_divisions(dbdata, groups);

        dbdata->course_tileinfo[course_id] = llist;
        auto tlist = cdata.value("TEACHERS").toArray();
        for (const auto &t : tlist) {
            int tid = t.toInt();
            dbdata->teacher_courses[tid].append(course_id);
        }
        for (auto iter = llist.cbegin(), end = llist.cend();
                iter != end; ++iter) {
            int cl = iter.key();
            dbdata->class_courses[cl].append(course_id);
        }
        viewtype->setEnabled(true);

        /*
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
        QString subject = dbdata->get_tag(cdata.value("SUBJECT").toInt());
        QStringList teachers;
        for (const auto &t : tlist) {
            teachers.append(dbdata->get_tag(t.toInt()));
        }
        qDebug() << "COURSE TILES" << subject << teachers.join(",")
                 << groups << "->" << ll.join(",");
        */
    }

    auto dbpath = fileName.section(".", 0, -2) + ".sqlite";
    dbdata->save(dbpath);
    qDebug() << "Saved data to" << dbpath;

    if (basic_constraints) delete basic_constraints;
    basic_constraints = new BasicConstraints(dbdata);
    localConstraints(basic_constraints);

    grid->setClickHandler([this](int d, int h, Tile *t){
        onClick(d, h, t);
    });
}

void ViewHandler::onClick(int day, int hour, Tile *tile) {
    if (tile) {
        int lid = tile->lid;
        auto ldata = &basic_constraints->lessons[
            basic_constraints->lid2lix[lid]];
        qDebug() << "TILE CLICKED:" << day << hour
                 << QString("[%1|%2]").arg(tile->tag).arg(lid);
        grid->clearCellOK();
        // Select tile
        grid->select_tile(tile);
        // Seek possible placements
        //TODO: parallel lessons
        //grid->clearCellOK();

        if (ldata->start_cells.empty()) {
            // This should be a "fixed" lesson
            if (ldata->fixed) {
                qDebug() << "FIXED";
                return;
            }

            // Go through all cells
//TODO: Actually, shouldn't ALL non-fixed cells have a start_cells list?
            return;
        }

        qDebug() << "START-CELLS:" << ldata->start_cells;
//TODO: The start-cells seem too restrictive. Are the wrong cells getting
// added somehow?
        auto free = basic_constraints->find_possible_places(ldata);
        for (int d = 0; d < basic_constraints->ndays; ++d) {
            const auto &dvec = free[d];
            for (int h : dvec)
                grid->setCellOK(d, h);
        }
    } else {
        qDebug() << "CELL CLICKED:" << day << hour;
        // Unselect tile
        grid->select_tile(nullptr);
        grid->clearCellOK();
    }
}

void ViewHandler::handle_rb_class()
{
    choice->clear();
    indexmap.clear();
    for (int c : dbdata->Tables.value("CLASSES")) {
        auto node = dbdata->Nodes.value(c).DATA;
        choice->addItem(QString("%2: %1")
            .arg(node.value("NAME").toString(),
            node.value("ID").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_rb_teacher()
{
    choice->clear();
    indexmap.clear();
    for (int c : dbdata->Tables.value("TEACHERS")) {
        auto node = dbdata->Nodes.value(c).DATA;
        choice->addItem(QString("%2: %1")
                            .arg(node.value("NAME").toString(),
                                 node.value("ID").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_rb_room()
{
    choice->clear();
    indexmap.clear();
    for (int c : dbdata->Tables.value("ROOMS")) {
        auto node = dbdata->Nodes.value(c).DATA;
        // Don't show room groups
        if (node.contains("ROOMS_NEEDED")) continue;
        choice->addItem(QString("%2: %1")
                            .arg(node.value("NAME").toString(),
                                 node.value("ID").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_item_chosen(int index)
{
    grid->scene->clear();
    grid->setup_grid();
    // Which type of item is being handled?
    if (rb_class->isChecked()) {
        ShowClass(grid, dbdata, indexmap.value(index));
    } else if (rb_teacher->isChecked()) {
        ShowTeacher(grid, dbdata, indexmap.value(index));
    } else if (rb_room->isChecked()) {
        ShowRoom(grid, dbdata, indexmap.value(index));
    }
}
