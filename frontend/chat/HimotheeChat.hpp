#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

enum class HimotheeChatPlatform {
	All,
	Twitch,
	YouTube,
	Kick,
	System,
};

enum class HimotheeChatMessageType {
	Chat,
	Subscription,
	Membership,
	Donation,
	System,
};

enum class HimotheeChatConnectionState {
	Disconnected,
	Connecting,
	Connected,
	Reconnecting,
	Error,
};

struct HimotheeChatMessage {
	uint64_t sequence = 0;
	HimotheeChatPlatform platform = HimotheeChatPlatform::System;
	HimotheeChatMessageType type = HimotheeChatMessageType::Chat;

	std::string messageId;
	std::string userId;
	std::string displayName;
	std::string text;

	bool broadcaster = false;
	bool moderator = false;
	bool subscriber = false;

	uint64_t timestampMs = 0;
};

struct HimotheeChatProviderStatus {
	HimotheeChatPlatform platform = HimotheeChatPlatform::System;
	HimotheeChatConnectionState state = HimotheeChatConnectionState::Disconnected;
	std::string accountName;
	std::string channelName;
	bool automaticChannel = true;
	std::string lastError;
};

class HimotheeChatProvider {
public:
	virtual ~HimotheeChatProvider() = default;

	virtual HimotheeChatPlatform Platform() const noexcept = 0;
	virtual HimotheeChatConnectionState State() const noexcept = 0;
	virtual std::string AccountName() const = 0;
	virtual std::string ChannelName() const { return {}; }
	virtual bool AutomaticChannel() const { return true; }
	virtual void SetChannelOverride(const std::string &) {}

	virtual bool Connect() = 0;
	virtual void Disconnect() = 0;
	virtual bool SendMessage(const std::string &message) = 0;
};

class HimotheeChatManager {
public:
	HimotheeChatManager();
	~HimotheeChatManager();

	HimotheeChatManager(const HimotheeChatManager &) = delete;
	HimotheeChatManager &operator=(const HimotheeChatManager &) = delete;

	void RegisterProvider(std::unique_ptr<HimotheeChatProvider> provider);
	void ConnectAll();
	void DisconnectAll();
	bool ConnectProvider(HimotheeChatPlatform platform);
	void DisconnectProvider(HimotheeChatPlatform platform);
	void SetChannelOverride(HimotheeChatPlatform platform, const std::string &channel);
	HimotheeChatProviderStatus ProviderStatus(HimotheeChatPlatform platform) const;

	bool SendMessage(HimotheeChatPlatform target, const std::string &message);

	void PushMessage(HimotheeChatMessage message);
	void ClearMessages();

	std::vector<HimotheeChatMessage> Snapshot(HimotheeChatPlatform filter = HimotheeChatPlatform::All) const;
	std::vector<HimotheeChatProviderStatus> ProviderStatuses() const;
	void SetProviderStatus(HimotheeChatProviderStatus status);

	size_t MessageCount() const;
	bool AnyConnected() const;

private:
	static constexpr size_t MaxRecentMessages = 500;

	mutable std::mutex mutex;
	std::deque<HimotheeChatMessage> messages;
	std::vector<std::unique_ptr<HimotheeChatProvider>> providers;
	std::vector<HimotheeChatProviderStatus> providerStatuses;
	uint64_t nextSequence = 1;
};

const char *HimotheeChatPlatformName(HimotheeChatPlatform platform) noexcept;
const char *HimotheeChatConnectionStateName(HimotheeChatConnectionState state) noexcept;
