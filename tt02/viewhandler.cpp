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
#include <QMessageBox>

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
    grid->setClickHandler([this](int d, int h, Tile *t, int km){
        onClick(d, h, t, km);
    });
}

void ViewHandler::onClick(int day, int hour, Tile *tile, int keymod)
{
    // It would be good to handle the case where the only clash is with
    // a flexible room differently – this should need no replacement.
    if (keymod == 0) {
        if (tile) {
            selected_lid = tile->lid;
            show_available(tile);
        } else {
            qDebug() << "CELL CLICKED:" << day << hour;
            // Unselect tile
            selected_lid = 0;
            grid->select_tile(nullptr);
            grid->clearHighlights();
        }
        return;
    }
    if (keymod == 1) {
        if (selected_lid != 0) {
            // There is a selected lesson, try to place it.
            int lix = basic_constraints->lid2lix.at(selected_lid);
            auto clashes = basic_constraints->find_clashes(lix, day, hour);
            if (clashes.empty()) {
                qDebug() << "PLACE:"
                         << basic_constraints->pr_lesson(lix);
            } else {
                // There are conflicts. Display them and offer to replace them.
                for (const auto &clash : clashes) {
                    if (clash.ctype != FLEXIROOM) goto blocked;
                }
                qDebug() << "PLACE (CLEAR FLEXIROOM):"
                         << basic_constraints->pr_lesson(lix);
            }
            return;

        blocked:

            QMessageBox msgBox;
            msgBox.setText("Conflicting Lessons. Do you want to remove them?");
//TODO? Reorganize the display?
// The DetailedText can have scroll-bars, but is initially hidden.
            msgBox.setDetailedText(
                QString("Place ") + basic_constraints->pr_lesson(lix));
            QStringList qsl;
            for (const auto &clash : clashes) {
                QString mstr = QString::number(clash.ctype) + ": ";

                //TODO: If blocked, a replacement won't help!
                if (clash.lesson_index < 0)
                    mstr += "*BLOCKED LESSON*";
                else mstr += basic_constraints
                                ->pr_lesson(clash.lesson_index);
                qsl.append(mstr);
            }
            msgBox.setInformativeText(qsl.join("\n"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            msgBox.setIcon(QMessageBox::Warning);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                qDebug() << "REPLACE";
                // Remove listed lessons (and anything hard-parallel)
                for (const auto &clash : clashes) {
                    int lixr = clash.lesson_index;
                    if (clash.ctype == FLEXIROOM) {
                        //TODO: Just remove flexiroom

                        continue;
                    }
                    //TODO--?
                    if (lixr < 0)
                        qFatal() << "Blocked lesson";

                    std::vector<int> lids = basic_constraints
                                                ->unplace_lesson_full(lixr);
                    for (int lid : lids) {
                        auto tile = grid->lid2tile.value(lid, nullptr);
                        if (tile)
                            tile->hide();
                    }
                }

                //TODO: Place the new lesson
            }

        } else {
            QMessageBox::information(
                this, "Place Lesson", "No Lesson is Selected.");
        }

        // After a change, update the database. Either update the internal
        // data structures (if that is not too complicated) or reload them
        // from the modified database. In the case of these simple placement
        // and removal operations it may well be possible to avoid a complete
        // reloading.

        // Reset grid selection (somehow).

        // If there is no selected lesson, do nothing (maybe report it).
    } else qDebug() << "$$$ keymod =" << keymod;
}

void ViewHandler::show_available(Tile *tile)
{
    int lid = tile->lid;
    int lix = basic_constraints->lid2lix[lid];
    auto &ldata = basic_constraints->lessons[lix];
    grid->clearHighlights();
    // Select tile
    grid->select_tile(tile);
    // Seek possible placements
    //TODO: parallel lessons
    //grid->clearCellOK();

    if (ldata.fixed) {
        qDebug() << "FIXED";
        return;
    }

    //TODO: Maybe each lesson cell in the grid could have an overlay
    // "highlight" rectangle, just inside the border, perhaps not opaque,
    // which can be shown in various colours?

    auto freeslots = basic_constraints->available_slots(lix);
    QString out{"  -> free:"};

    for (auto const& [ttslot, clashes] : freeslots)
    {
        if (clashes.empty()) {
            grid->setHighlight(ttslot.day, ttslot.hour, NOCLASH);
            continue;
        }
        for (const auto &clash : clashes) {
            if (clash.ctype != FLEXIROOM) goto blocked;
        }
        grid->setHighlight(ttslot.day, ttslot.hour, ONLY_FLEXIROOM);
        continue;
    blocked:
        //TODO: Remove testing code
        if (ttslot.day % 2 == 0)
            grid->setHighlight(ttslot.day, ttslot.hour, REPLACEABLE);
        else
            grid->setHighlight(ttslot.day, ttslot.hour, ONLY_FLEXIROOM);
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
    selected_lid = 0;
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
