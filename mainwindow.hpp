#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "./ui_mainwindow.h"

#include <QMainWindow>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);

private:
  Ui::MainWindow ui;
};
#endif // MAINWINDOW_HPP
