#include "TwitchChatProvider.hpp"

#include <widgets/OBSBasic.hpp>

#ifdef TWITCH_ENABLED
#include <oauth/TwitchAuth.hpp>
#endif

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <util/config-file.h>

using namespace std;

namespace {

constexpr auto TwitchValidateUrl = "https://id.twitch.tv/oauth2/validate";
constexpr auto TwitchUsersUrl = "https://api.twitch.tv/helix/users";
constexpr auto TwitchEventSubUrl = "https://api.twitch.tv/helix/eventsub/subscriptions";
constexpr auto TwitchEventSubWebSocketUrl = "wss://eventsub.wss.twitch.tv/ws?keepalive_timeout_seconds=30";

bool HasScope(const QJsonArray &scopes, const QString &wanted)
{
	for (const auto &scope : scopes) {
		if (scope.toString() == wanted) {
			return true;
		}
	}
	return false;
}

} // namespace

HimotheeTwitchChatProvider::HimotheeTwitchChatProvider(HimotheeChatManager *manager_)
	: QObject(nullptr),
	  manager(manager_)
{
	reconnectTimer.setSingleShot(true);
	reconnectTimer.setInterval(3000);
	connect(&reconnectTimer, &QTimer::timeout, this, [this]() {
		if (!userInitiatedDisconnect) {
			ValidateToken();
		}
	});

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
	connect(&socket, &QWebSocket::connected, this, [this]() { HandleSocketConnected(); });
	connect(&socket, &QWebSocket::disconnected, this, [this]() { HandleSocketDisconnected(); });
	connect(&socket, &QWebSocket::textMessageReceived, this,
		[this](const QString &message) { HandleSocketMessage(message); });
#endif
}

HimotheeTwitchChatProvider::~HimotheeTwitchChatProvider()
{
	Disconnect();
}

string HimotheeTwitchChatProvider::ChannelName() const
{
	if (!broadcasterLogin.empty()) {
		return broadcasterLogin;
	}
	if (!channelOverride.empty()) {
		return channelOverride;
	}
	return accountLogin;
}

void HimotheeTwitchChatProvider::LoadChannelOverride()
{
	OBSBasic *main = OBSBasic::Get();
	if (!main || !main->Config() || !channelOverride.empty()) {
		return;
	}

	const char *saved = config_get_string(main->Config(), "HimotheeChat", "TwitchChannelOverride");
	if (saved && *saved) {
		channelOverride = saved;
	}
}

void HimotheeTwitchChatProvider::SetChannelOverride(const string &channel)
{
	channelOverride = channel;
	broadcasterUserId.clear();
	broadcasterLogin.clear();

	OBSBasic *main = OBSBasic::Get();
	if (main && main->Config()) {
		config_set_string(main->Config(), "HimotheeChat", "TwitchChannelOverride", channelOverride.c_str());
	}

	UpdateStatus(state);
}

void HimotheeTwitchChatProvider::UpdateStatus(HimotheeChatConnectionState newState, const string &error)
{
	state = newState;
	if (!manager) {
		return;
	}

	HimotheeChatProviderStatus status;
	status.platform = HimotheeChatPlatform::Twitch;
	status.state = state;
	status.accountName = accountLogin;
	status.channelName = ChannelName();
	status.automaticChannel = channelOverride.empty();
	status.lastError = error;
	manager->SetProviderStatus(std::move(status));
}

void HimotheeTwitchChatProvider::PushSystemMessage(const string &message)
{
	if (!manager) {
		return;
	}

	HimotheeChatMessage item;
	item.platform = HimotheeChatPlatform::System;
	item.type = HimotheeChatMessageType::System;
	item.displayName = "Twitch";
	item.text = message;
	manager->PushMessage(std::move(item));
}

