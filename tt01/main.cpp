#include "canvas.h"
#include <QBoxLayout>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QWidget mainwindow;
    QWidget right;
    right.setFixedWidth(200);

    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(255, 255, 200));
    right.setAutoFillBackground(true);
    right.setPalette(pal);

    QHBoxLayout hb(&mainwindow);

    QGraphicsView view;
    hb.addWidget(&view);
    hb.addWidget(&right);

    Canvas canv(&view);

    view.setFixedSize(A4.toSize());

    mainwindow.show();

    return a.exec();
}
