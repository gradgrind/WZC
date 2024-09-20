#include "canvas.h"
//#include "tt_grid.h"
//#include "readxml.h"
#include "viewhandler.h"
//#include <QStyleFactory>
#include <QHBoxLayout>

int main(int argc, char *argv[])
{
    //qDebug() << QStyleFactory::keys();
    //QApplication::setStyle("fusion");
    QApplication a(argc, argv);
    QWidget mainwindow;
    QHBoxLayout hb(&mainwindow);
    QGraphicsView view;
    hb.addWidget(&view);
    ViewHandler right(&view);
    hb.addWidget(&right);
    right.setFixedWidth(200);

    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(255, 255, 200));
    right.setAutoFillBackground(true);
    right.setPalette(pal);

    /*
    //Canvas canv(&view);
    TT_Grid grid(&view,
        {"Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag"},
        {"HU A", "HU B", "FS 1", "FS 2", "FS 3", "FS 4", "FS 5", "FS 6", "FS 7"},
        {2, 4, 6});

    qDebug() << "@ (3, 1): " << grid.cols[4][2]->rect();
    qDebug() << "  ... " << grid.cols[4][2]->pos();

    Tile *t = new Tile(grid.scene, grid.settings, QJsonObject
        {
            {"TEXT", "Centre"},
            {"TR", "rTop"},
            {"LENGTH", 2},
            {"DIV0", 2},
            {"DIVS", 1},
            {"NDIVS", 3},
        }
    );
    grid.place_tile(t, 3, 1);
    //t->place(200, 300, 150, 150);

    t = new Tile(grid.scene, grid.settings,
        QJsonObject {
            {"TEXT", "Second"},
            {"BL", "lBottom"},
        }
    );
    grid.place_tile(t, 4, 6);
    //t->place(500, 250, 150, 150);
    */

    view.setFixedSize(A4.toSize());

    //readxml_test();

    mainwindow.show();

    return a.exec();
}

/*#DOC
The basic idea is to build a (school) timetable viewer and editor.

The timetable data is in an sqlite database, but the current approach
does not take advantage of the possibilities of an SQL database. Only
a single table is used, containing mostly JSON data. A single JSON file
would be a possible alternative. The database is handled in the module
"database".

How the data gets into the database is only touched on in the current code.
There are many possibilities, but at present there is only a reader for
certain files from the fet program (https://www.lalescu.ro/liviu/fet).
Primarily addressed are the result files "XXXXX_data_and_timetable.fet",
but – in principle – also the basic "XXXXX.fet" files should be useable.
Only a small subset of the fet constraints is imported, those which I
regard as essential basic "hard" constraints. It should be possible to
import more, but that is not a priority. I would prefer to develop my
own constraints. When these exist, it may be that certain fet constraints
can be transformed to these, but this is not currently planned.
The module "fetdata" manages this importing, using module "readxml" to
perform the low-level reading of the XML. There are also modules to deal
with reading the constraints: "readtimeconstraints" and
"readspaceconstraints".

The "canvas" module forms the basis for the timetable display using a
QGraphicsView/Scene. The "chip" module provides a rectangle item with
various useful properties (including text fields and basic support for
"hover" interaction). The "tt_grid" module builds on these to provide
a grid structure for the timetable and potentially interactive tiles
for the lessons.

Module "viewhandler" ties all the components together and manages the
loading and display of the timetable. The interaction is currently
under development.
*/