bool HimotheeTwitchChatProvider::Connect()
{
	userInitiatedDisconnect = false;
	reconnectTimer.stop();
	LoadChannelOverride();

#ifndef HIMOTHEE_HAS_QT_WEBSOCKETS
	UpdateStatus(HimotheeChatConnectionState::Error,
		     "This Himothee Studio build does not include Qt WebSockets support.");
	return false;
#elif !defined(TWITCH_ENABLED)
	UpdateStatus(HimotheeChatConnectionState::Error,
		     "Twitch API support is not enabled in this Himothee Studio build.");
	return false;
#else
	OBSBasic *main = OBSBasic::Get();
	if (!main) {
		UpdateStatus(HimotheeChatConnectionState::Error, "OBS main window is unavailable.");
		return false;
	}

	auto *auth = dynamic_cast<TwitchAuth *>(main->GetAuth());
	if (!auth) {
		UpdateStatus(HimotheeChatConnectionState::Error,
			     "Connect a Twitch account in Settings > Stream first, then press Connect Twitch again.");
		return false;
	}

	accessToken = auth->AccessToken();
	accountLogin = auth->AccountName();

	if (accessToken.empty()) {
		UpdateStatus(HimotheeChatConnectionState::Error,
			     "The connected Twitch account has no usable OAuth token. Reconnect Twitch in Settings > Stream.");
		return false;
	}

	UpdateStatus(HimotheeChatConnectionState::Connecting);
	ValidateToken();
	return true;
#endif
}

void HimotheeTwitchChatProvider::Disconnect()
{
	userInitiatedDisconnect = true;
	reconnectTimer.stop();
	websocketSessionId.clear();
	pendingReconnectUrl.clear();
	reconnectSession = false;

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
	if (socket.state() != QAbstractSocket::UnconnectedState) {
		socket.close();
	}
#endif

	UpdateStatus(HimotheeChatConnectionState::Disconnected);
}

bool HimotheeTwitchChatProvider::SendMessage(const string &)
{
	// Sending is intentionally added in Stage 9.5.
	return false;
}

void HimotheeTwitchChatProvider::ValidateToken()
{
	if (userInitiatedDisconnect || accessToken.empty()) {
		return;
	}

	QNetworkRequest request{QUrl(QString::fromUtf8(TwitchValidateUrl))};
	request.setRawHeader("Authorization", QByteArray("OAuth ") + QByteArray::fromStdString(accessToken));

	QNetworkReply *reply = network.get(request);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		HandleValidationReply(reply);
		reply->deleteLater();
	});
}

void HimotheeTwitchChatProvider::HandleValidationReply(QNetworkReply *reply)
{
	if (userInitiatedDisconnect) {
		return;
	}

	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
	const QJsonObject root = json.object();

	if (statusCode != 200 || root.isEmpty()) {
		UpdateStatus(HimotheeChatConnectionState::Error,
			     "Twitch OAuth validation failed. Reconnect the Twitch account in Settings > Stream.");
		return;
	}

	const QJsonArray scopes = root.value(QStringLiteral("scopes")).toArray();
	if (!HasScope(scopes, QStringLiteral("user:read:chat"))) {
		UpdateStatus(
			HimotheeChatConnectionState::Error,
			"The connected Twitch OAuth token does not include user:read:chat. Twitch chat needs a refreshed "
			"authorization that grants chat read access.");
		return;
	}

	clientId = root.value(QStringLiteral("client_id")).toString().toStdString();
	authenticatedUserId = root.value(QStringLiteral("user_id")).toString().toStdString();
	const string validatedLogin = root.value(QStringLiteral("login")).toString().toStdString();
	if (!validatedLogin.empty()) {
		accountLogin = validatedLogin;
	}

	if (clientId.empty() || authenticatedUserId.empty() || accountLogin.empty()) {
		UpdateStatus(HimotheeChatConnectionState::Error, "Twitch returned incomplete account identity data.");
		return;
	}

	ResolveTargetChannel();
}

void HimotheeTwitchChatProvider::ResolveTargetChannel()
{
	if (channelOverride.empty()) {
		broadcasterUserId = authenticatedUserId;
		broadcasterLogin = accountLogin;
#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
		OpenWebSocket(QUrl(QString::fromUtf8(TwitchEventSubWebSocketUrl)), false);
#endif
		return;
	}

	QUrl url(QString::fromUtf8(TwitchUsersUrl));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("login"), QString::fromStdString(channelOverride));
	url.setQuery(query);

	QNetworkRequest request{url};
	request.setRawHeader("Authorization", QByteArray("Bearer ") + QByteArray::fromStdString(accessToken));
	request.setRawHeader("Client-Id", QByteArray::fromStdString(clientId));

	QNetworkReply *reply = network.get(request);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		HandleChannelLookupReply(reply);
		reply->deleteLater();
	});
}

