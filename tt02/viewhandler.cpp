#include "viewhandler.h"
#include "readxml.h"
#include "fetdata.h"
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

void ViewHandler::handle_load_file()
{
    auto fileName = QFileDialog::getOpenFileName(nullptr,
        ("Open XML file"), "", ("XML Files (*.xml *.fet)"));
    QFile fdata(fileName);
    if (!fdata.open(QFile::ReadOnly | QIODevice::Text)) return;

    QTextStream indat(&fdata);
    QString tdat = indat.readAll();
    XMLNode xml = readXMLTree(tdat);

    auto fetdata = fetData(xml);
    if (dbdata) delete dbdata;
    dbdata = new DBData(fetdata.nodes);
    auto dbpath = fileName.section(".", 0, -2) + ".sqlite";
    dbdata->save(dbpath);
    qDebug() << "Saved data to" << dbpath;
    new_timetable_data();
}

void ViewHandler::new_timetable_data()
{
    QStringList dlist; // Ordered list of days (short-names)
    for (int d : dbdata->Tables["DAYS"]) {
        auto node = dbdata->Nodes.value(d);
        auto day = node.value("NAME").toString();
        if (day.isEmpty()) {
            day = node.value("TAG").toString();
        }
        dlist.append(day);
    }
    QStringList hlist; // Ordered list of hours (short-names)
    QList<int> breaks; // Ordered list of hour indexes which follow breaks
    // Determine the breaks by looking at the start and end times of
    // the periods
    int t0 = 0;
    int i = 0;
    for (int h : dbdata->Tables["HOURS"]) {
        hlist.append(dbdata->get_tag(h));
        auto node = dbdata->Nodes.value(h);
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

    // If a grid already exists, delete it. Then make a new (empty) one.
    if (grid) delete grid;
    grid = new TT_Grid(view, dlist, hlist, breaks);

    // Make course lists for all classes and teachers.
    if (ttdata) delete ttdata;
    ttdata = new TimetableData(dbdata);
    viewtype->setEnabled(true);

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
        int lix = basic_constraints->lid2lix[lid];
        auto &ldata = basic_constraints->lessons[lix];
        qDebug() << "TILE CLICKED:" << day << hour
                 << QString("[%1|%2]").arg(tile->ref).arg(lid);
        grid->clearCellOK();
        // Select tile
        grid->select_tile(tile);
        // Seek possible placements
        //TODO: parallel lessons
        //grid->clearCellOK();

        if (!ldata.start_cells) {
            // This should be a "fixed" lesson
            if (ldata.fixed) {
                qDebug() << "FIXED";
                return;
            }

            // Go through all cells
//TODO: Actually, shouldn't ALL non-fixed cells have a start_cells list?
            return;
        }

        qDebug() << "START-CELLS:" << ldata.start_cells;

//TODO: Use a member variable for slot_array?
        std::vector<std::vector<int>> slot_array{*ldata.start_cells};
        basic_constraints->find_slots(slot_array, lix);
        //qDebug() << "  -> free:" << basic_constraints->found_slots;
        for (const auto [d, h] : basic_constraints->found_slots) {
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
        auto node = dbdata->Nodes.value(c);
        choice->addItem(QString("%2: %1")
            .arg(node.value("NAME").toString(),
            node.value("TAG").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_rb_teacher()
{
    choice->clear();
    indexmap.clear();
    for (int c : dbdata->Tables.value("TEACHERS")) {
        auto node = dbdata->Nodes.value(c);
        choice->addItem(QString("%2: %1")
                            .arg(node.value("NAME").toString(),
                                 node.value("TAG").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_rb_room()
{
    choice->clear();
    indexmap.clear();
    for (int c : dbdata->Tables.value("ROOMS")) {
        auto node = dbdata->Nodes.value(c);
        // Don't show room groups
        if (node.contains("FIXED_ROOMS")) continue;
        choice->addItem(QString("%2: %1")
                            .arg(node.value("NAME").toString(),
                                 node.value("TAG").toString()));
        indexmap.append(c);
    }
}

void ViewHandler::handle_item_chosen(int index)
{
    grid->scene->clear();
    grid->setup_grid();
    // Which type of item is being handled?
    if (rb_class->isChecked()) {
        ShowClass(grid, ttdata, indexmap.value(index));
    } else if (rb_teacher->isChecked()) {
        ShowTeacher(grid, ttdata, indexmap.value(index));
    } else if (rb_room->isChecked()) {
        ShowRoom(grid, dbdata, indexmap.value(index));
    }
}
