#pragma once

#include <QString>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class OBSBasic;
class QTcpServer;
class QTcpSocket;

enum class HimotheeOverlayType {
	Text,
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
};

class HimotheeOverlayManager {
public:
	explicit HimotheeOverlayManager(OBSBasic *main);
	~HimotheeOverlayManager();

	bool LoadForCurrentProfile();
	bool Load();
	bool Save() const;

	const std::vector<HimotheeOverlayDefinition> &Overlays() const noexcept { return overlays; }
	bool ReplaceOverlays(const std::vector<HimotheeOverlayDefinition> &updated);

	std::string AddOverlay();
	bool RemoveOverlay(const std::string &id);
	bool SetVisible(const std::string &id, bool visible);
	bool UpdateOverlay(const HimotheeOverlayDefinition &definition);

	const HimotheeOverlayDefinition *Find(const std::string &id) const;
	HimotheeOverlayDefinition *Find(const std::string &id);

	bool StartServer();
	bool ServerRunning() const;
	uint16_t ServerPort() const noexcept { return serverPort; }
	const std::string &ServerError() const noexcept { return serverError; }

	QString OverlayUrl(const std::string &id) const;
	QString BaseUrl() const;

private:
	OBSBasic *main = nullptr;
	std::vector<HimotheeOverlayDefinition> overlays;
	std::string loadedProfilePath;

	std::unique_ptr<QTcpServer> server;
	uint16_t serverPort = 3293;
	std::string serverError;

	std::string MakeOverlayId() const;
	void AcceptConnections();
	void HandleRequest(QTcpSocket *socket, const QByteArray &request);
	QByteArray BuildOverlayHtml(const HimotheeOverlayDefinition &overlay) const;
	QByteArray BuildOverlayJson(const HimotheeOverlayDefinition &overlay) const;
	void WriteResponse(QTcpSocket *socket, int statusCode, const QByteArray &contentType, const QByteArray &body) const;
};
