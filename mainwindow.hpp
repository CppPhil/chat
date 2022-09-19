#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP
#include <string>
#include <vector>

#include <QHostAddress>
#include <QMainWindow>
#include <QTcpSocket>

#include "./ui_mainwindow.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(
    const QHostAddress& serverAddress,
    QWidget*            parent = nullptr);

  Q_DISABLE_COPY_MOVE(MainWindow)

  ~MainWindow() override;

private slots:
  void onServerConnectionLost();

  void onServerReadyRead();

private:
  void setupTcpSocket();

  void setTitle();

  void showPeerAddressesInGui();

  Ui::MainWindow           ui;
  const QHostAddress       m_serverAddress;
  QTcpSocket               m_tcpSocket;
  std::vector<std::string> m_peerAddresses;
};
#endif // MAINWINDOW_HPP
