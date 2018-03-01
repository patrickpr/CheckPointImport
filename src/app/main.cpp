#include "mainimportwin.h"
#include <QApplication>
#include <QLibrary>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  MainImportWin w;
  w.show();

  return a.exec();
}