void HimotheeTwitchChatProvider::HandleChannelLookupReply(QNetworkReply *reply)
{
	if (userInitiatedDisconnect) {
		return;
	}

	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
	const QJsonArray data = root.value(QStringLiteral("data")).toArray();

	if (statusCode != 200 || data.isEmpty()) {
		UpdateStatus(HimotheeChatConnectionState::Error,
			     string("Could not find Twitch channel '") + channelOverride + "'.");
		return;
	}

	const QJsonObject user = data.first().toObject();
	broadcasterUserId = user.value(QStringLiteral("id")).toString().toStdString();
	broadcasterLogin = user.value(QStringLiteral("login")).toString().toStdString();

	if (broadcasterUserId.empty()) {
		UpdateStatus(HimotheeChatConnectionState::Error, "Twitch channel lookup returned no user ID.");
		return;
	}

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
	OpenWebSocket(QUrl(QString::fromUtf8(TwitchEventSubWebSocketUrl)), false);
#endif
}

#ifdef HIMOTHEE_HAS_QT_WEBSOCKETS
void HimotheeTwitchChatProvider::OpenWebSocket(const QUrl &url, bool isReconnect)
{
	if (userInitiatedDisconnect) {
		return;
	}

	reconnectSession = isReconnect;
	UpdateStatus(isReconnect ? HimotheeChatConnectionState::Reconnecting : HimotheeChatConnectionState::Connecting);
	socket.open(url);
}

void HimotheeTwitchChatProvider::HandleSocketConnected()
{
	// EventSub becomes usable after session_welcome supplies the session ID.
}

void HimotheeTwitchChatProvider::HandleSocketDisconnected()
{
	if (userInitiatedDisconnect) {
		UpdateStatus(HimotheeChatConnectionState::Disconnected);
		return;
	}

	if (!pendingReconnectUrl.isEmpty()) {
		const QUrl url = pendingReconnectUrl;
		pendingReconnectUrl.clear();
		OpenWebSocket(url, true);
		return;
	}

	UpdateStatus(HimotheeChatConnectionState::Reconnecting,
		     socket.errorString().isEmpty() ? "Twitch EventSub disconnected." : socket.errorString().toStdString());
	if (!reconnectTimer.isActive()) {
		reconnectTimer.start();
	}
}

void HimotheeTwitchChatProvider::HandleSocketMessage(const QString &message)
{
	const QJsonObject root = QJsonDocument::fromJson(message.toUtf8()).object();
	if (root.isEmpty()) {
		return;
	}

	const QJsonObject metadata = root.value(QStringLiteral("metadata")).toObject();
	const string envelopeId = metadata.value(QStringLiteral("message_id")).toString().toStdString();
	if (!envelopeId.empty()) {
		if (recentEnvelopeIdSet.contains(envelopeId)) {
			return;
		}
		RememberEnvelopeId(envelopeId);
	}

	const QString messageType = metadata.value(QStringLiteral("message_type")).toString();
	const QJsonObject payload = root.value(QStringLiteral("payload")).toObject();

	if (messageType == QStringLiteral("session_welcome")) {
		HandleWelcome(payload);
		return;
	}
	if (messageType == QStringLiteral("session_keepalive")) {
		return;
	}
	if (messageType == QStringLiteral("session_reconnect")) {
		const QString reconnectUrl =
			payload.value(QStringLiteral("session")).toObject().value(QStringLiteral("reconnect_url")).toString();
		if (!reconnectUrl.isEmpty()) {
			pendingReconnectUrl = reconnectUrl;
			UpdateStatus(HimotheeChatConnectionState::Reconnecting);
			socket.close();
		}
		return;
	}
	if (messageType == QStringLiteral("notification")) {
		HandleChatNotification(root);
		return;
	}
	if (messageType == QStringLiteral("revocation")) {
		UpdateStatus(HimotheeChatConnectionState::Error,
			     "Twitch revoked the chat EventSub subscription. Reconnect Twitch chat.");
		socket.close();
	}
}

