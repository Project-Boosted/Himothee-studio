#include "HimotheeOverlayEngine.hpp"

#include <widgets/OBSBasic.hpp>

#include <obs.hpp>

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <algorithm>
#include <filesystem>

using namespace std;

namespace {

constexpr const char *kOverlayFileName = "overlays.json";
constexpr uint16_t kOverlayServerPort = 3293;

const char *OverlayTypeName(HimotheeOverlayType type)
{
	switch (type) {
	case HimotheeOverlayType::Text:
	default:
		return "text";
	}
}

HimotheeOverlayType ParseOverlayType(const char *type)
{
	if (type && astrcmpi(type, "text") == 0) {
		return HimotheeOverlayType::Text;
	}
	return HimotheeOverlayType::Text;
}

} // namespace

HimotheeOverlayManager::HimotheeOverlayManager(OBSBasic *main_) : main(main_), serverPort(kOverlayServerPort)
{
	server = make_unique<QTcpServer>();
	QObject::connect(server.get(), &QTcpServer::newConnection, server.get(), [this]() { AcceptConnections(); });
	StartServer();
	LoadForCurrentProfile();
}

HimotheeOverlayManager::~HimotheeOverlayManager()
{
	if (server && server->isListening()) {
		server->close();
	}
}

bool HimotheeOverlayManager::LoadForCurrentProfile()
{
	if (!main) {
		return false;
	}

	const OBSProfile &profile = main->GetCurrentProfile();
	const string profilePath = profile.path.u8string();
	if (loadedProfilePath == profilePath) {
		return true;
	}

	loadedProfilePath = profilePath;
	return Load();
}

bool HimotheeOverlayManager::Load()
{
	overlays.clear();

	if (!main) {
		return false;
	}

	const OBSProfile &profile = main->GetCurrentProfile();
	const filesystem::path path = profile.path / filesystem::u8path(kOverlayFileName);
	loadedProfilePath = profile.path.u8string();

	OBSDataAutoRelease root = obs_data_create_from_json_file_safe(path.u8string().c_str(), "bak");
	if (!root) {
		blog(LOG_INFO, "[Himothee Overlays] No overlay file at '%s'; starting with an empty overlay list.",
		     path.u8string().c_str());
		return true;
	}

	OBSDataArrayAutoRelease array = obs_data_get_array(root, "overlays");
	if (!array) {
		return true;
	}

	const size_t count = obs_data_array_count(array);
	overlays.reserve(count);

	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(array, i);
		if (!item) {
			continue;
		}

		HimotheeOverlayDefinition overlay;
		overlay.id = obs_data_get_string(item, "id");
		overlay.name = obs_data_get_string(item, "name");
		overlay.type = ParseOverlayType(obs_data_get_string(item, "type"));
		overlay.visible = obs_data_has_user_value(item, "visible") ? obs_data_get_bool(item, "visible") : true;
		overlay.width = static_cast<uint32_t>(obs_data_get_int(item, "width"));
		overlay.height = static_cast<uint32_t>(obs_data_get_int(item, "height"));
		overlay.title = obs_data_get_string(item, "title");
		overlay.text = obs_data_get_string(item, "text");

		if (overlay.id.empty()) {
			overlay.id = "overlay-" + to_string(i + 1);
		}
		if (overlay.name.empty()) {
			overlay.name = "Overlay " + to_string(i + 1);
		}
		if (overlay.width == 0) {
			overlay.width = 1920;
		}
		if (overlay.height == 0) {
			overlay.height = 1080;
		}
		if (overlay.title.empty()) {
			overlay.title = overlay.name;
		}

		overlays.emplace_back(std::move(overlay));
	}

	blog(LOG_INFO, "[Himothee Overlays] Loaded %zu overlay definition(s).", overlays.size());
	return true;
}

bool HimotheeOverlayManager::Save() const
{
	if (!main) {
		return false;
	}

	const OBSProfile &profile = main->GetCurrentProfile();
	const filesystem::path path = profile.path / filesystem::u8path(kOverlayFileName);

	OBSDataAutoRelease root = obs_data_create();
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (const auto &overlay : overlays) {
		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, "id", overlay.id.c_str());
		obs_data_set_string(item, "name", overlay.name.c_str());
		obs_data_set_string(item, "type", OverlayTypeName(overlay.type));
		obs_data_set_bool(item, "visible", overlay.visible);
		obs_data_set_int(item, "width", overlay.width);
		obs_data_set_int(item, "height", overlay.height);
		obs_data_set_string(item, "title", overlay.title.c_str());
		obs_data_set_string(item, "text", overlay.text.c_str());
		obs_data_array_push_back(array, item);
	}

	obs_data_set_array(root, "overlays", array);
	if (!obs_data_save_json_safe(root, path.u8string().c_str(), "tmp", "bak")) {
		blog(LOG_WARNING, "[Himothee Overlays] Failed to save '%s'.", path.u8string().c_str());
		return false;
	}

	return true;
}

