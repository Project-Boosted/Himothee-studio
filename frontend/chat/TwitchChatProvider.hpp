#pragma once

#include "HimotheeChat.hpp"

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>

#include <deque>
#include <string>
#include <unordered_set>

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
#include <QWebSocket>
#endif

class QNetworkReply;

class HimotheeTwitchChatProvider : public QObject, public HimotheeChatProvider {
public:
	explicit HimotheeTwitchChatProvider(HimotheeChatManager *manager);
	~HimotheeTwitchChatProvider() override;

	HimotheeChatPlatform Platform() const noexcept override { return HimotheeChatPlatform::Twitch; }
	HimotheeChatConnectionState State() const noexcept override { return state; }
	std::string AccountName() const override { return accountLogin; }
	std::string ChannelName() const override;
	bool AutomaticChannel() const override { return channelOverride.empty(); }
	void SetChannelOverride(const std::string &channel) override;

	bool Connect() override;
	void Disconnect() override;
	bool SendMessage(const std::string &message) override;

private:
	HimotheeChatManager *manager = nullptr;
	HimotheeChatConnectionState state = HimotheeChatConnectionState::Disconnected;

	QNetworkAccessManager network;
	QTimer reconnectTimer;

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
	QWebSocket socket;
#endif

	std::string accessToken;
	std::string clientId;
	std::string authenticatedUserId;
	std::string accountLogin;
	std::string broadcasterUserId;
	std::string broadcasterLogin;
	std::string channelOverride;
	std::string websocketSessionId;

	bool userInitiatedDisconnect = false;
	bool reconnectSession = false;
	QString pendingReconnectUrl;

	std::deque<std::string> recentEnvelopeIds;
	std::unordered_set<std::string> recentEnvelopeIdSet;

	void LoadChannelOverride();
	void UpdateStatus(HimotheeChatConnectionState newState, const std::string &error = {});
	void PushSystemMessage(const std::string &message);

	void ValidateToken();
	void HandleValidationReply(QNetworkReply *reply);
	void ResolveTargetChannel();
	void HandleChannelLookupReply(QNetworkReply *reply);

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
	void OpenWebSocket(const QUrl &url, bool isReconnect);
	void HandleSocketConnected();
	void HandleSocketDisconnected();
	void HandleSocketMessage(const QString &message);
	void HandleWelcome(const QJsonObject &payload);
	void CreateChatSubscription();
	void HandleSubscriptionReply(QNetworkReply *reply);
	void HandleChatNotification(const QJsonObject &root);
	void RememberEnvelopeId(const std::string &id);
#endif
};
