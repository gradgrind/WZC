#include "viewhandler.h"
#include "showclass.h"
#include <QBoxLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QListWidget>

ViewHandler::ViewHandler() : QWidget()
{
    auto box1 = new QVBoxLayout(this);
    auto load_file = new QPushButton("Datei öffnen");
    box1->addWidget(load_file);
    connect(
        load_file, &QPushButton::clicked,
        this, &ViewHandler::handle_load_file);
    auto viewtype = new QWidget();
    auto rb_class = new QRadioButton("Klasse");
    auto rb_teacher = new QRadioButton("Lehrer(in)");
    auto rb_room = new QRadioButton("Raum");
    auto box2 = new QVBoxLayout(viewtype);
    box1->addWidget(viewtype);
    box2->addWidget(rb_class);
    box2->addWidget(rb_teacher);
    box2->addWidget(rb_room);
    choice = new QListWidget();
    box1->addWidget(choice);
    connect(
        rb_class, &QRadioButton::clicked,
        this, &ViewHandler::handle_rb_class);

}

void ViewHandler::handle_load_file()
{
    //TODO
}

void ViewHandler::set_data(DBData *db_data, TT_Grid *gridref)
{
    qDebug() << "ViewHandler::set_data()";
    dbdata = db_data;
    grid = gridref;
}

void ViewHandler::handle_rb_class()
{
    choice->clear();
    for (int c : dbdata->Tables.value("CLASSES")) {
        auto node = dbdata->Nodes.value(c).DATA;
        choice->addItem(QString("%1 (%2)")
            .arg(node.value("NAME").toString(),
            node.value("ID").toString()));
    }
}

void ViewHandler::handle_class_chosen(int index)
{
    grid->scene->clear();
    grid->setup_grid();
    int c = dbdata->Tables.value("CLASSES").value(index);
    ShowClass(grid, dbdata, c);
}
