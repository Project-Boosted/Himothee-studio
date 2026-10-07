#include "HimotheeMultistream.hpp"

#include "BasicOutputHandler.hpp"
#include <widgets/OBSBasic.hpp>

#include <util/config-file.h>

#include <algorithm>
#include <cctype>
#include <string_view>
#include <filesystem>
#include <utility>

using namespace std;

namespace {

constexpr const char *kMultistreamFileName = "multistream.json";

bool CodecListContains(const char *supportedCodecs, const char *codec)
{
	if (!codec || !*codec) {
		return false;
	}
	if (!supportedCodecs || !*supportedCodecs) {
		return true;
	}

	auto normalize = [](string value) {
		transform(value.begin(), value.end(), value.begin(),
			  [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
		return value;
	};

	const string wanted = normalize(codec);
	string_view remaining{supportedCodecs};

	while (!remaining.empty()) {
		const size_t separator = remaining.find(';');
		const string token = normalize(string(remaining.substr(0, separator)));
		if (token == wanted) {
			return true;
		}

		if (separator == string_view::npos) {
			break;
		}
		remaining.remove_prefix(separator + 1);
	}

	return false;
}

const char *StateName(HimotheeDestinationState state)
{
	switch (state) {
	case HimotheeDestinationState::Disabled:
		return "disabled";
	case HimotheeDestinationState::Idle:
		return "idle";
	case HimotheeDestinationState::Prepared:
		return "prepared";
	case HimotheeDestinationState::Starting:
		return "starting";
	case HimotheeDestinationState::Active:
		return "active";
	case HimotheeDestinationState::Reconnecting:
		return "reconnecting";
	case HimotheeDestinationState::Stopping:
		return "stopping";
	case HimotheeDestinationState::Error:
		return "error";
	}

	return "unknown";
}

} // namespace

struct HimotheeMultistreamManager::DestinationRuntime {
	HimotheeDestinationConfig config;
	HimotheeDestinationState state = HimotheeDestinationState::Idle;
	string lastError;

	OBSServiceAutoRelease service;
	OBSOutputAutoRelease output;
	OBSEncoderAutoRelease independentVideoEncoder;
	OBSEncoderAutoRelease independentAudioEncoder;

	OBSSignal startSignal;
	OBSSignal stopSignal;
	OBSSignal reconnectSignal;
	OBSSignal reconnectSuccessSignal;
};

HimotheeMultistreamManager::HimotheeMultistreamManager(OBSBasic *main_) : main(main_) {}

HimotheeMultistreamManager::~HimotheeMultistreamManager()
{
	StopAll(true);
	runtimes.clear();
}

bool HimotheeMultistreamManager::LoadForCurrentProfile()
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

bool HimotheeMultistreamManager::Load()
{
	destinations.clear();

	if (!main) {
		return false;
	}

	const OBSProfile &profile = main->GetCurrentProfile();
	const filesystem::path path = profile.path / filesystem::u8path(kMultistreamFileName);
	loadedProfilePath = profile.path.u8string();

	OBSDataAutoRelease root = obs_data_create_from_json_file_safe(path.u8string().c_str(), "bak");
	if (!root) {
		blog(LOG_INFO, "[Himothee Multistream] No destination file at '%s'; multistream is disabled.",
		     path.u8string().c_str());
		return true;
	}

	OBSDataArrayAutoRelease array = obs_data_get_array(root, "destinations");
	if (!array) {
		return true;
	}

	const size_t count = obs_data_array_count(array);
	destinations.reserve(count);

	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(array, i);
		if (!item) {
			continue;
		}

		HimotheeDestinationConfig config;
		config.id = obs_data_get_string(item, "id");
		config.name = obs_data_get_string(item, "name");
		config.platform = obs_data_get_string(item, "platform");
		if (config.platform.empty()) {
			config.platform = "Custom RTMP";
		}
		config.enabled = obs_data_get_bool(item, "enabled");
		const char *encoderMode = obs_data_get_string(item, "encoder_mode");
		config.encoderMode =
			encoderMode && astrcmpi(encoderMode, "independent") == 0 ? HimotheeEncoderMode::Independent
										     : HimotheeEncoderMode::Shared;
		config.videoBitrateKbps = static_cast<int>(obs_data_get_int(item, "video_bitrate_kbps"));
		config.audioBitrateKbps = static_cast<int>(obs_data_get_int(item, "audio_bitrate_kbps"));
		config.outputWidth = static_cast<uint32_t>(obs_data_get_int(item, "output_width"));
		config.outputHeight = static_cast<uint32_t>(obs_data_get_int(item, "output_height"));
		config.server = obs_data_get_string(item, "server");
		config.key = obs_data_get_string(item, "key");
		config.useAuth = obs_data_get_bool(item, "use_auth");
		config.username = obs_data_get_string(item, "username");
		config.password = obs_data_get_string(item, "password");

		if (obs_data_has_user_value(item, "max_retries")) {
			config.maxRetries = static_cast<int>(obs_data_get_int(item, "max_retries"));
		}
		if (obs_data_has_user_value(item, "retry_delay_seconds")) {
			config.retryDelaySeconds = static_cast<int>(obs_data_get_int(item, "retry_delay_seconds"));
		}

		if (config.id.empty()) {
			config.id = "destination-" + to_string(i + 1);
		}
		if (config.name.empty()) {
			config.name = "Destination " + to_string(i + 1);
		}

		destinations.emplace_back(std::move(config));
	}

	blog(LOG_INFO, "[Himothee Multistream] Loaded %zu destination(s).", destinations.size());
	return true;
}

bool HimotheeMultistreamManager::Save() const
{
	if (!main) {
		return false;
	}

	const OBSProfile &profile = main->GetCurrentProfile();
	const filesystem::path path = profile.path / filesystem::u8path(kMultistreamFileName);

	OBSDataAutoRelease root = obs_data_create();
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (const auto &config : destinations) {
		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, "id", config.id.c_str());
		obs_data_set_string(item, "name", config.name.c_str());
		obs_data_set_string(item, "platform", config.platform.c_str());
		obs_data_set_bool(item, "enabled", config.enabled);
		obs_data_set_string(item, "encoder_mode",
				    config.encoderMode == HimotheeEncoderMode::Independent ? "independent" : "shared");
		obs_data_set_int(item, "video_bitrate_kbps", config.videoBitrateKbps);
		obs_data_set_int(item, "audio_bitrate_kbps", config.audioBitrateKbps);
		obs_data_set_int(item, "output_width", config.outputWidth);
		obs_data_set_int(item, "output_height", config.outputHeight);
		obs_data_set_string(item, "server", config.server.c_str());
		obs_data_set_string(item, "key", config.key.c_str());
		obs_data_set_bool(item, "use_auth", config.useAuth);
		obs_data_set_string(item, "username", config.username.c_str());
		obs_data_set_string(item, "password", config.password.c_str());
		obs_data_set_int(item, "max_retries", config.maxRetries);
		obs_data_set_int(item, "retry_delay_seconds", config.retryDelaySeconds);
		obs_data_array_push_back(array, item);
	}