bool HimotheeOverlayManager::ReplaceOverlays(const vector<HimotheeOverlayDefinition> &updated)
{
	overlays = updated;
	return Save();
}

string HimotheeOverlayManager::MakeOverlayId() const
{
	for (size_t candidate = 1;; candidate++) {
		const string id = "overlay-" + to_string(candidate);
		if (none_of(overlays.begin(), overlays.end(), [&](const auto &overlay) { return overlay.id == id; })) {
			return id;
		}
	}
}

string HimotheeOverlayManager::AddOverlay()
{
	HimotheeOverlayDefinition overlay;
	overlay.id = MakeOverlayId();
	overlay.name = "Overlay " + to_string(overlays.size() + 1);
	overlay.title = overlay.name;

	obs_video_info videoInfo{};
	if (obs_get_video_info(&videoInfo)) {
		overlay.width = videoInfo.base_width;
		overlay.height = videoInfo.base_height;
	}

	const string id = overlay.id;
	overlays.emplace_back(std::move(overlay));
	Save();
	return id;
}

bool HimotheeOverlayManager::RemoveOverlay(const string &id)
{
	auto it = find_if(overlays.begin(), overlays.end(), [&](const auto &overlay) { return overlay.id == id; });
	if (it == overlays.end()) {
		return false;
	}

	overlays.erase(it);
	return Save();
}

bool HimotheeOverlayManager::SetVisible(const string &id, bool visible)
{
	auto *overlay = Find(id);
	if (!overlay) {
		return false;
	}
	overlay->visible = visible;
	return Save();
}

bool HimotheeOverlayManager::UpdateOverlay(const HimotheeOverlayDefinition &definition)
{
	auto *overlay = Find(definition.id);
	if (!overlay) {
		return false;
	}
	*overlay = definition;
	return Save();
}

const HimotheeOverlayDefinition *HimotheeOverlayManager::Find(const string &id) const
{
	auto it = find_if(overlays.begin(), overlays.end(), [&](const auto &overlay) { return overlay.id == id; });
	return it != overlays.end() ? &*it : nullptr;
}

HimotheeOverlayDefinition *HimotheeOverlayManager::Find(const string &id)
{
	auto it = find_if(overlays.begin(), overlays.end(), [&](const auto &overlay) { return overlay.id == id; });
	return it != overlays.end() ? &*it : nullptr;
}

bool HimotheeOverlayManager::StartServer()
{
	if (!server) {
		serverError = "Overlay HTTP server is unavailable.";
		return false;
	}
	if (server->isListening()) {
		return true;
	}

	serverError.clear();
	if (!server->listen(QHostAddress::LocalHost, serverPort)) {
		serverError = server->errorString().toStdString();
		blog(LOG_WARNING, "[Himothee Overlays] Could not listen on 127.0.0.1:%u: %s", serverPort,
		     serverError.c_str());
		return false;
	}

	blog(LOG_INFO, "[Himothee Overlays] Browser-source server listening on 127.0.0.1:%u.", serverPort);
	return true;
}

bool HimotheeOverlayManager::ServerRunning() const
{
	return server && server->isListening();
}

QString HimotheeOverlayManager::BaseUrl() const
{
	return QStringLiteral("http://127.0.0.1:%1").arg(serverPort);
}

QString HimotheeOverlayManager::OverlayUrl(const string &id) const
{
	return QStringLiteral("%1/overlay/%2").arg(BaseUrl(), QString::fromStdString(id));
}

void HimotheeOverlayManager::AcceptConnections()
{
	while (server && server->hasPendingConnections()) {
		QTcpSocket *socket = server->nextPendingConnection();
		if (!socket) {
			continue;
		}

		QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
			QByteArray buffer = socket->property("himotheeHttpBuffer").toByteArray();
			buffer += socket->readAll();
			socket->setProperty("himotheeHttpBuffer", buffer);
			if (!buffer.contains("\r\n\r\n")) {
				return;
			}
			HandleRequest(socket, buffer);
		});
		QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
	}
}

