#include "BCLDiag.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  BCLDiag window;
  window.show();

  return app.exec();
}