	obs_data_set_array(root, "destinations", array);
	return obs_data_save_json_safe(root, path.u8string().c_str(), "tmp", "bak");
}

bool HimotheeMultistreamManager::ReplaceDestinations(vector<HimotheeDestinationConfig> newDestinations)
{
	if (AnyActive()) {
		blog(LOG_WARNING, "[Himothee Multistream] Destination settings cannot be replaced while streaming.");
		return false;
	}

	for (size_t i = 0; i < newDestinations.size(); i++) {
		auto &config = newDestinations[i];
		if (config.id.empty()) {
			config.id = "destination-" + to_string(i + 1);
		}
		if (config.name.empty()) {
			config.name = "Destination " + to_string(i + 1);
		}

		for (size_t j = 0; j < i; j++) {
			if (newDestinations[j].id == config.id) {
				config.id += "-" + to_string(i + 1);
				break;
			}
		}
	}

	runtimes.clear();
	destinations = std::move(newDestinations);
	return Save();
}

bool HimotheeMultistreamManager::BuildDestinationRuntime(const HimotheeDestinationConfig &config, size_t index,
						    obs_output_t *primaryOutput)
{
	if (!config.enabled) {
		return true;
	}

	auto runtime = make_unique<DestinationRuntime>();
	runtime->config = config;

	if (config.server.empty()) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = "No RTMP/RTMPS server is configured.";
		blog(LOG_WARNING, "[Himothee Multistream] Destination '%s' has no server URL and will be skipped.",
		     config.name.c_str());
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	obs_encoder_t *primaryVideoEncoder = obs_output_get_video_encoder(primaryOutput);
	obs_encoder_t *primaryAudioEncoder = obs_output_get_audio_encoder(primaryOutput, 0);

	if (!primaryVideoEncoder || !primaryAudioEncoder) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = "The primary stream does not expose usable video/audio encoders.";
		blog(LOG_WARNING,
		     "[Himothee Multistream] Destination '%s' cannot be prepared because the primary output "
		     "does not expose both video and audio encoders.",
		     config.name.c_str());
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	obs_encoder_t *videoEncoder = primaryVideoEncoder;
	obs_encoder_t *audioEncoder = primaryAudioEncoder;

	if (config.encoderMode == HimotheeEncoderMode::Independent) {
		const char *videoEncoderId = obs_encoder_get_id(primaryVideoEncoder);
		const char *audioEncoderId = obs_encoder_get_id(primaryAudioEncoder);
		if (!videoEncoderId || !audioEncoderId) {
			runtime->state = HimotheeDestinationState::Error;
			runtime->lastError = "Could not determine the primary encoder types.";
			runtimes.emplace_back(std::move(runtime));
			return false;
		}

		OBSDataAutoRelease videoSettings = obs_encoder_get_settings(primaryVideoEncoder);
		OBSDataAutoRelease audioSettings = obs_encoder_get_settings(primaryAudioEncoder);
		if (config.videoBitrateKbps > 0) {
			obs_data_set_int(videoSettings, "bitrate", config.videoBitrateKbps);
		}
		if (config.audioBitrateKbps > 0) {
			obs_data_set_int(audioSettings, "bitrate", config.audioBitrateKbps);
		}

		const string videoName = "himothee_independent_video_" + to_string(index + 1);
		const string audioName = "himothee_independent_audio_" + to_string(index + 1);

		runtime->independentVideoEncoder = OBSVideoEncoderAutoRelease{
			obs_video_encoder_create(videoEncoderId, videoName.c_str(), videoSettings, nullptr)};
		runtime->independentAudioEncoder = OBSAudioEncoderAutoRelease{
			obs_audio_encoder_create(audioEncoderId, audioName.c_str(), audioSettings,
						 obs_encoder_get_mixer_index(primaryAudioEncoder), nullptr)};

		if (!runtime->independentVideoEncoder || !runtime->independentAudioEncoder) {
			runtime->state = HimotheeDestinationState::Error;
			runtime->lastError = "Failed to create independent video/audio encoders.";
			blog(LOG_WARNING, "[Himothee Multistream] Failed to create independent encoders for '%s'.",
			     config.name.c_str());
			runtimes.emplace_back(std::move(runtime));
			return false;
		}

		obs_encoder_set_video(runtime->independentVideoEncoder, obs_get_video());
		obs_encoder_set_audio(runtime->independentAudioEncoder, obs_get_audio());

		if (config.outputWidth > 0 && config.outputHeight > 0) {
			obs_encoder_set_scaled_size(runtime->independentVideoEncoder, config.outputWidth, config.outputHeight);
		}

		videoEncoder = runtime->independentVideoEncoder;
		audioEncoder = runtime->independentAudioEncoder;
	}

	OBSDataAutoRelease serviceSettings = obs_data_create();
	obs_data_set_string(serviceSettings, "server", config.server.c_str());
	obs_data_set_string(serviceSettings, "key", config.key.c_str());
	obs_data_set_bool(serviceSettings, "use_auth", config.useAuth);
	obs_data_set_string(serviceSettings, "username", config.username.c_str());
	obs_data_set_string(serviceSettings, "password", config.password.c_str());

	const string serviceName = "himothee_multistream_service_" + to_string(index + 1);
	runtime->service =
		OBSServiceAutoRelease{obs_service_create("rtmp_custom", serviceName.c_str(), serviceSettings, nullptr)};
	if (!runtime->service) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = "Failed to create RTMP service.";
		blog(LOG_WARNING, "[Himothee Multistream] Failed to create service for '%s'.", config.name.c_str());
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	const char *outputType = GetStreamOutputType(runtime->service);
	if (!outputType) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = "No compatible OBS output type is available.";
		blog(LOG_WARNING, "[Himothee Multistream] No compatible output for '%s'.", config.name.c_str());
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	const string outputName = "himothee_multistream_output_" + to_string(index + 1);
	runtime->output = OBSOutputAutoRelease{obs_output_create(outputType, outputName.c_str(), nullptr, nullptr)};
	if (!runtime->output) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = "Failed to create streaming output.";
		blog(LOG_WARNING, "[Himothee Multistream] Failed to create output for '%s'.", config.name.c_str());
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	const char *videoCodec = obs_encoder_get_codec(videoEncoder);
	const char *audioCodec = obs_encoder_get_codec(audioEncoder);
	const char *supportedVideoCodecs = obs_output_get_supported_video_codecs(runtime->output);
	const char *supportedAudioCodecs = obs_output_get_supported_audio_codecs(runtime->output);

	if (!CodecListContains(supportedVideoCodecs, videoCodec)) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = string(config.encoderMode == HimotheeEncoderMode::Independent ? "Independent video codec '" :
													   "Shared video codec '") +
				     (videoCodec ? videoCodec : "unknown") + "' is not supported by this destination output.";
		blog(LOG_WARNING, "[Himothee Multistream] Destination '%s' rejected shared video codec '%s'.",
		     config.name.c_str(), videoCodec ? videoCodec : "unknown");
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	if (!CodecListContains(supportedAudioCodecs, audioCodec)) {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = string(config.encoderMode == HimotheeEncoderMode::Independent ? "Independent audio codec '" :
													   "Shared audio codec '") +
				     (audioCodec ? audioCodec : "unknown") + "' is not supported by this destination output.";
		blog(LOG_WARNING, "[Himothee Multistream] Destination '%s' rejected shared audio codec '%s'.",
		     config.name.c_str(), audioCodec ? audioCodec : "unknown");
		runtimes.emplace_back(std::move(runtime));
		return false;
	}

	obs_output_set_video_encoder(runtime->output, videoEncoder);
	obs_output_set_audio_encoder(runtime->output, audioEncoder, 0);
	obs_output_set_service(runtime->output, runtime->service);

	bool reconnect = config_get_bool(main->Config(), "Output", "Reconnect");
	int maxRetries = config.maxRetries >= 0 ? config.maxRetries : config_get_int(main->Config(), "Output", "MaxRetries");
	int retryDelay =
		config.retryDelaySeconds >= 0 ? config.retryDelaySeconds : config_get_int(main->Config(), "Output", "RetryDelay");

	if (!reconnect) {
		maxRetries = 0;
	}

	obs_output_set_reconnect_settings(runtime->output, maxRetries, retryDelay);

	const bool useDelay = config_get_bool(main->Config(), "Output", "DelayEnable");
	const int delaySec = config_get_int(main->Config(), "Output", "DelaySec");
	const bool preserveDelay = config_get_bool(main->Config(), "Output", "DelayPreserve");
	obs_output_set_delay(runtime->output, useDelay ? delaySec : 0,
			     preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	OBSDataAutoRelease outputSettings = obs_data_create();
	obs_data_set_string(outputSettings, "bind_ip", config_get_string(main->Config(), "Output", "BindIP"));
	obs_data_set_string(outputSettings, "ip_family", config_get_string(main->Config(), "Output", "IPFamily"));
	obs_data_set_bool(outputSettings, "dyn_bitrate", config_get_bool(main->Config(), "Output", "DynamicBitrate"));
#ifdef _WIN32
	obs_data_set_bool(outputSettings, "new_socket_loop_enabled",
			  config_get_bool(main->Config(), "Output", "NewSocketLoopEnable"));
	obs_data_set_bool(outputSettings, "low_latency_mode_enabled",
			  config_get_bool(main->Config(), "Output", "LowLatencyEnable"));
#endif
	obs_output_update(runtime->output, outputSettings);

	signal_handler_t *signalHandler = obs_output_get_signal_handler(runtime->output);
	runtime->startSignal.Connect(signalHandler, "start", OnOutputStart, runtime.get());
	runtime->stopSignal.Connect(signalHandler, "stop", OnOutputStop, runtime.get());
	runtime->reconnectSignal.Connect(signalHandler, "reconnect", OnOutputReconnect, runtime.get());
	runtime->reconnectSuccessSignal.Connect(signalHandler, "reconnect_success", OnOutputReconnectSuccess, runtime.get());

	runtime->state = HimotheeDestinationState::Prepared;
	blog(LOG_INFO, "[Himothee Multistream] Prepared destination '%s' using %s encoders.",
	     config.name.c_str(),
	     config.encoderMode == HimotheeEncoderMode::Independent ? "independent" : "shared");

	runtimes.emplace_back(std::move(runtime));
	return true;
}

bool HimotheeMultistreamManager::PrepareSharedOutputs(obs_output_t *primaryOutput)
{
	if (!primaryOutput) {
		return false;
	}

	if (AnyActive()) {
		blog(LOG_WARNING, "[Himothee Multistream] Refusing to rebuild destinations while an output is active.");
		return false;
	}

	runtimes.clear();

	if (!LoadForCurrentProfile()) {
		return false;
	}

	const uint32_t flags = obs_output_get_flags(primaryOutput);
	const bool primaryIsMultitrackVideo = (flags & OBS_OUTPUT_MULTI_TRACK_VIDEO) != 0;

	bool allPrepared = true;
	for (size_t i = 0; i < destinations.size(); i++) {
		if (!destinations[i].enabled) {
			continue;
		}

		if (primaryIsMultitrackVideo && destinations[i].encoderMode == HimotheeEncoderMode::Shared) {
			auto runtime = make_unique<DestinationRuntime>();
			runtime->config = destinations[i];
			runtime->state = HimotheeDestinationState::Error;
			runtime->lastError =
				"Shared mode is unavailable while the primary OBS multitrack-video output is active.";
			runtimes.emplace_back(std::move(runtime));
			allPrepared = false;
			continue;
		}

		if (!BuildDestinationRuntime(destinations[i], i, primaryOutput)) {
			allPrepared = false;
		}
	}

	return allPrepared;
}

size_t HimotheeMultistreamManager::StartPrepared()
{
	size_t started = 0;

	for (const auto &runtime : runtimes) {
		if (runtime->state == HimotheeDestinationState::Prepared && StartDestination(runtime->config.id)) {
			started++;
		}
	}

	if (started > 0) {
		blog(LOG_INFO, "[Himothee Multistream] Started %zu secondary destination(s).", started);
	}

	return started;
}

bool HimotheeMultistreamManager::StartDestination(const string &id)
{
	auto it = find_if(runtimes.begin(), runtimes.end(), [&](const auto &runtime) {
		return runtime->config.id == id;
	});
	if (it == runtimes.end()) {
		return false;
	}

	auto &runtime = *it;
	if (!runtime->output || !runtime->config.enabled) {
		return false;
	}
	if (obs_output_active(runtime->output)) {
		return true;
	}
	if (runtime->state == HimotheeDestinationState::Stopping) {
		return false;
	}

	runtime->state = HimotheeDestinationState::Starting;
	if (obs_output_start(runtime->output)) {
		return true;
	}

	const char *error = obs_output_get_last_error(runtime->output);
	runtime->lastError = error ? error : "Output failed to start.";
	runtime->state = HimotheeDestinationState::Error;

	blog(LOG_WARNING, "[Himothee Multistream] Destination '%s' failed to start: %s",
	     runtime->config.name.c_str(), runtime->lastError.c_str());
	return false;
}

void HimotheeMultistreamManager::StopDestination(const string &id, bool force)
{
	auto it = find_if(runtimes.begin(), runtimes.end(), [&](const auto &runtime) {
		return runtime->config.id == id;
	});
	if (it == runtimes.end()) {
		return;
	}

	auto &runtime = *it;
	if (!runtime->output || !obs_output_active(runtime->output)) {
		return;
	}

	runtime->state = HimotheeDestinationState::Stopping;
	if (force) {
		obs_output_force_stop(runtime->output);
	} else {
		obs_output_stop(runtime->output);
	}
}

void HimotheeMultistreamManager::StopAll(bool force)
{
	for (auto &runtime : runtimes) {
		if (!runtime->output) {
			continue;
		}

		if (obs_output_active(runtime->output)) {
			runtime->state = HimotheeDestinationState::Stopping;
			if (force) {
				obs_output_force_stop(runtime->output);
			} else {
				obs_output_stop(runtime->output);
			}
		}
	}
}

void HimotheeMultistreamManager::ResetPrepared()
{
	if (!AnyActive()) {
		runtimes.clear();
	}
}

bool HimotheeMultistreamManager::AnyActive() const
{
	return any_of(runtimes.begin(), runtimes.end(), [](const auto &runtime) {
		return runtime->output && obs_output_active(runtime->output);
	});
}

size_t HimotheeMultistreamManager::EnabledCount() const
{
	return count_if(destinations.begin(), destinations.end(), [](const auto &config) { return config.enabled; });
}

const vector<HimotheeDestinationConfig> &HimotheeMultistreamManager::Destinations() const noexcept
{
	return destinations;
}

vector<HimotheeDestinationStatus> HimotheeMultistreamManager::Status() const
{
	vector<HimotheeDestinationStatus> result;
	result.reserve(runtimes.size());

	for (const auto &runtime : runtimes) {
		HimotheeDestinationStatus status;
		status.id = runtime->config.id;
		status.name = runtime->config.name;
		status.encoderMode = runtime->config.encoderMode;
		status.state = runtime->state;
		status.lastError = runtime->lastError;

		if (runtime->output) {
			status.totalBytes = obs_output_get_total_bytes(runtime->output);
			status.droppedFrames = obs_output_get_frames_dropped(runtime->output);
			status.totalFrames = obs_output_get_total_frames(runtime->output);
			status.connectTimeMs = obs_output_get_connect_time_ms(runtime->output);
			status.congestion = obs_output_get_congestion(runtime->output);

			obs_encoder_t *videoEncoder = obs_output_get_video_encoder(runtime->output);
			obs_encoder_t *audioEncoder = obs_output_get_audio_encoder(runtime->output, 0);
			const char *videoCodec = videoEncoder ? obs_encoder_get_codec(videoEncoder) : nullptr;
			const char *audioCodec = audioEncoder ? obs_encoder_get_codec(audioEncoder) : nullptr;
			status.videoCodec = videoCodec ? videoCodec : "";
			status.audioCodec = audioCodec ? audioCodec : "";
		}

		result.emplace_back(std::move(status));
	}

	return result;
}

void HimotheeMultistreamManager::OnOutputStart(void *data, calldata_t *)
{
	auto *runtime = static_cast<DestinationRuntime *>(data);
	runtime->state = HimotheeDestinationState::Active;
	runtime->lastError.clear();
	blog(LOG_INFO, "[Himothee Multistream] Destination '%s' is active.", runtime->config.name.c_str());
}

void HimotheeMultistreamManager::OnOutputStop(void *data, calldata_t *params)
{
	auto *runtime = static_cast<DestinationRuntime *>(data);
	const int code = static_cast<int>(calldata_int(params, "code"));
	const char *error = calldata_string(params, "last_error");

	if (code == OBS_OUTPUT_SUCCESS) {
		runtime->state = HimotheeDestinationState::Idle;
		runtime->lastError.clear();
	} else {
		runtime->state = HimotheeDestinationState::Error;
		runtime->lastError = error ? error : "Output stopped with an error.";
	}

	blog(code == OBS_OUTPUT_SUCCESS ? LOG_INFO : LOG_WARNING,
	     "[Himothee Multistream] Destination '%s' stopped (code %d, state %s).", runtime->config.name.c_str(),
	     code, StateName(runtime->state));
}

void HimotheeMultistreamManager::OnOutputReconnect(void *data, calldata_t *)
{
	auto *runtime = static_cast<DestinationRuntime *>(data);
	runtime->state = HimotheeDestinationState::Reconnecting;
	blog(LOG_WARNING, "[Himothee Multistream] Destination '%s' is reconnecting.",
	     runtime->config.name.c_str());
}

void HimotheeMultistreamManager::OnOutputReconnectSuccess(void *data, calldata_t *)
{
	auto *runtime = static_cast<DestinationRuntime *>(data);
	runtime->state = HimotheeDestinationState::Active;
	runtime->lastError.clear();
	blog(LOG_INFO, "[Himothee Multistream] Destination '%s' reconnected.",
	     runtime->config.name.c_str());
}
