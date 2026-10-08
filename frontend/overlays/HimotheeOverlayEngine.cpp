#include "HimotheeOverlayEngine.hpp"

#include <widgets/OBSBasic.hpp>

#include <obs.hpp>

#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <filesystem>

using namespace std;

namespace {

constexpr const char *kOverlayFileName = "overlays.json";
constexpr uint16_t kOverlayServerPort = 3293;

int64_t NowMs()
{
	return QDateTime::currentMSecsSinceEpoch();
}

QString FormatDuration(int64_t milliseconds)
{
	milliseconds = max<int64_t>(0, milliseconds);
	const int64_t totalSeconds = milliseconds / 1000;
	const int64_t hours = totalSeconds / 3600;
	const int64_t minutes = (totalSeconds % 3600) / 60;
	const int64_t seconds = totalSeconds % 60;

	if (hours > 0) {
		return QStringLiteral("%1:%2:%3")
			.arg(hours, 2, 10, QLatin1Char('0'))
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(seconds, 2, 10, QLatin1Char('0'));
	}

	return QStringLiteral("%1:%2")
		.arg(minutes, 2, 10, QLatin1Char('0'))
		.arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace

const char *HimotheeOverlayTypeId(HimotheeOverlayType type)
{
	switch (type) {
	case HimotheeOverlayType::Counter:
		return "counter";
	case HimotheeOverlayType::KillCounter:
		return "kill_counter";
	case HimotheeOverlayType::StreakCounter:
		return "streak_counter";
	case HimotheeOverlayType::Progress:
		return "progress";
	case HimotheeOverlayType::Countdown:
		return "countdown";
	case HimotheeOverlayType::Stopwatch:
		return "stopwatch";
	case HimotheeOverlayType::StreamUptime:
		return "stream_uptime";
	case HimotheeOverlayType::Text:
	default:
		return "text";
	}
}

QString HimotheeOverlayTypeName(HimotheeOverlayType type)
{
	switch (type) {
	case HimotheeOverlayType::Counter:
		return QStringLiteral("Counter");
	case HimotheeOverlayType::KillCounter:
		return QStringLiteral("Kill Counter");
	case HimotheeOverlayType::StreakCounter:
		return QStringLiteral("Streak Counter");
	case HimotheeOverlayType::Progress:
		return QStringLiteral("Challenge Progress");
	case HimotheeOverlayType::Countdown:
		return QStringLiteral("Countdown");
	case HimotheeOverlayType::Stopwatch:
		return QStringLiteral("Stopwatch");
	case HimotheeOverlayType::StreamUptime:
		return QStringLiteral("Stream Uptime");
	case HimotheeOverlayType::Text:
	default:
		return QStringLiteral("Text");
	}
}

HimotheeOverlayType HimotheeOverlayTypeFromId(const char *type)
{
	if (!type) {
		return HimotheeOverlayType::Text;
	}
	if (astrcmpi(type, "counter") == 0) {
		return HimotheeOverlayType::Counter;
	}
	if (astrcmpi(type, "kill_counter") == 0) {
		return HimotheeOverlayType::KillCounter;
	}
	if (astrcmpi(type, "streak_counter") == 0) {
		return HimotheeOverlayType::StreakCounter;
	}
	if (astrcmpi(type, "progress") == 0) {
		return HimotheeOverlayType::Progress;
	}
	if (astrcmpi(type, "countdown") == 0) {
		return HimotheeOverlayType::Countdown;
	}
	if (astrcmpi(type, "stopwatch") == 0) {
		return HimotheeOverlayType::Stopwatch;
	}
	if (astrcmpi(type, "stream_uptime") == 0) {
		return HimotheeOverlayType::StreamUptime;
	}
	return HimotheeOverlayType::Text;
}

bool HimotheeOverlayIsCounter(HimotheeOverlayType type)
{
	return type == HimotheeOverlayType::Counter || type == HimotheeOverlayType::KillCounter ||
	       type == HimotheeOverlayType::StreakCounter || type == HimotheeOverlayType::Progress;
}

bool HimotheeOverlayIsTimer(HimotheeOverlayType type)
{
	return type == HimotheeOverlayType::Countdown || type == HimotheeOverlayType::Stopwatch;
}

HimotheeOverlayManager::HimotheeOverlayManager(OBSBasic *main_) : main(main_), serverPort(kOverlayServerPort)
{
	server = make_unique<QTcpServer>();
	QObject::connect(server.get(), &QTcpServer::newConnection, server.get(), [this]() { AcceptConnections(); });

	runtimeTimer = make_unique<QTimer>();
	runtimeTimer->setInterval(250);
	QObject::connect(runtimeTimer.get(), &QTimer::timeout, runtimeTimer.get(), [this]() { UpdateRuntimeState(); });
	runtimeTimer->start();

	StartServer();
	LoadForCurrentProfile();
	UpdateRuntimeState();
}

HimotheeOverlayManager::~HimotheeOverlayManager()
{
	if (runtimeTimer) {
		runtimeTimer->stop();
	}
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
		overlay.type = HimotheeOverlayTypeFromId(obs_data_get_string(item, "type"));
		overlay.visible = obs_data_has_user_value(item, "visible") ? obs_data_get_bool(item, "visible") : true;
		overlay.width = static_cast<uint32_t>(obs_data_get_int(item, "width"));
		overlay.height = static_cast<uint32_t>(obs_data_get_int(item, "height"));
		overlay.title = obs_data_get_string(item, "title");
		overlay.text = obs_data_get_string(item, "text");
		overlay.value = obs_data_has_user_value(item, "value") ? obs_data_get_int(item, "value") : 0;
		overlay.target = obs_data_has_user_value(item, "target") ? obs_data_get_int(item, "target") : 100;
		overlay.durationMs =
			obs_data_has_user_value(item, "duration_ms") ? obs_data_get_int(item, "duration_ms") : 300000;
		overlay.elapsedMs =
			obs_data_has_user_value(item, "elapsed_ms") ? obs_data_get_int(item, "elapsed_ms") : 0;
		overlay.running = obs_data_has_user_value(item, "running") && obs_data_get_bool(item, "running");
		overlay.startedAtMs =
			obs_data_has_user_value(item, "started_at_ms") ? obs_data_get_int(item, "started_at_ms") : 0;

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
		if (overlay.target <= 0) {
			overlay.target = 100;
		}
		if (overlay.durationMs <= 0) {
			overlay.durationMs = 300000;
		}
		if (!HimotheeOverlayIsTimer(overlay.type)) {
			overlay.running = false;
			overlay.startedAtMs = 0;
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
		obs_data_set_string(item, "type", HimotheeOverlayTypeId(overlay.type));
		obs_data_set_bool(item, "visible", overlay.visible);
		obs_data_set_int(item, "width", overlay.width);
		obs_data_set_int(item, "height", overlay.height);
		obs_data_set_string(item, "title", overlay.title.c_str());
		obs_data_set_string(item, "text", overlay.text.c_str());
		obs_data_set_int(item, "value", overlay.value);
		obs_data_set_int(item, "target", overlay.target);
		obs_data_set_int(item, "duration_ms", overlay.durationMs);
		obs_data_set_int(item, "elapsed_ms", overlay.elapsedMs);
		obs_data_set_bool(item, "running", overlay.running);
		obs_data_set_int(item, "started_at_ms", overlay.startedAtMs);
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
	vector<HimotheeOverlayDefinition> merged = updated;

	for (auto &candidate : merged) {
		const auto *existing = Find(candidate.id);
		if (!existing || existing->type != candidate.type || !HimotheeOverlayIsTimer(candidate.type)) {
			continue;
		}

		candidate.running = existing->running;
		candidate.startedAtMs = existing->startedAtMs;
		candidate.elapsedMs = existing->elapsedMs;
	}

	overlays = std::move(merged);
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

string HimotheeOverlayManager::AddOverlay(HimotheeOverlayType type)
{
	HimotheeOverlayDefinition overlay;
	overlay.id = MakeOverlayId();
	overlay.type = type;

	switch (type) {
	case HimotheeOverlayType::Counter:
		overlay.name = "Counter";
		overlay.title = "Counter";
		break;
	case HimotheeOverlayType::KillCounter:
		overlay.name = "Kill Counter";
		overlay.title = "Kills";
		break;
	case HimotheeOverlayType::StreakCounter:
		overlay.name = "Streak Counter";
		overlay.title = "Streak";
		break;
	case HimotheeOverlayType::Progress:
		overlay.name = "Challenge Progress";
		overlay.title = "Challenge";
		overlay.target = 100;
		break;
	case HimotheeOverlayType::Countdown:
		overlay.name = "Countdown";
		overlay.title = "Countdown";
		overlay.durationMs = 300000;
		break;
	case HimotheeOverlayType::Stopwatch:
		overlay.name = "Stopwatch";
		overlay.title = "Stopwatch";
		break;
	case HimotheeOverlayType::StreamUptime:
		overlay.name = "Stream Uptime";
		overlay.title = "Stream Uptime";
		break;
	case HimotheeOverlayType::Text:
	default:
		overlay.name = "Text Overlay";
		overlay.title = "Himothee Overlay";
		overlay.text = "Ready";
		break;
	}

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

bool HimotheeOverlayManager::AdjustValue(const string &id, int64_t delta)
{
	auto *overlay = Find(id);
	if (!overlay || !HimotheeOverlayIsCounter(overlay->type)) {
		return false;
	}

	overlay->value += delta;
	if (overlay->type == HimotheeOverlayType::KillCounter || overlay->type == HimotheeOverlayType::StreakCounter ||
	    overlay->type == HimotheeOverlayType::Progress) {
		overlay->value = max<int64_t>(0, overlay->value);
	}

	return Save();
}

bool HimotheeOverlayManager::ResetValue(const string &id)
{
	auto *overlay = Find(id);
	if (!overlay || !HimotheeOverlayIsCounter(overlay->type)) {
		return false;
	}

	overlay->value = 0;
	return Save();
}

bool HimotheeOverlayManager::StartTimer(const string &id)
{
	auto *overlay = Find(id);
	if (!overlay || !HimotheeOverlayIsTimer(overlay->type)) {
		return false;
	}
	if (overlay->running) {
		return true;
	}

	if (overlay->type == HimotheeOverlayType::Countdown && overlay->elapsedMs >= overlay->durationMs) {
		overlay->elapsedMs = 0;
	}

	overlay->startedAtMs = NowMs();
	overlay->running = true;
	return Save();
}

bool HimotheeOverlayManager::PauseTimer(const string &id)
{
	auto *overlay = Find(id);
	if (!overlay || !HimotheeOverlayIsTimer(overlay->type)) {
		return false;
	}
	if (!overlay->running) {
		return true;
	}

	overlay->elapsedMs = CurrentElapsedMs(*overlay);
	if (overlay->type == HimotheeOverlayType::Countdown) {
		overlay->elapsedMs = min(overlay->elapsedMs, overlay->durationMs);
	}
	overlay->running = false;
	overlay->startedAtMs = 0;
	return Save();
}

bool HimotheeOverlayManager::ResetTimer(const string &id)
{
	auto *overlay = Find(id);
	if (!overlay || !HimotheeOverlayIsTimer(overlay->type)) {
		return false;
	}

	overlay->elapsedMs = 0;
	overlay->running = false;
	overlay->startedAtMs = 0;
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

int64_t HimotheeOverlayManager::CurrentElapsedMs(const HimotheeOverlayDefinition &overlay) const
{
	int64_t elapsed = max<int64_t>(0, overlay.elapsedMs);
	if (overlay.running && overlay.startedAtMs > 0) {
		elapsed += max<int64_t>(0, NowMs() - overlay.startedAtMs);
	}
	return elapsed;
}

int64_t HimotheeOverlayManager::CurrentTimerElapsedMs(const string &id) const
{
	const auto *overlay = Find(id);
	return overlay ? CurrentElapsedMs(*overlay) : 0;
}

int64_t HimotheeOverlayManager::StreamUptimeMs() const
{
	if (!streamWasActive || streamStartedAtMs <= 0) {
		return 0;
	}
	return max<int64_t>(0, NowMs() - streamStartedAtMs);
}

QString HimotheeOverlayManager::RuntimeDisplay(const HimotheeOverlayDefinition &overlay) const
{
	switch (overlay.type) {
	case HimotheeOverlayType::Counter:
	case HimotheeOverlayType::KillCounter:
	case HimotheeOverlayType::StreakCounter:
		return QString::number(overlay.value);
	case HimotheeOverlayType::Progress:
		return QStringLiteral("%1 / %2").arg(overlay.value).arg(max<int64_t>(1, overlay.target));
	case HimotheeOverlayType::Countdown: {
		const int64_t remaining = max<int64_t>(0, overlay.durationMs - CurrentElapsedMs(overlay));
		return FormatDuration(remaining);
	}
	case HimotheeOverlayType::Stopwatch:
		return FormatDuration(CurrentElapsedMs(overlay));
	case HimotheeOverlayType::StreamUptime:
		return FormatDuration(StreamUptimeMs());
	case HimotheeOverlayType::Text:
	default:
		return QString::fromStdString(overlay.text);
	}
}

QString HimotheeOverlayManager::RuntimeDisplay(const string &id) const
{
	const auto *overlay = Find(id);
	return overlay ? RuntimeDisplay(*overlay) : QStringLiteral("-");
}

void HimotheeOverlayManager::UpdateRuntimeState()
{
	const bool streaming = main && main->StreamingActive();
	if (streaming && !streamWasActive) {
		streamStartedAtMs = NowMs();
	} else if (!streaming && streamWasActive) {
		streamStartedAtMs = 0;
	}
	streamWasActive = streaming;

	bool changed = false;
	for (auto &overlay : overlays) {
		if (overlay.type != HimotheeOverlayType::Countdown || !overlay.running) {
			continue;
		}
		if (CurrentElapsedMs(overlay) >= overlay.durationMs) {
			overlay.elapsedMs = overlay.durationMs;
			overlay.startedAtMs = 0;
			overlay.running = false;
			changed = true;
		}
	}

	if (changed) {
		Save();
	}
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
html,body{width:100%;height:100%;margin:0;background:transparent;overflow:hidden;font-family:Inter,Segoe UI,Arial,sans-serif}
#stage{width:100%;height:100%;display:flex;align-items:center;justify-content:center}
#card{min-width:320px;max-width:88%;padding:22px 30px;border:1px solid rgba(255,255,255,.18);border-radius:18px;background:rgba(10,12,18,.80);box-shadow:0 18px 55px rgba(0,0,0,.32);color:white;text-align:center}
#title{font-size:22px;font-weight:700;letter-spacing:.04em;text-transform:uppercase;opacity:.78;margin-bottom:8px}
#value{font-size:64px;font-weight:800;line-height:1.05;word-break:break-word}
#meta{font-size:15px;opacity:.62;margin-top:8px}
#progressWrap{height:14px;border-radius:999px;background:rgba(255,255,255,.14);overflow:hidden;margin-top:18px;display:none}
#progressBar{height:100%;width:0%;background:white;transition:width .2s ease}
.hidden{display:none!important}
</style>
</head>
<body>
<div id="stage">
  <div id="card">
    <div id="title"></div>
    <div id="value"></div>
    <div id="meta"></div>
    <div id="progressWrap"><div id="progressBar"></div></div>
  </div>
</div>
<script>
const endpoint='%1';
async function refreshOverlay(){
  try{
    const response=await fetch(endpoint,{cache:'no-store'});
    if(!response.ok)return;
    const data=await response.json();
    const card=document.getElementById('card');
    card.classList.toggle('hidden',!data.visible);
    document.getElementById('title').textContent=data.title||data.name||'';
    document.getElementById('value').textContent=data.display||'';
    document.getElementById('meta').textContent=data.meta||'';
    const progress=document.getElementById('progressWrap');
    const isProgress=data.type==='progress';
    progress.style.display=isProgress?'block':'none';
    if(isProgress) document.getElementById('progressBar').style.width=(data.progress||0)+'%';
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
	object.insert(QStringLiteral("type"), QString::fromUtf8(HimotheeOverlayTypeId(overlay.type)));
	object.insert(QStringLiteral("visible"), overlay.visible);
	object.insert(QStringLiteral("width"), static_cast<int>(overlay.width));
	object.insert(QStringLiteral("height"), static_cast<int>(overlay.height));
	object.insert(QStringLiteral("title"), QString::fromStdString(overlay.title));
	object.insert(QStringLiteral("text"), QString::fromStdString(overlay.text));
	object.insert(QStringLiteral("value"), static_cast<double>(overlay.value));
	object.insert(QStringLiteral("target"), static_cast<double>(overlay.target));
	object.insert(QStringLiteral("running"), overlay.running);
	object.insert(QStringLiteral("display"), RuntimeDisplay(overlay));

	QString meta;
	int progress = 0;
	int64_t runtimeMs = 0;

	if (overlay.type == HimotheeOverlayType::Progress) {
		const int64_t target = max<int64_t>(1, overlay.target);
		progress = static_cast<int>(clamp<int64_t>((overlay.value * 100) / target, 0, 100));
		meta = QStringLiteral("%1% complete").arg(progress);
	} else if (overlay.type == HimotheeOverlayType::Countdown) {
		runtimeMs = max<int64_t>(0, overlay.durationMs - CurrentElapsedMs(overlay));
		meta = overlay.running ? QStringLiteral("LIVE") : QStringLiteral("PAUSED");
	} else if (overlay.type == HimotheeOverlayType::Stopwatch) {
		runtimeMs = CurrentElapsedMs(overlay);
		meta = overlay.running ? QStringLiteral("RUNNING") : QStringLiteral("PAUSED");
	} else if (overlay.type == HimotheeOverlayType::StreamUptime) {
		runtimeMs = StreamUptimeMs();
		meta = streamWasActive ? QStringLiteral("LIVE") : QStringLiteral("OFFLINE");
	}

	object.insert(QStringLiteral("meta"), meta);
	object.insert(QStringLiteral("progress"), progress);
	object.insert(QStringLiteral("runtime_ms"), static_cast<double>(runtimeMs));
	object.insert(QStringLiteral("stream_active"), streamWasActive);
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

	QByteArray headers = "HTTP/1.1 ";
	headers += QByteArray::number(statusCode);
	headers += " ";
	headers += reason;
	headers += "\r\n";
	headers += "Content-Type: ";
	headers += contentType;
	headers += "\r\n";
	headers += "Cache-Control: no-store, no-cache, must-revalidate\r\n";
	headers += "Access-Control-Allow-Origin: *\r\n";
	headers += "Content-Length: ";
	headers += QByteArray::number(body.size());
	headers += "\r\n";
	headers += "Connection: close\r\n\r\n";

	socket->write(headers);
	socket->write(body);
	socket->disconnectFromHost();
}
