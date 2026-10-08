#pragma once

#include <QString>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class OBSBasic;
class QByteArray;
class QTcpServer;
class QTcpSocket;
class QTimer;

enum class HimotheeOverlayType {
	Text,
	Counter,
	KillCounter,
	StreakCounter,
	Progress,
	Countdown,
	Stopwatch,
	StreamUptime,
	Darts180,
	Darts140Plus,
	Darts100Plus,
	DartsLegs,
	DartsWins,
	DartsAverage,
	DartsCheckout,
	GamingKDA,
	GamingWinsLosses,
	GamingRound,
	GamingAttempts,
	GamingDeaths,
	GamingPersonalBest,
	GamingSessionStats,
};

enum class HimotheeOverlayPosition {
	TopLeft,
	TopCenter,
	TopRight,
	MiddleLeft,
	Center,
	MiddleRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
};

enum class HimotheeOverlayAnimation {
	None,
	Fade,
	Pop,
	SlideUp,
	SlideLeft,
};

struct HimotheeOverlayDefinition {
	std::string id;
	std::string name;
	HimotheeOverlayType type = HimotheeOverlayType::Text;
	bool visible = true;
	uint32_t width = 1920;
	uint32_t height = 1080;
	std::string title = "Himothee Overlay";
	std::string text = "Ready";

	int64_t value = 0;
	int64_t target = 100;
	int64_t durationMs = 300000;
	int64_t elapsedMs = 0;
	bool running = false;
	int64_t startedAtMs = 0;

	std::string theme = "himothee_dark";
	std::string fontFamily = "Segoe UI";
	uint32_t fontSize = 64;
	std::string textColor = "#FFFFFF";
	std::string backgroundColor = "#0A0C12";
	uint32_t backgroundOpacity = 80;
	uint32_t cornerRadius = 18;
	HimotheeOverlayPosition position = HimotheeOverlayPosition::Center;
	HimotheeOverlayAnimation animation = HimotheeOverlayAnimation::Pop;

	std::string mediaPath;
	uint32_t mediaOpacity = 100;
	bool mediaLoop = true;

	double decimalValue = 0.0;
	int64_t checkoutScore = 0;
	std::string checkoutRoute;
	int64_t notificationDurationMs = 3000;
	int64_t notificationUntilMs = 0;

	int64_t gamingKills = 0;
	int64_t gamingDeaths = 0;
	int64_t gamingAssists = 0;
	int64_t gamingWins = 0;
	int64_t gamingLosses = 0;
	std::string personalBest = "PB";
};

class HimotheeOverlayManager {
public:
	explicit HimotheeOverlayManager(OBSBasic *main);
	~HimotheeOverlayManager();

	bool LoadForCurrentProfile();
	bool Load();
	bool Save() const;

	const std::vector<HimotheeOverlayDefinition> &Overlays() const noexcept { return overlays; }
	const std::string &LoadedProfilePath() const noexcept { return loadedProfilePath; }
	bool ReplaceOverlays(const std::vector<HimotheeOverlayDefinition> &updated);

	std::string AddOverlay(HimotheeOverlayType type = HimotheeOverlayType::Text);
	bool RemoveOverlay(const std::string &id);
	bool SetVisible(const std::string &id, bool visible);
	bool UpdateOverlay(const HimotheeOverlayDefinition &definition);

	bool AdjustValue(const std::string &id, int64_t delta);
	bool ResetValue(const std::string &id);
	bool StartTimer(const std::string &id);
	bool PauseTimer(const std::string &id);
	bool ResetTimer(const std::string &id);

	bool SetDecimalValue(const std::string &id, double value);
	bool TriggerCheckout(const std::string &id, int64_t score, const std::string &route,
			    int64_t durationMs = 3000);
	bool ClearCheckout(const std::string &id);

	bool AdjustGamingStat(const std::string &id, const std::string &stat, int64_t delta);
	bool ResetGamingStats(const std::string &id);
	bool SetPersonalBest(const std::string &id, const std::string &value);

	const HimotheeOverlayDefinition *Find(const std::string &id) const;
	HimotheeOverlayDefinition *Find(const std::string &id);

	QString RuntimeDisplay(const std::string &id) const;
	int64_t CurrentTimerElapsedMs(const std::string &id) const;
	int64_t StreamUptimeMs() const;

	bool StartServer();
	bool ServerRunning() const;
	uint16_t ServerPort() const noexcept { return serverPort; }
	const std::string &ServerError() const noexcept { return serverError; }

	QString OverlayUrl(const std::string &id) const;
	QString MediaUrl(const std::string &id) const;
	QString BaseUrl() const;

private:
	OBSBasic *main = nullptr;
	std::vector<HimotheeOverlayDefinition> overlays;
	std::string loadedProfilePath;

	std::unique_ptr<QTcpServer> server;
	std::unique_ptr<QTimer> runtimeTimer;
	uint16_t serverPort = 3293;
	std::string serverError;

	bool streamWasActive = false;
	int64_t streamStartedAtMs = 0;

	std::string MakeOverlayId() const;
	void UpdateRuntimeState();
	int64_t CurrentElapsedMs(const HimotheeOverlayDefinition &overlay) const;
	QString RuntimeDisplay(const HimotheeOverlayDefinition &overlay) const;

	void AcceptConnections();
	void HandleRequest(QTcpSocket *socket, const QByteArray &request);
	QByteArray BuildOverlayHtml(const HimotheeOverlayDefinition &overlay) const;
	QByteArray BuildOverlayJson(const HimotheeOverlayDefinition &overlay) const;
	void WriteResponse(QTcpSocket *socket, int statusCode, const QByteArray &contentType, const QByteArray &body) const;
};

const char *HimotheeOverlayTypeId(HimotheeOverlayType type);
QString HimotheeOverlayTypeName(HimotheeOverlayType type);
HimotheeOverlayType HimotheeOverlayTypeFromId(const char *type);
bool HimotheeOverlayIsCounter(HimotheeOverlayType type);
bool HimotheeOverlayIsTimer(HimotheeOverlayType type);
bool HimotheeOverlayIsDarts(HimotheeOverlayType type);
bool HimotheeOverlayIsGaming(HimotheeOverlayType type);

const char *HimotheeOverlayPositionId(HimotheeOverlayPosition position);
QString HimotheeOverlayPositionName(HimotheeOverlayPosition position);
HimotheeOverlayPosition HimotheeOverlayPositionFromId(const char *position);

const char *HimotheeOverlayAnimationId(HimotheeOverlayAnimation animation);
QString HimotheeOverlayAnimationName(HimotheeOverlayAnimation animation);
HimotheeOverlayAnimation HimotheeOverlayAnimationFromId(const char *animation);
