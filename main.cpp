#include <QtGui/QApplication>
#include "colorcreator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ColorCreator w;
    w.show();

    return a.exec();
}