void HimotheeOverlayManager::HandleRequest(QTcpSocket *socket, const QByteArray &request)
{
	const int firstLineEnd = request.indexOf("\r\n");
	if (firstLineEnd <= 0) {
		WriteResponse(socket, 400, "text/plain; charset=utf-8", "Bad request");
		return;
	}

	const QList<QByteArray> parts = request.left(firstLineEnd).split(' ');
	if (parts.size() < 2 || parts[0] != "GET") {
		WriteResponse(socket, 405, "text/plain; charset=utf-8", "Method not allowed");
		return;
	}

	const QUrl url = QUrl::fromEncoded(parts[1]);
	const QString path = url.path();

	if (path == QStringLiteral("/health")) {
		QJsonObject health;
		health.insert(QStringLiteral("ok"), true);
		health.insert(QStringLiteral("service"), QStringLiteral("Himothee Overlays"));
		health.insert(QStringLiteral("port"), static_cast<int>(serverPort));
		WriteResponse(socket, 200, "application/json; charset=utf-8",
			      QJsonDocument(health).toJson(QJsonDocument::Compact));
		return;
	}

	const QString overlayPrefix = QStringLiteral("/overlay/");
	const QString apiPrefix = QStringLiteral("/api/overlay/");
	if (path.startsWith(overlayPrefix)) {
		const string id = path.mid(overlayPrefix.size()).toStdString();
		const auto *overlay = Find(id);
		if (!overlay) {
			WriteResponse(socket, 404, "text/plain; charset=utf-8", "Overlay not found");
			return;
		}
		WriteResponse(socket, 200, "text/html; charset=utf-8", BuildOverlayHtml(*overlay));
		return;
	}

	if (path.startsWith(apiPrefix)) {
		const string id = path.mid(apiPrefix.size()).toStdString();
		const auto *overlay = Find(id);
		if (!overlay) {
			WriteResponse(socket, 404, "application/json; charset=utf-8", "{\"error\":\"Overlay not found\"}");
			return;
		}
		WriteResponse(socket, 200, "application/json; charset=utf-8", BuildOverlayJson(*overlay));
		return;
	}

	WriteResponse(socket, 404, "text/plain; charset=utf-8", "Not found");
}

QByteArray HimotheeOverlayManager::BuildOverlayHtml(const HimotheeOverlayDefinition &overlay) const
{
	const QString apiPath = QStringLiteral("/api/overlay/%1").arg(QString::fromStdString(overlay.id));
	const QString html = QStringLiteral(R"HTML(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
html,body{width:100%%;height:100%%;margin:0;background:transparent;overflow:hidden;font-family:Inter,Segoe UI,Arial,sans-serif}
#stage{width:100%%;height:100%%;display:flex;align-items:center;justify-content:center}
#card{min-width:320px;max-width:88%%;padding:22px 30px;border:1px solid rgba(255,255,255,.18);border-radius:18px;background:rgba(10,12,18,.80);box-shadow:0 18px 55px rgba(0,0,0,.32);color:white;text-align:center}
#title{font-size:22px;font-weight:700;letter-spacing:.04em;text-transform:uppercase;opacity:.78;margin-bottom:8px}
#text{font-size:56px;font-weight:800;line-height:1.05;word-break:break-word}
.hidden{display:none!important}
</style>
</head>
<body>
<div id="stage"><div id="card"><div id="title"></div><div id="text"></div></div></div>
<script>
const endpoint='%1';
async function refreshOverlay(){
  try{
    const response=await fetch(endpoint,{cache:'no-store'});
    if(!response.ok)return;
    const data=await response.json();
    document.getElementById('card').classList.toggle('hidden',!data.visible);
    document.getElementById('title').textContent=data.title||data.name||'';
    document.getElementById('text').textContent=data.text||'';
  }catch(e){}
}
refreshOverlay();
setInterval(refreshOverlay,250);
</script>
</body>
</html>)HTML")
				.arg(apiPath);
	return html.toUtf8();
}

QByteArray HimotheeOverlayManager::BuildOverlayJson(const HimotheeOverlayDefinition &overlay) const
{
	QJsonObject object;
	object.insert(QStringLiteral("id"), QString::fromStdString(overlay.id));
	object.insert(QStringLiteral("name"), QString::fromStdString(overlay.name));
	object.insert(QStringLiteral("type"), QString::fromUtf8(OverlayTypeName(overlay.type)));
	object.insert(QStringLiteral("visible"), overlay.visible);
	object.insert(QStringLiteral("width"), static_cast<int>(overlay.width));
	object.insert(QStringLiteral("height"), static_cast<int>(overlay.height));
	object.insert(QStringLiteral("title"), QString::fromStdString(overlay.title));
	object.insert(QStringLiteral("text"), QString::fromStdString(overlay.text));
	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

void HimotheeOverlayManager::WriteResponse(QTcpSocket *socket, int statusCode, const QByteArray &contentType,
					    const QByteArray &body) const
{
	if (!socket) {
		return;
	}

	QByteArray reason = "OK";
	if (statusCode == 400) {
		reason = "Bad Request";
	} else if (statusCode == 404) {
		reason = "Not Found";
	} else if (statusCode == 405) {
		reason = "Method Not Allowed";
	}

	QByteArray headers = "HTTP/1.1 " + QByteArray::number(statusCode) + " " + reason + "\r\n";
	headers += "Content-Type: " + contentType + "\r\n";
	headers += "Cache-Control: no-store, no-cache, must-revalidate\r\n";
	headers += "Access-Control-Allow-Origin: *\r\n";
	headers += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
	headers += "Connection: close\r\n\r\n";

	socket->write(headers);
	socket->write(body);
	socket->disconnectFromHost();
}
