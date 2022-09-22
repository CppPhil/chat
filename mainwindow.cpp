#include <cctype>

#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>

#include <QIODeviceBase>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include <pl/algo/ranged_algorithms.hpp>

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

std::vector<std::string> collectHostMachineAddresses()
{
  const QList<QHostAddress> list{QNetworkInterface::allAddresses()};
  std::vector<std::string>  result(static_cast<std::size_t>(list.size()));

  for (qsizetype i{0}; i < list.size(); ++i) {
    const QString     qString{list[i].toString()};
    const std::string stdString{qString.toStdString()};
    result[i] = stdString;
  }

  return result;
}
} // anonymous namespace

MainWindow::MainWindow(const QHostAddress& serverAddress, QWidget* parent)
  : QMainWindow{parent}
  , m_serverAddress{serverAddress}
  , m_tcpSocket{}
  , m_peerAddresses{}
  , m_udpSocket{}
{
  ui.setupUi(this);
  setTitle();
  setupTcpSocket();
  setupUdpSocket();

  connect(
    ui.sendChatMessagePushButton,
    &QAbstractButton::clicked,
    this,
    &MainWindow::onSendMessageButtonClicked);
}

MainWindow::~MainWindow()
{
  m_tcpSocket.disconnectFromHost();
}

void MainWindow::onServerConnectionLost()
{
  close();
}

void MainWindow::onServerReadyRead()
{
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
    const std::string              json{netString.asPlainString()};
    const lib::ClientListMessage   clientListMessage{json};
    const std::vector<std::string> ipAddresses{clientListMessage.ipAddresses()};
    const std::vector<std::string> hostMachineAddresses{
      collectHostMachineAddresses()};
    std::vector<std::string> peerAddresses{};
    pl::algo::copy_if(
      ipAddresses,
      std::back_inserter(peerAddresses),
      [&hostMachineAddresses](const std::string& ipAddress) {
        return pl::algo::none_of(
          hostMachineAddresses, [&ipAddress](const std::string& hostAddress) {
            return hostAddress == ipAddress;
          });
      });
    m_peerAddresses = std::move(peerAddresses);
    showPeerAddressesInGui();
  }
  catch (const std::runtime_error&) {
    close();
    return;
  }
}

void MainWindow::onUdpSocketReadyRead()
{
  while (m_udpSocket.hasPendingDatagrams()) {
    const QNetworkDatagram datagram{m_udpSocket.receiveDatagram()};
    const QString          message{QString{"%1: %2"}.arg(
      datagram.senderAddress().toString(), QString{datagram.data()})};
    ui.chatMessagesTextEdit->append(message);
  }
}

void MainWindow::onSendMessageButtonClicked()
{
  const QString message{ui.chatMessageToSendLineEdit->text()};

  if (message.isEmpty()) {
    return;
  }

  const QHostAddress hostAddress{ui.chatPeersComboBox->currentText()};

  if (hostAddress.isNull()) {
    return;
  }

  ui.chatMessageToSendLineEdit->clear();
  const QByteArray utf8{message.toUtf8()};
  m_udpSocket.writeDatagram(utf8, hostAddress, lib::udpPort);
  ui.chatMessagesTextEdit->append(QString{"You: %1"}.arg(QString{utf8}));
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

void MainWindow::setupUdpSocket()
{
  if (!m_udpSocket.bind(QHostAddress::Any, lib::udpPort)) {
    close();
    return;
  }

  connect(
    &m_udpSocket,
    &QIODevice::readyRead,
    this,
    &MainWindow::onUdpSocketReadyRead);
}

void MainWindow::setTitle()
{
  setWindowTitle("Chat application");
}

void MainWindow::showPeerAddressesInGui()
{
  const QString previousSelection{ui.chatPeersComboBox->currentText()};
  ui.chatPeersComboBox->clear();

  for (const std::string& address : m_peerAddresses) {
    ui.chatPeersComboBox->addItem(QString::fromStdString(address));
  }

  const int index{ui.chatPeersComboBox->findText(previousSelection)};

  if (index != -1) {
    ui.chatPeersComboBox->setCurrentIndex(index);
  }
}
