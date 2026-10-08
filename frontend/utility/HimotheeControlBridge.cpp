#include "HimotheeControlBridge.hpp"
#include "HimotheeActionRegistry.hpp"
#include <widgets/OBSBasic.hpp>
#include <QAbstractSocket>
#include <QApplication>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QByteArray>
#include <QObject>
#include <memory>

namespace {
constexpr quint16 PORT = 3294;
constexpr qsizetype MAX_REQUEST = 65536;
void Reply(QTcpSocket *socket, int status, const QJsonObject &payload)
{
 const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
 const QByteArray reason = status == 200 ? "OK" : status == 400 ? "Bad Request" :
                           status == 403 ? "Forbidden" : status == 404 ? "Not Found" :
                           status == 405 ? "Method Not Allowed" : status == 413 ? "Payload Too Large" :
                           status == 503 ? "Service Unavailable" : "Error";
 QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n"
   "Content-Type: application/json; charset=utf-8\r\n"
   "Cache-Control: no-store\r\n"
   "X-Content-Type-Options: nosniff\r\n"
   "Access-Control-Allow-Origin: null-not-allowed\r\n"
   "Connection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
 socket->write(response);
 socket->disconnectFromHost();
}
QJsonObject Error(const char *code, const char *message)
{
 return {{"success", false}, {"code", QString::fromLatin1(code)}, {"message", QString::fromLatin1(message)}};
}
void Handle(QTcpSocket *socket, OBSBasic *main, const QByteArray &request)
{
 const qsizetype headerEnd = request.indexOf("\r\n\r\n");
 if (headerEnd < 0) { Reply(socket, 400, Error("bad_request", "Invalid HTTP request.")); return; }
 const QList<QByteArray> lines = request.left(headerEnd).split('\n');
 if (lines.isEmpty()) { Reply(socket, 400, Error("bad_request", "Missing request line.")); return; }
 const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
 if (requestLine.size() != 3 || !requestLine[2].startsWith("HTTP/1.")) {
  Reply(socket, 400, Error("bad_request", "Invalid HTTP request line.")); return;
 }
 const QByteArray method = requestLine[0];
 const QByteArray path = requestLine[1];
 QByteArray host, client, contentType;
 int contentLength = 0;
 bool lengthSeen = false;
 for (int i = 1; i < lines.size(); ++i) {
  const QByteArray line = lines[i].trimmed();
  const int colon = line.indexOf(':');
  if (colon <= 0) { Reply(socket, 400, Error("bad_request", "Malformed header.")); return; }
  const QByteArray key = line.left(colon).trimmed().toLower();
  const QByteArray value = line.mid(colon + 1).trimmed();
  if (key == "host") host = value.toLower();
  else if (key == "x-himothee-client") client = value;
  else if (key == "content-type") contentType = value.toLower();
  else if (key == "content-length") {
   if (lengthSeen) { Reply(socket, 400, Error("bad_request", "Duplicate content length.")); return; }
   lengthSeen = true;
   bool ok = false;
   contentLength = value.toInt(&ok);
   if (!ok || contentLength < 0 || contentLength > MAX_REQUEST) {
    Reply(socket, 413, Error("request_too_large", "Invalid content length.")); return;
   }
  }
  else if (key == "transfer-encoding") {
   Reply(socket, 400, Error("bad_request", "Chunked requests are unsupported.")); return;
  }
 }
 if (host != "127.0.0.1:3294" && host != "localhost:3294") {
  Reply(socket, 403, Error("invalid_host", "Only localhost host headers are permitted.")); return;
 }
 auto *registry = main ? main->GetHimotheeActionRegistry() : nullptr;
 if (!registry || QThread::currentThread() != qApp->thread()) {
  Reply(socket, 503, Error("not_ready", "Action Registry is not ready.")); return;
 }
 if (method == "GET" && path == "/v1/health") {
  Reply(socket, 200, {{"name", "Himothee Studio"}, {"bridge_version", 1}, {"ready", true}});
 } else if (method == "GET" && path == "/v1/actions") {
  Reply(socket, 200, {{"actions", registry->ActionsJson()}});
 } else if (method == "GET" && path == "/v1/state") {
  Reply(socket, 200, registry->StateSnapshot());
 } else if (method == "POST" && path == "/v1/execute") {
  // Browser fetch cannot supply this non-simple header without CORS preflight.
  if (client != "stream-deck" && client != "himothee-controller") {
   Reply(socket, 403, Error("client_required", "A recognised local controller header is required.")); return;
  }
  if (!contentType.startsWith("application/json") || !lengthSeen) {
   Reply(socket, 400, Error("bad_request", "JSON content type and content length required.")); return;
  }
  const QByteArray body = request.mid(headerEnd + 4, contentLength);
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
   Reply(socket, 400, Error("bad_json", "Expected a JSON object.")); return;
  }
  const QJsonObject command = document.object();
  const QString id = command.value("action_id").toString();
  if (id.isEmpty() || (!command.value("params").isUndefined() && !command.value("params").isObject())) {
   Reply(socket, 400, Error("invalid_action", "action_id and optional object params required.")); return;
  }
  Reply(socket, 200, registry->Execute(id, command.value("params").toObject()).ToJson());
 } else if (method != "GET" && method != "POST") {
  Reply(socket, 405, Error("method_not_allowed", "Only GET and POST are supported."));
 } else {
  Reply(socket, 404, Error("not_found", "Unknown endpoint."));
 }
}
} // namespace

HimotheeControlBridge::HimotheeControlBridge(OBSBasic *main_) : main(main_), server(std::make_unique<QTcpServer>())
{
 QObject::connect(server.get(), &QTcpServer::newConnection, main, [this]() { Accept(); });
 if (!server->listen(QHostAddress::LocalHost, PORT)) {
  error = server->errorString();
 }
}
HimotheeControlBridge::~HimotheeControlBridge() = default;
bool HimotheeControlBridge::IsListening() const { return server && server->isListening(); }
quint16 HimotheeControlBridge::Port() const { return IsListening() ? server->serverPort() : 0; }
QString HimotheeControlBridge::Error() const { return error; }
void HimotheeControlBridge::Accept()
{
 while (server->hasPendingConnections()) {
  auto *socket = server->nextPendingConnection();
  if (!socket->peerAddress().isLoopback()) {
   socket->disconnectFromHost(); socket->deleteLater(); continue;
  }
  auto buffer = std::make_shared<QByteArray>();
  QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, buffer, main = this->main]() {
   buffer->append(socket->readAll());
   if (buffer->size() > MAX_REQUEST) {
    Reply(socket, 413, Error("request_too_large", "Request exceeds 64 KiB.")); return;
   }
   const qsizetype split = buffer->indexOf("\r\n\r\n");
   if (split < 0) return;
   int length = 0;
   const auto headers = buffer->left(split).split('\n');
   for (const auto &line : headers) {
    if (line.trimmed().toLower().startsWith("content-length:")) {
     bool ok = false;
     length = line.mid(line.indexOf(':') + 1).trimmed().toInt(&ok);
     if (!ok || length < 0 || length > MAX_REQUEST) {
      Reply(socket, 413, Error("request_too_large", "Invalid content length.")); return;
     }
    }
   }
   if (buffer->size() < split + 4 + length) return;
   Handle(socket, main, *buffer);
  });
  QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
 }
}
