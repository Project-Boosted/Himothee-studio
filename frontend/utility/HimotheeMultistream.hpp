#pragma once

#include <obs.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class OBSBasic;

enum class HimotheeEncoderMode {
	Shared,
	Independent,
};

enum class HimotheeReconnectPolicy {
	Inherit,
	Enabled,
	Disabled,
};

enum class HimotheeDestinationState {
	Disabled,
	Idle,
	Prepared,
	Starting,
	Active,
	Reconnecting,
	Stopping,
	Error,
};

struct HimotheeDestinationConfig {
	std::string id;
	std::string name;
	std::string platform = "Custom RTMP";
	bool enabled = true;
	HimotheeEncoderMode encoderMode = HimotheeEncoderMode::Shared;
	int videoBitrateKbps = 0;
	int audioBitrateKbps = 0;
	uint32_t audioTrack = 1;
	uint32_t outputWidth = 0;
	uint32_t outputHeight = 0;
	HimotheeReconnectPolicy reconnectPolicy = HimotheeReconnectPolicy::Inherit;

	std::string server;
	std::string key;

	bool useAuth = false;
	std::string username;
	std::string password;

	int maxRetries = -1;
	int retryDelaySeconds = -1;
};

struct HimotheeDestinationStatus {
	std::string id;
	std::string name;
	HimotheeEncoderMode encoderMode = HimotheeEncoderMode::Shared;
	HimotheeDestinationState state = HimotheeDestinationState::Idle;
	std::string lastError;
	uint64_t totalBytes = 0;
	int droppedFrames = 0;
	int totalFrames = 0;
	int connectTimeMs = -1;
	float congestion = 0.0f;
	uint32_t reconnectCount = 0;
	uint32_t errorCount = 0;
	uint64_t uptimeSeconds = 0;
	uint64_t stateSeconds = 0;
	uint32_t audioTrack = 1;
	bool dedicatedAudioEncoder = false;
	std::string videoCodec;
	std::string audioCodec;
};

class HimotheeMultistreamManager {
public:
	explicit HimotheeMultistreamManager(OBSBasic *main);
	~HimotheeMultistreamManager();

	bool Load();
	bool Save() const;
	bool ReplaceDestinations(std::vector<HimotheeDestinationConfig> newDestinations);

	bool PrepareSharedOutputs(obs_output_t *primaryOutput);
	size_t StartPrepared();
	bool StartDestination(const std::string &id);
	void StopDestination(const std::string &id, bool force = false);
	void StopAll(bool force = false);
	void ResetPrepared();

	bool AnyActive() const;
	size_t EnabledCount() const;

	const std::vector<HimotheeDestinationConfig> &Destinations() const noexcept;
	std::vector<HimotheeDestinationStatus> Status() const;

private:
	struct DestinationRuntime;

	OBSBasic *main = nullptr;
	std::vector<HimotheeDestinationConfig> destinations;
	std::vector<std::unique_ptr<DestinationRuntime>> runtimes;
	std::string loadedProfilePath;

	bool LoadForCurrentProfile();
	bool BuildDestinationRuntime(const HimotheeDestinationConfig &config, size_t index, obs_output_t *primaryOutput);

	static void OnOutputStart(void *data, calldata_t *params);
	static void OnOutputStop(void *data, calldata_t *params);
	static void OnOutputReconnect(void *data, calldata_t *params);
	static void OnOutputReconnectSuccess(void *data, calldata_t *params);
};
