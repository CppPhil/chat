#include "mainwindow.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow{parent}
{
  ui.setupUi(this);
  setTitle();
}

void MainWindow::setTitle()
{
  setWindowTitle("Chat application");
}