void HimotheeTwitchChatProvider::HandleWelcome(const QJsonObject &payload)
{
	const QJsonObject session = payload.value(QStringLiteral("session")).toObject();
	websocketSessionId = session.value(QStringLiteral("id")).toString().toStdString();
	if (websocketSessionId.empty()) {
		UpdateStatus(HimotheeChatConnectionState::Error, "Twitch EventSub welcome message contained no session ID.");
		return;
	}

	if (reconnectSession) {
		reconnectSession = false;
		UpdateStatus(HimotheeChatConnectionState::Connected);
		return;
	}

	CreateChatSubscription();
}

void HimotheeTwitchChatProvider::CreateChatSubscription()
{
	QJsonObject condition;
	condition.insert(QStringLiteral("broadcaster_user_id"), QString::fromStdString(broadcasterUserId));
	condition.insert(QStringLiteral("user_id"), QString::fromStdString(authenticatedUserId));

	QJsonObject transport;
	transport.insert(QStringLiteral("method"), QStringLiteral("websocket"));
	transport.insert(QStringLiteral("session_id"), QString::fromStdString(websocketSessionId));

	QJsonObject body;
	body.insert(QStringLiteral("type"), QStringLiteral("channel.chat.message"));
	body.insert(QStringLiteral("version"), QStringLiteral("1"));
	body.insert(QStringLiteral("condition"), condition);
	body.insert(QStringLiteral("transport"), transport);

	QNetworkRequest request{QUrl(QString::fromUtf8(TwitchEventSubUrl))};
	request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
	request.setRawHeader("Authorization", QByteArray("Bearer ") + QByteArray::fromStdString(accessToken));
	request.setRawHeader("Client-Id", QByteArray::fromStdString(clientId));

	QNetworkReply *reply = network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		HandleSubscriptionReply(reply);
		reply->deleteLater();
	});
}

void HimotheeTwitchChatProvider::HandleSubscriptionReply(QNetworkReply *reply)
{
	if (userInitiatedDisconnect) {
		return;
	}

	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();

	if (statusCode != 202) {
		string message = root.value(QStringLiteral("message")).toString().toStdString();
		if (message.empty()) {
			message = "Twitch rejected the channel.chat.message EventSub subscription.";
		}
		UpdateStatus(HimotheeChatConnectionState::Error, message);
		return;
	}

	UpdateStatus(HimotheeChatConnectionState::Connected);
	PushSystemMessage(string("Connected to Twitch chat: #") + ChannelName());
}

void HimotheeTwitchChatProvider::HandleChatNotification(const QJsonObject &root)
{
	const QJsonObject payload = root.value(QStringLiteral("payload")).toObject();
	const QJsonObject subscription = payload.value(QStringLiteral("subscription")).toObject();
	if (subscription.value(QStringLiteral("type")).toString() != QStringLiteral("channel.chat.message")) {
		return;
	}

	const QJsonObject event = payload.value(QStringLiteral("event")).toObject();
	const QJsonObject messageObject = event.value(QStringLiteral("message")).toObject();

	HimotheeChatMessage message;
	message.platform = HimotheeChatPlatform::Twitch;
	message.type = HimotheeChatMessageType::Chat;
	message.messageId = event.value(QStringLiteral("message_id")).toString().toStdString();
	message.userId = event.value(QStringLiteral("chatter_user_id")).toString().toStdString();
	message.displayName = event.value(QStringLiteral("chatter_user_name")).toString().toStdString();
	message.text = messageObject.value(QStringLiteral("text")).toString().toStdString();

	const QJsonArray badges = event.value(QStringLiteral("badges")).toArray();
	for (const auto &badgeValue : badges) {
		const QString badge = badgeValue.toObject().value(QStringLiteral("set_id")).toString();
		message.broadcaster = message.broadcaster || badge == QStringLiteral("broadcaster");
		message.moderator = message.moderator || badge == QStringLiteral("moderator");
		message.subscriber = message.subscriber || badge == QStringLiteral("subscriber") ||
				     badge == QStringLiteral("founder");
	}

	if (manager && !message.text.empty()) {
		manager->PushMessage(std::move(message));
	}
}

void HimotheeTwitchChatProvider::RememberEnvelopeId(const string &id)
{
	recentEnvelopeIds.push_back(id);
	recentEnvelopeIdSet.insert(id);

	while (recentEnvelopeIds.size() > 256) {
		recentEnvelopeIdSet.erase(recentEnvelopeIds.front());
		recentEnvelopeIds.pop_front();
	}
}
#endif
