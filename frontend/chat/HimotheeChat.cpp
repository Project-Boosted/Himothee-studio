#include "HimotheeChat.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

using namespace std;

namespace {

bool IsChatPlatform(HimotheeChatPlatform platform)
{
	return platform == HimotheeChatPlatform::Twitch || platform == HimotheeChatPlatform::YouTube ||
	       platform == HimotheeChatPlatform::Kick;
}

uint64_t CurrentTimeMs()
{
	return static_cast<uint64_t>(chrono::duration_cast<chrono::milliseconds>(
					    chrono::system_clock::now().time_since_epoch())
					    .count());
}

} // namespace

const char *HimotheeChatPlatformName(HimotheeChatPlatform platform) noexcept
{
	switch (platform) {
	case HimotheeChatPlatform::All:
		return "All";
	case HimotheeChatPlatform::Twitch:
		return "Twitch";
	case HimotheeChatPlatform::YouTube:
		return "YouTube";
	case HimotheeChatPlatform::Kick:
		return "Kick";
	case HimotheeChatPlatform::System:
		return "System";
	}
	return "Unknown";
}

const char *HimotheeChatConnectionStateName(HimotheeChatConnectionState state) noexcept
{
	switch (state) {
	case HimotheeChatConnectionState::Disconnected:
		return "Offline";
	case HimotheeChatConnectionState::Connecting:
		return "Connecting";
	case HimotheeChatConnectionState::Connected:
		return "Connected";
	case HimotheeChatConnectionState::Reconnecting:
		return "Reconnecting";
	case HimotheeChatConnectionState::Error:
		return "Error";
	}
	return "Unknown";
}

HimotheeChatManager::HimotheeChatManager()
{
	providerStatuses = {
		{HimotheeChatPlatform::Twitch, HimotheeChatConnectionState::Disconnected, {}, {}, true, {}},
		{HimotheeChatPlatform::YouTube, HimotheeChatConnectionState::Disconnected, {}, {}, true, {}},
		{HimotheeChatPlatform::Kick, HimotheeChatConnectionState::Disconnected, {}, {}, true, {}},
	};

	HimotheeChatMessage ready;
	ready.platform = HimotheeChatPlatform::System;
	ready.type = HimotheeChatMessageType::System;
	ready.displayName = "Himothee Studio";
	ready.text = "Unified Chat is ready. Twitch, YouTube and Kick connectors will plug into this timeline.";
	PushMessage(std::move(ready));
}

HimotheeChatManager::~HimotheeChatManager()
{
	DisconnectAll();
}

void HimotheeChatManager::RegisterProvider(unique_ptr<HimotheeChatProvider> provider)
{
	if (!provider || !IsChatPlatform(provider->Platform())) {
		return;
	}

	HimotheeChatProviderStatus status;
	status.platform = provider->Platform();
	status.state = provider->State();
	status.accountName = provider->AccountName();
	status.channelName = provider->ChannelName();
	status.automaticChannel = provider->AutomaticChannel();

	{
		lock_guard lock(mutex);
		auto existing = find_if(providers.begin(), providers.end(), [&](const auto &candidate) {
			return candidate->Platform() == provider->Platform();
		});
		if (existing != providers.end()) {
			*existing = std::move(provider);
		} else {
			providers.emplace_back(std::move(provider));
		}

		auto statusIt = find_if(providerStatuses.begin(), providerStatuses.end(),
					[&](const auto &candidate) { return candidate.platform == status.platform; });
		if (statusIt != providerStatuses.end()) {
			*statusIt = std::move(status);
		} else {
			providerStatuses.emplace_back(std::move(status));
		}
	}
}

void HimotheeChatManager::ConnectAll()
{
	vector<HimotheeChatProvider *> snapshot;
	{
		lock_guard lock(mutex);
		for (const auto &provider : providers) {
			snapshot.push_back(provider.get());
		}
	}

	for (auto *provider : snapshot) {
		if (!provider) {
			continue;
		}

		HimotheeChatProviderStatus status;
		status.platform = provider->Platform();
		status.state = HimotheeChatConnectionState::Connecting;
		status.accountName = provider->AccountName();
		SetProviderStatus(status);

		const bool connected = provider->Connect();
		status.state = connected ? provider->State() : HimotheeChatConnectionState::Error;
		if (!connected) {
			status.lastError = "Provider connection failed.";
		}
		SetProviderStatus(std::move(status));
	}
}

void HimotheeChatManager::DisconnectAll()
{
	vector<HimotheeChatProvider *> snapshot;
	{
		lock_guard lock(mutex);
		for (const auto &provider : providers) {
			snapshot.push_back(provider.get());
		}
	}

	for (auto *provider : snapshot) {
		if (!provider) {
			continue;
		}
		provider->Disconnect();
		SetProviderStatus({provider->Platform(), HimotheeChatConnectionState::Disconnected,
				   provider->AccountName(), provider->ChannelName(), provider->AutomaticChannel(), {}});
	}
}

