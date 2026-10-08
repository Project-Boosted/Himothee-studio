#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <string>
#include <vector>

class OBSBasic;
class HimotheeOverlayManager;

struct HimotheeActionDescriptor {
	QString id;
	QString name;
	QString category;
	QString description;
	QJsonObject parameters;
};

struct HimotheeActionResult {
	bool success = false;
	QString code;
	QString message;
	QJsonObject data;

	QJsonObject ToJson() const;
};

class HimotheeActionRegistry {
public:
	explicit HimotheeActionRegistry(OBSBasic *main);

	const std::vector<HimotheeActionDescriptor> &Actions() const noexcept { return actions; }
	QJsonArray ActionsJson() const;
	QJsonObject StateSnapshot() const;

	HimotheeActionResult Execute(const QString &actionId, const QJsonObject &params = {});

private:
	OBSBasic *main = nullptr;
	std::vector<HimotheeActionDescriptor> actions;

	void RegisterBuiltInActions();
	HimotheeOverlayManager *OverlayManager() const;

	HimotheeActionResult ExecuteStream(const QString &actionId);
	HimotheeActionResult ExecuteRecording(const QString &actionId);
	HimotheeActionResult ExecuteReplayBuffer(const QString &actionId);
	HimotheeActionResult ExecuteScene(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteAudio(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteDestination(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteOverlay(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteCounter(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteTimer(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteDarts(const QString &actionId, const QJsonObject &params);
	HimotheeActionResult ExecuteGaming(const QString &actionId, const QJsonObject &params);

	static HimotheeActionResult Ok(const QString &message, const QJsonObject &data = {});
	static HimotheeActionResult Fail(const QString &code, const QString &message);
	static QString RequiredString(const QJsonObject &params, const QString &key);
};
