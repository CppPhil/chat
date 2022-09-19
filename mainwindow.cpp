#include <cctype>

#include <optional>
#include <stdexcept>
#include <string>

#include <QIODeviceBase>
#include <QMessageBox>

#include "lib/client_list_message.hpp"
#include "lib/net_string.hpp"
#include "lib/ports.hpp"

#include "mainwindow.hpp"

namespace {
char peek(QTcpSocket& tcpSocket)
{
  char buffer{'\xFF'};
  tcpSocket.peek(&buffer, static_cast<qint64>(sizeof(buffer)));
  return buffer;
}

void consume(QTcpSocket& tcpSocket)
{
  char buffer{'\xFF'};
  tcpSocket.read(&buffer, static_cast<qint64>(sizeof(buffer)));
}

std::optional<long long> parseNumber(const std::string& string)
{
  try {
    return std::stoll(string);
  }
  catch (const std::invalid_argument&) {
    return std::nullopt;
  }
  catch (const std::out_of_range&) {
    return std::nullopt;
  }
}
} // anonymous namespace

MainWindow::MainWindow(const QHostAddress& serverAddress, QWidget* parent)
  : QMainWindow{parent}, m_serverAddress{serverAddress}, m_tcpSocket{}
{
  ui.setupUi(this);
  setTitle();
  setupTcpSocket();
}

MainWindow::~MainWindow()
{
  m_tcpSocket.disconnectFromHost();
}

void MainWindow::onServerConnectionLost()
{
  QMessageBox::critical(this, "Connection lost", "Lost connection to server");
  close();
}

void MainWindow::onServerReadyRead()
{
  // TODO: Handle errors

  const char firstByte{peek(m_tcpSocket)};

  if (firstByte == '\0') {
    return;
  }

  if (!std::isdigit(firstByte)) {
    close();
    return;
  }

  std::string buffer{};
  char        character{};

  while (std::isdigit(character = peek(m_tcpSocket))) {
    buffer.push_back(character);
    consume(m_tcpSocket);
  }

  const std::optional<long long> optionalStringLength{parseNumber(buffer)};

  if (!optionalStringLength.has_value()) {
    close();
    return;
  }

  const long long stringLength{*optionalStringLength};

  if (stringLength <= 0) {
    close();
    return;
  }

  const std::string::size_type oldStringLength{buffer.size()};
  buffer.resize(
    oldStringLength + stringLength + 1 + 1); // + 1 for the : and + 1 for the ,
  m_tcpSocket.read(buffer.data() + oldStringLength, stringLength + 1 + 1);

  try {
    const lib::NetString netString{
      lib::FromNetStringData{}, buffer.data(), buffer.size()};
    const std::string            json{netString.asPlainString()};
    const lib::ClientListMessage clientListMessage{json};
  }
  catch (const std::runtime_error&) {
    close();
    return;
  }
}

void MainWindow::setupTcpSocket()
{
  connect(
    &m_tcpSocket,
    &QAbstractSocket::disconnected,
    this,
    &MainWindow::onServerConnectionLost);
  connect(
    &m_tcpSocket,
    &QAbstractSocket::errorOccurred,
    this,
    &MainWindow::onServerConnectionLost);
  connect(
    &m_tcpSocket, &QIODevice::readyRead, this, &MainWindow::onServerReadyRead);
  m_tcpSocket.connectToHost(
    /* address */ m_serverAddress,
    /* port */ lib::tcpPort,
    /* openMode */ QIODeviceBase::ReadOnly);
}

void MainWindow::setTitle()
{
  setWindowTitle("Chat application");
}
