#include <cstdlib>

#include <QApplication>
#include <QHostAddress>
#include <QMessageBox>
#include <QStringList>

#include "mainwindow.hpp"

int main(int argc, char* argv[])
{
  QApplication       application{argc, argv};
  const QStringList  arguments{QCoreApplication::arguments()};
  const QString&     lastArgument{arguments.last()};
  const QHostAddress hostAddress{lastArgument};

  if (hostAddress.isNull()) {
    QMessageBox messageBox{
      /* icon */ QMessageBox::Critical,
      /* title */ "Invalid command line arguments",
      /* text */
      QString{
        "\"%1\" is not a valid IP address.\n\n Example:\n  %2 192.168.178.24"}
        .arg(lastArgument, QString(argv[0])),
      /* buttons */ QMessageBox::Ok};
    messageBox.exec();
    return EXIT_FAILURE;
  }

  MainWindow mainWindow{};
  mainWindow.show();
  return application.exec();
}