bool HimotheeChatManager::ConnectProvider(HimotheeChatPlatform platform)
{
	HimotheeChatProvider *provider = nullptr;
	{
		lock_guard lock(mutex);
		auto it = find_if(providers.begin(), providers.end(),
				  [&](const auto &candidate) { return candidate && candidate->Platform() == platform; });
		if (it != providers.end()) {
			provider = it->get();
		}
	}

	if (!provider) {
		return false;
	}

	HimotheeChatProviderStatus status;
	status.platform = platform;
	status.state = HimotheeChatConnectionState::Connecting;
	status.accountName = provider->AccountName();
	status.channelName = provider->ChannelName();
	status.automaticChannel = provider->AutomaticChannel();
	SetProviderStatus(status);

	const bool accepted = provider->Connect();
	if (!accepted) {
		status.state = HimotheeChatConnectionState::Error;
		status.lastError = "Provider connection could not be started.";
		SetProviderStatus(std::move(status));
	}
	return accepted;
}

void HimotheeChatManager::DisconnectProvider(HimotheeChatPlatform platform)
{
	HimotheeChatProvider *provider = nullptr;
	{
		lock_guard lock(mutex);
		auto it = find_if(providers.begin(), providers.end(),
				  [&](const auto &candidate) { return candidate && candidate->Platform() == platform; });
		if (it != providers.end()) {
			provider = it->get();
		}
	}

	if (!provider) {
		return;
	}

	provider->Disconnect();
	SetProviderStatus({platform, HimotheeChatConnectionState::Disconnected, provider->AccountName(),
			   provider->ChannelName(), provider->AutomaticChannel(), {}});
}

void HimotheeChatManager::SetChannelOverride(HimotheeChatPlatform platform, const string &channel)
{
	HimotheeChatProvider *provider = nullptr;
	{
		lock_guard lock(mutex);
		auto it = find_if(providers.begin(), providers.end(),
				  [&](const auto &candidate) { return candidate && candidate->Platform() == platform; });
		if (it != providers.end()) {
			provider = it->get();
		}
	}

	if (!provider) {
		return;
	}

	provider->SetChannelOverride(channel);

	auto status = ProviderStatus(platform);
	status.channelName = provider->ChannelName();
	status.automaticChannel = provider->AutomaticChannel();
	SetProviderStatus(std::move(status));
}

HimotheeChatProviderStatus HimotheeChatManager::ProviderStatus(HimotheeChatPlatform platform) const
{
	lock_guard lock(mutex);
	auto it = find_if(providerStatuses.begin(), providerStatuses.end(),
			  [&](const auto &status) { return status.platform == platform; });
	return it != providerStatuses.end() ? *it : HimotheeChatProviderStatus{platform};
}

bool HimotheeChatManager::SendMessage(HimotheeChatPlatform target, const string &message)
{
	if (message.empty()) {
		return false;
	}

	vector<HimotheeChatProvider *> targets;
	{
		lock_guard lock(mutex);
		for (const auto &provider : providers) {
			if (!provider || provider->State() != HimotheeChatConnectionState::Connected) {
				continue;
			}
			if (target == HimotheeChatPlatform::All || provider->Platform() == target) {
				targets.push_back(provider.get());
			}
		}
	}

	bool sent = false;
	for (auto *provider : targets) {
		sent = provider->SendMessage(message) || sent;
	}
	return sent;
}

void HimotheeChatManager::PushMessage(HimotheeChatMessage message)
{
	lock_guard lock(mutex);
	message.sequence = nextSequence++;
	if (message.timestampMs == 0) {
		message.timestampMs = CurrentTimeMs();
	}

	messages.emplace_back(std::move(message));
	while (messages.size() > MaxRecentMessages) {
		messages.pop_front();
	}
}

void HimotheeChatManager::ClearMessages()
{
	lock_guard lock(mutex);
	messages.clear();
}

vector<HimotheeChatMessage> HimotheeChatManager::Snapshot(HimotheeChatPlatform filter) const
{
	lock_guard lock(mutex);
	vector<HimotheeChatMessage> snapshot;
	snapshot.reserve(messages.size());

	for (const auto &message : messages) {
		if (filter == HimotheeChatPlatform::All || message.platform == filter ||
		    message.platform == HimotheeChatPlatform::System) {
			snapshot.push_back(message);
		}
	}

	return snapshot;
}

vector<HimotheeChatProviderStatus> HimotheeChatManager::ProviderStatuses() const
{
	lock_guard lock(mutex);
	return providerStatuses;
}

void HimotheeChatManager::SetProviderStatus(HimotheeChatProviderStatus status)
{
	if (!IsChatPlatform(status.platform)) {
		return;
	}

	lock_guard lock(mutex);
	auto it = find_if(providerStatuses.begin(), providerStatuses.end(),
			  [&](const auto &candidate) { return candidate.platform == status.platform; });
	if (it != providerStatuses.end()) {
		*it = std::move(status);
	} else {
		providerStatuses.emplace_back(std::move(status));
	}
}

size_t HimotheeChatManager::MessageCount() const
{
	lock_guard lock(mutex);
	return messages.size();
}

bool HimotheeChatManager::AnyConnected() const
{
	lock_guard lock(mutex);
	return any_of(providerStatuses.begin(), providerStatuses.end(), [](const auto &status) {
		return status.state == HimotheeChatConnectionState::Connected;
	});
}
