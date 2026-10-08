#include "HimotheeActionRegistry.hpp"

#include <docks/HimotheeOverlayDock.hpp>
#include <overlays/HimotheeOverlayEngine.hpp>
#include <utility/HimotheeMultistream.hpp>
#include <widgets/OBSBasic.hpp>

#include <QApplication>
#include <QJsonDocument>
#include <QThread>

#include <algorithm>

using namespace std;

namespace {

QJsonObject Param(const QString &type, bool required, const QString &description)
{
	QJsonObject object;
	object.insert(QStringLiteral("type"), type);
	object.insert(QStringLiteral("required"), required);
	object.insert(QStringLiteral("description"), description);
	return object;
}

QJsonObject Params(initializer_list<pair<QString, QJsonObject>> values)
{
	QJsonObject object;
	for (const auto &[key, value] : values) {
		object.insert(key, value);
	}
	return object;
}

QString DestinationStateName(HimotheeDestinationState state)
{
	switch (state) {
	case HimotheeDestinationState::Disabled:
		return QStringLiteral("disabled");
	case HimotheeDestinationState::Idle:
		return QStringLiteral("idle");
	case HimotheeDestinationState::Prepared:
		return QStringLiteral("prepared");
	case HimotheeDestinationState::Starting:
		return QStringLiteral("starting");
	case HimotheeDestinationState::Active:
		return QStringLiteral("active");
	case HimotheeDestinationState::Reconnecting:
		return QStringLiteral("reconnecting");
	case HimotheeDestinationState::Stopping:
		return QStringLiteral("stopping");
	case HimotheeDestinationState::Error:
	default:
		return QStringLiteral("error");
	}
}

} // namespace

QJsonObject HimotheeActionResult::ToJson() const
{
	QJsonObject object;
	object.insert(QStringLiteral("success"), success);
	object.insert(QStringLiteral("code"), code);
	object.insert(QStringLiteral("message"), message);
	object.insert(QStringLiteral("data"), data);
	return object;
}

HimotheeActionRegistry::HimotheeActionRegistry(OBSBasic *main_) : main(main_)
{
	RegisterBuiltInActions();
	blog(LOG_INFO, "[Himothee Actions] Registered %zu action(s).", actions.size());
}

void HimotheeActionRegistry::RegisterBuiltInActions()
{
	actions = {
		{QStringLiteral("stream.start"), QStringLiteral("Start Stream"), QStringLiteral("Streaming"),
		 QStringLiteral("Start the OBS primary stream."), {}},
		{QStringLiteral("stream.stop"), QStringLiteral("Stop Stream"), QStringLiteral("Streaming"),
		 QStringLiteral("Stop the OBS primary stream."), {}},
		{QStringLiteral("stream.toggle"), QStringLiteral("Toggle Stream"), QStringLiteral("Streaming"),
		 QStringLiteral("Start or stop the OBS primary stream."), {}},

		{QStringLiteral("record.start"), QStringLiteral("Start Recording"), QStringLiteral("Recording"),
		 QStringLiteral("Start recording."), {}},
		{QStringLiteral("record.stop"), QStringLiteral("Stop Recording"), QStringLiteral("Recording"),
		 QStringLiteral("Stop recording."), {}},
		{QStringLiteral("record.toggle"), QStringLiteral("Toggle Recording"), QStringLiteral("Recording"),
		 QStringLiteral("Start or stop recording."), {}},

		{QStringLiteral("replay.start"), QStringLiteral("Start Replay Buffer"), QStringLiteral("Replay Buffer"),
		 QStringLiteral("Start the replay buffer."), {}},
		{QStringLiteral("replay.stop"), QStringLiteral("Stop Replay Buffer"), QStringLiteral("Replay Buffer"),
		 QStringLiteral("Stop the replay buffer."), {}},
		{QStringLiteral("replay.toggle"), QStringLiteral("Toggle Replay Buffer"), QStringLiteral("Replay Buffer"),
		 QStringLiteral("Start or stop the replay buffer."), {}},
		{QStringLiteral("replay.save"), QStringLiteral("Save Replay"), QStringLiteral("Replay Buffer"),
		 QStringLiteral("Save the current replay buffer."), {}},

		{QStringLiteral("destination.start"), QStringLiteral("Start Destination"), QStringLiteral("Multistream"),
		 QStringLiteral("Start one prepared Himothee multistream destination."),
		 Params({{QStringLiteral("destination_id"), Param(QStringLiteral("string"), true,
							       QStringLiteral("Destination ID."))}})},
		{QStringLiteral("destination.stop"), QStringLiteral("Stop Destination"), QStringLiteral("Multistream"),
		 QStringLiteral("Stop one Himothee multistream destination."),
		 Params({{QStringLiteral("destination_id"), Param(QStringLiteral("string"), true,
							       QStringLiteral("Destination ID."))}})},
		{QStringLiteral("destination.start_all"), QStringLiteral("Start Enabled Destinations"),
		 QStringLiteral("Multistream"), QStringLiteral("Start every enabled prepared secondary destination."), {}},
		{QStringLiteral("destination.stop_all"), QStringLiteral("Stop Secondary Destinations"),
		 QStringLiteral("Multistream"), QStringLiteral("Stop every Himothee secondary destination."), {}},

		{QStringLiteral("overlay.show"), QStringLiteral("Show Overlay"), QStringLiteral("Overlays"),
		 QStringLiteral("Show one overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},
		{QStringLiteral("overlay.hide"), QStringLiteral("Hide Overlay"), QStringLiteral("Overlays"),
		 QStringLiteral("Hide one overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},
		{QStringLiteral("overlay.toggle"), QStringLiteral("Toggle Overlay"), QStringLiteral("Overlays"),
		 QStringLiteral("Toggle one overlay's configured visibility."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},

		{QStringLiteral("counter.increment"), QStringLiteral("Increment Counter"), QStringLiteral("Counters"),
		 QStringLiteral("Increase a counter-compatible overlay."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))},
			 {QStringLiteral("amount"), Param(QStringLiteral("integer"), false, QStringLiteral("Amount, default 1."))},
		 })},
		{QStringLiteral("counter.decrement"), QStringLiteral("Decrement Counter"), QStringLiteral("Counters"),
		 QStringLiteral("Decrease a counter-compatible overlay."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))},
			 {QStringLiteral("amount"), Param(QStringLiteral("integer"), false, QStringLiteral("Amount, default 1."))},
		 })},
		{QStringLiteral("counter.reset"), QStringLiteral("Reset Counter"), QStringLiteral("Counters"),
		 QStringLiteral("Reset a counter-compatible overlay to zero."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},

		{QStringLiteral("timer.start"), QStringLiteral("Start Timer"), QStringLiteral("Timers"),
		 QStringLiteral("Start or resume a timer overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},
		{QStringLiteral("timer.pause"), QStringLiteral("Pause Timer"), QStringLiteral("Timers"),
		 QStringLiteral("Pause a timer overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},
		{QStringLiteral("timer.reset"), QStringLiteral("Reset Timer"), QStringLiteral("Timers"),
		 QStringLiteral("Reset a timer overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Overlay ID."))}})},

		{QStringLiteral("darts.average.set"), QStringLiteral("Set Darts Average"), QStringLiteral("Darts"),
		 QStringLiteral("Set a Darts Average overlay."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Darts Average overlay ID."))},
			 {QStringLiteral("value"), Param(QStringLiteral("number"), true, QStringLiteral("Three-dart average."))},
		 })},
		{QStringLiteral("darts.checkout.trigger"), QStringLiteral("Trigger Checkout"), QStringLiteral("Darts"),
		 QStringLiteral("Trigger a timed Darts Checkout notification."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Darts Checkout overlay ID."))},
			 {QStringLiteral("score"), Param(QStringLiteral("integer"), true, QStringLiteral("Checkout score 0-170."))},
			 {QStringLiteral("route"), Param(QStringLiteral("string"), false, QStringLiteral("Checkout route text."))},
			 {QStringLiteral("duration_ms"), Param(QStringLiteral("integer"), false,
							       QStringLiteral("Visible duration, default 3000 ms."))},
		 })},
		{QStringLiteral("darts.checkout.clear"), QStringLiteral("Clear Checkout"), QStringLiteral("Darts"),
		 QStringLiteral("Hide an active Darts Checkout notification."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true,
							    QStringLiteral("Darts Checkout overlay ID."))}})},

		{QStringLiteral("gaming.stat.increment"), QStringLiteral("Increment Gaming Stat"),
		 QStringLiteral("Gaming"), QStringLiteral("Increase kills, deaths, assists, wins, or losses."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Gaming overlay ID."))},
			 {QStringLiteral("stat"), Param(QStringLiteral("string"), true,
							      QStringLiteral("kills/deaths/assists/wins/losses."))},
			 {QStringLiteral("amount"), Param(QStringLiteral("integer"), false, QStringLiteral("Amount, default 1."))},
		 })},
		{QStringLiteral("gaming.stat.decrement"), QStringLiteral("Decrement Gaming Stat"),
		 QStringLiteral("Gaming"), QStringLiteral("Decrease kills, deaths, assists, wins, or losses."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Gaming overlay ID."))},
			 {QStringLiteral("stat"), Param(QStringLiteral("string"), true,
							      QStringLiteral("kills/deaths/assists/wins/losses."))},
			 {QStringLiteral("amount"), Param(QStringLiteral("integer"), false, QStringLiteral("Amount, default 1."))},
		 })},
		{QStringLiteral("gaming.stats.reset"), QStringLiteral("Reset Gaming Stats"), QStringLiteral("Gaming"),
		 QStringLiteral("Reset gaming statistics for one gaming overlay."),
		 Params({{QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true, QStringLiteral("Gaming overlay ID."))}})},
		{QStringLiteral("gaming.pb.set"), QStringLiteral("Set Personal Best"), QStringLiteral("Gaming"),
		 QStringLiteral("Set the free-text Gaming Personal Best value."),
		 Params({
			 {QStringLiteral("overlay_id"), Param(QStringLiteral("string"), true,
							    QStringLiteral("Gaming Personal Best overlay ID."))},
			 {QStringLiteral("value"), Param(QStringLiteral("string"), true, QStringLiteral("Personal Best text."))},
		 })},
	};
}

QJsonArray HimotheeActionRegistry::ActionsJson() const
{
	QJsonArray result;
	for (const auto &action : actions) {
		QJsonObject object;
		object.insert(QStringLiteral("id"), action.id);
		object.insert(QStringLiteral("name"), action.name);
		object.insert(QStringLiteral("category"), action.category);
		object.insert(QStringLiteral("description"), action.description);
		object.insert(QStringLiteral("parameters"), action.parameters);
		result.push_back(object);
	}
	return result;
}

QJsonObject HimotheeActionRegistry::StateSnapshot() const
{
	QJsonObject state;
	if (!main) {
		state.insert(QStringLiteral("ready"), false);
		return state;
	}

	state.insert(QStringLiteral("ready"), main->Config() != nullptr);
	state.insert(QStringLiteral("streaming"), main->StreamingActive());
	state.insert(QStringLiteral("recording"), main->RecordingActive());
	state.insert(QStringLiteral("replay_buffer"), main->ReplayBufferActive());

	QJsonArray destinations;
	if (auto *manager = main->GetHimotheeMultistreamManager()) {
		for (const auto &status : manager->Status()) {
			QJsonObject item;
			item.insert(QStringLiteral("id"), QString::fromStdString(status.id));
			item.insert(QStringLiteral("name"), QString::fromStdString(status.name));
			item.insert(QStringLiteral("state"), DestinationStateName(status.state));
			item.insert(QStringLiteral("bitrate_kbps"),
				    status.uptimeSeconds > 0 ? static_cast<double>((status.totalBytes * 8) / status.uptimeSeconds / 1000)
						     : 0.0);
			item.insert(QStringLiteral("dropped_frames"), status.droppedFrames);
			item.insert(QStringLiteral("last_error"), QString::fromStdString(status.lastError));
			destinations.push_back(item);
		}
	}
	state.insert(QStringLiteral("destinations"), destinations);

	QJsonArray overlays;
	if (auto *manager = OverlayManager()) {
		manager->LoadForCurrentProfile();
		for (const auto &overlay : manager->Overlays()) {
			QJsonObject item;
			item.insert(QStringLiteral("id"), QString::fromStdString(overlay.id));
			item.insert(QStringLiteral("name"), QString::fromStdString(overlay.name));
			item.insert(QStringLiteral("type"), QString::fromUtf8(HimotheeOverlayTypeId(overlay.type)));
			item.insert(QStringLiteral("visible"), overlay.visible);
			item.insert(QStringLiteral("display"), manager->RuntimeDisplay(overlay.id));
			overlays.push_back(item);
		}
	}
	state.insert(QStringLiteral("overlays"), overlays);
	return state;
}

HimotheeActionResult HimotheeActionRegistry::Execute(const QString &actionId, const QJsonObject &params)
{
	if (!main) {
		return Fail(QStringLiteral("not_ready"), QStringLiteral("Himothee Studio is not ready."));
	}
	if (QApplication::instance() && QThread::currentThread() != QApplication::instance()->thread()) {
		return Fail(QStringLiteral("wrong_thread"),
			    QStringLiteral("Himothee actions must execute on the application UI thread."));
	}

	if (actionId.startsWith(QStringLiteral("stream."))) return ExecuteStream(actionId);
	if (actionId.startsWith(QStringLiteral("record."))) return ExecuteRecording(actionId);
	if (actionId.startsWith(QStringLiteral("replay."))) return ExecuteReplayBuffer(actionId);
	if (actionId.startsWith(QStringLiteral("destination."))) return ExecuteDestination(actionId, params);
	if (actionId.startsWith(QStringLiteral("overlay."))) return ExecuteOverlay(actionId, params);
	if (actionId.startsWith(QStringLiteral("counter."))) return ExecuteCounter(actionId, params);
	if (actionId.startsWith(QStringLiteral("timer."))) return ExecuteTimer(actionId, params);
	if (actionId.startsWith(QStringLiteral("darts."))) return ExecuteDarts(actionId, params);
	if (actionId.startsWith(QStringLiteral("gaming."))) return ExecuteGaming(actionId, params);

	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown Himothee action: %1").arg(actionId));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteStream(const QString &actionId)
{
	if (actionId == QStringLiteral("stream.start")) {
		if (main->StreamingActive()) return Ok(QStringLiteral("Stream is already active."));
		main->StartStreaming();
		return Ok(QStringLiteral("Stream start requested."));
	}
	if (actionId == QStringLiteral("stream.stop")) {
		if (!main->StreamingActive()) return Ok(QStringLiteral("Stream is already stopped."));
		main->StopStreaming();
		return Ok(QStringLiteral("Stream stop requested."));
	}
	if (actionId == QStringLiteral("stream.toggle")) {
		return ExecuteStream(main->StreamingActive() ? QStringLiteral("stream.stop") : QStringLiteral("stream.start"));
	}
	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown streaming action."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteRecording(const QString &actionId)
{
	if (actionId == QStringLiteral("record.start")) {
		if (main->RecordingActive()) return Ok(QStringLiteral("Recording is already active."));
		main->StartRecording();
		return Ok(QStringLiteral("Recording start requested."));
	}
	if (actionId == QStringLiteral("record.stop")) {
		if (!main->RecordingActive()) return Ok(QStringLiteral("Recording is already stopped."));
		main->StopRecording();
		return Ok(QStringLiteral("Recording stop requested."));
	}
	if (actionId == QStringLiteral("record.toggle")) {
		return ExecuteRecording(main->RecordingActive() ? QStringLiteral("record.stop") : QStringLiteral("record.start"));
	}
	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown recording action."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteReplayBuffer(const QString &actionId)
{
	if (actionId == QStringLiteral("replay.start")) {
		if (main->ReplayBufferActive()) return Ok(QStringLiteral("Replay buffer is already active."));
		main->StartReplayBuffer();
		return Ok(QStringLiteral("Replay buffer start requested."));
	}
	if (actionId == QStringLiteral("replay.stop")) {
		if (!main->ReplayBufferActive()) return Ok(QStringLiteral("Replay buffer is already stopped."));
		main->StopReplayBuffer();
		return Ok(QStringLiteral("Replay buffer stop requested."));
	}
	if (actionId == QStringLiteral("replay.toggle")) {
		return ExecuteReplayBuffer(main->ReplayBufferActive() ? QStringLiteral("replay.stop")
								       : QStringLiteral("replay.start"));
	}
	if (actionId == QStringLiteral("replay.save")) {
		if (!main->ReplayBufferActive()) {
			return Fail(QStringLiteral("replay_not_active"), QStringLiteral("Replay buffer is not active."));
		}
		main->ReplayBufferSave();
		return Ok(QStringLiteral("Replay save requested."));
	}
	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown replay-buffer action."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteDestination(const QString &actionId, const QJsonObject &params)
{
	auto *manager = main->GetHimotheeMultistreamManager();
	if (!manager) {
		return Fail(QStringLiteral("multistream_unavailable"), QStringLiteral("Multistream manager is unavailable."));
	}

	if (actionId == QStringLiteral("destination.start_all")) {
		if (!main->StreamingActive()) {
			return Fail(QStringLiteral("primary_not_live"),
				    QStringLiteral("The primary stream must be live before starting secondary destinations."));
		}
		const size_t started = manager->StartAllEnabled();
		QJsonObject data;
		data.insert(QStringLiteral("started"), static_cast<int>(started));
		return Ok(QStringLiteral("Enabled secondary destinations processed."), data);
	}

	if (actionId == QStringLiteral("destination.stop_all")) {
		manager->StopAll(false);
		return Ok(QStringLiteral("Secondary destinations stop requested."));
	}

	const QString id = RequiredString(params, QStringLiteral("destination_id"));
	if (id.isEmpty()) {
		return Fail(QStringLiteral("missing_parameter"), QStringLiteral("destination_id is required."));
	}

	if (actionId == QStringLiteral("destination.start")) {
		if (!main->StreamingActive()) {
			return Fail(QStringLiteral("primary_not_live"),
				    QStringLiteral("The primary stream must be live before starting a secondary destination."));
		}
		if (!manager->StartDestination(id.toStdString())) {
			return Fail(QStringLiteral("destination_start_failed"),
				    QStringLiteral("The destination could not be started. It may be disabled or not prepared."));
		}
		return Ok(QStringLiteral("Destination start requested."));
	}
	if (actionId == QStringLiteral("destination.stop")) {
		manager->StopDestination(id.toStdString(), false);
		return Ok(QStringLiteral("Destination stop requested."));
	}

	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown destination action."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteOverlay(const QString &actionId, const QJsonObject &params)
{
	auto *manager = OverlayManager();
	if (!manager || !manager->LoadForCurrentProfile()) {
		return Fail(QStringLiteral("overlay_unavailable"), QStringLiteral("Overlay manager/profile is unavailable."));
	}

	const QString id = RequiredString(params, QStringLiteral("overlay_id"));
	if (id.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("overlay_id is required."));

	auto *overlay = manager->Find(id.toStdString());
	if (!overlay) return Fail(QStringLiteral("overlay_not_found"), QStringLiteral("Overlay was not found."));

	bool visible = overlay->visible;
	if (actionId == QStringLiteral("overlay.show")) visible = true;
	else if (actionId == QStringLiteral("overlay.hide")) visible = false;
	else if (actionId == QStringLiteral("overlay.toggle")) visible = !visible;
	else return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown overlay action."));

	if (!manager->SetVisible(id.toStdString(), visible)) {
		return Fail(QStringLiteral("overlay_update_failed"), QStringLiteral("Overlay visibility could not be saved."));
	}

	QJsonObject data;
	data.insert(QStringLiteral("visible"), visible);
	return Ok(visible ? QStringLiteral("Overlay shown.") : QStringLiteral("Overlay hidden."), data);
}

HimotheeActionResult HimotheeActionRegistry::ExecuteCounter(const QString &actionId, const QJsonObject &params)
{
	auto *manager = OverlayManager();
	if (!manager || !manager->LoadForCurrentProfile()) {
		return Fail(QStringLiteral("overlay_unavailable"), QStringLiteral("Overlay manager/profile is unavailable."));
	}

	const QString id = RequiredString(params, QStringLiteral("overlay_id"));
	if (id.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("overlay_id is required."));

	if (actionId == QStringLiteral("counter.reset")) {
		if (!manager->ResetValue(id.toStdString())) {
			return Fail(QStringLiteral("counter_action_failed"), QStringLiteral("Overlay is not a counter."));
		}
		return Ok(QStringLiteral("Counter reset."));
	}

	const int amount = max(1, params.value(QStringLiteral("amount")).toInt(1));
	const int delta = actionId == QStringLiteral("counter.increment") ? amount
			 : actionId == QStringLiteral("counter.decrement")   ? -amount
										    : 0;
	if (delta == 0) return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown counter action."));

	if (!manager->AdjustValue(id.toStdString(), delta)) {
		return Fail(QStringLiteral("counter_action_failed"), QStringLiteral("Overlay is not a counter."));
	}
	return Ok(QStringLiteral("Counter updated."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteTimer(const QString &actionId, const QJsonObject &params)
{
	auto *manager = OverlayManager();
	if (!manager || !manager->LoadForCurrentProfile()) {
		return Fail(QStringLiteral("overlay_unavailable"), QStringLiteral("Overlay manager/profile is unavailable."));
	}

	const QString id = RequiredString(params, QStringLiteral("overlay_id"));
	if (id.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("overlay_id is required."));

	bool success = false;
	if (actionId == QStringLiteral("timer.start")) success = manager->StartTimer(id.toStdString());
	else if (actionId == QStringLiteral("timer.pause")) success = manager->PauseTimer(id.toStdString());
	else if (actionId == QStringLiteral("timer.reset")) success = manager->ResetTimer(id.toStdString());
	else return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown timer action."));

	return success ? Ok(QStringLiteral("Timer updated."))
		       : Fail(QStringLiteral("timer_action_failed"), QStringLiteral("Overlay is not a timer."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteDarts(const QString &actionId, const QJsonObject &params)
{
	auto *manager = OverlayManager();
	if (!manager || !manager->LoadForCurrentProfile()) {
		return Fail(QStringLiteral("overlay_unavailable"), QStringLiteral("Overlay manager/profile is unavailable."));
	}

	const QString id = RequiredString(params, QStringLiteral("overlay_id"));
	if (id.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("overlay_id is required."));

	if (actionId == QStringLiteral("darts.average.set")) {
		if (!params.contains(QStringLiteral("value"))) {
			return Fail(QStringLiteral("missing_parameter"), QStringLiteral("value is required."));
		}
		if (!manager->SetDecimalValue(id.toStdString(), params.value(QStringLiteral("value")).toDouble())) {
			return Fail(QStringLiteral("darts_action_failed"), QStringLiteral("Overlay is not a Darts Average widget."));
		}
		return Ok(QStringLiteral("Darts average updated."));
	}

	if (actionId == QStringLiteral("darts.checkout.trigger")) {
		if (!params.contains(QStringLiteral("score"))) {
			return Fail(QStringLiteral("missing_parameter"), QStringLiteral("score is required."));
		}
		const int score = params.value(QStringLiteral("score")).toInt();
		const QString route = params.value(QStringLiteral("route")).toString();
		const int durationMs = params.value(QStringLiteral("duration_ms")).toInt(3000);
		if (!manager->TriggerCheckout(id.toStdString(), score, route.toStdString(), durationMs)) {
			return Fail(QStringLiteral("darts_action_failed"), QStringLiteral("Overlay is not a Darts Checkout widget."));
		}
		return Ok(QStringLiteral("Checkout notification triggered."));
	}

	if (actionId == QStringLiteral("darts.checkout.clear")) {
		if (!manager->ClearCheckout(id.toStdString())) {
			return Fail(QStringLiteral("darts_action_failed"), QStringLiteral("Overlay is not a Darts Checkout widget."));
		}
		return Ok(QStringLiteral("Checkout notification cleared."));
	}

	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown darts action."));
}

HimotheeActionResult HimotheeActionRegistry::ExecuteGaming(const QString &actionId, const QJsonObject &params)
{
	auto *manager = OverlayManager();
	if (!manager || !manager->LoadForCurrentProfile()) {
		return Fail(QStringLiteral("overlay_unavailable"), QStringLiteral("Overlay manager/profile is unavailable."));
	}

	const QString id = RequiredString(params, QStringLiteral("overlay_id"));
	if (id.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("overlay_id is required."));

	if (actionId == QStringLiteral("gaming.stats.reset")) {
		if (!manager->ResetGamingStats(id.toStdString())) {
			return Fail(QStringLiteral("gaming_action_failed"), QStringLiteral("Overlay is not a gaming widget."));
		}
		return Ok(QStringLiteral("Gaming statistics reset."));
	}

	if (actionId == QStringLiteral("gaming.pb.set")) {
		const QString value = RequiredString(params, QStringLiteral("value"));
		if (value.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("value is required."));
		if (!manager->SetPersonalBest(id.toStdString(), value.toStdString())) {
			return Fail(QStringLiteral("gaming_action_failed"),
				    QStringLiteral("Overlay is not a Gaming Personal Best widget."));
		}
		return Ok(QStringLiteral("Personal Best updated."));
	}

	if (actionId == QStringLiteral("gaming.stat.increment") ||
	    actionId == QStringLiteral("gaming.stat.decrement")) {
		const QString stat = RequiredString(params, QStringLiteral("stat")).toLower();
		if (stat.isEmpty()) return Fail(QStringLiteral("missing_parameter"), QStringLiteral("stat is required."));
		const int amount = max(1, params.value(QStringLiteral("amount")).toInt(1));
		const int delta = actionId == QStringLiteral("gaming.stat.increment") ? amount : -amount;
		if (!manager->AdjustGamingStat(id.toStdString(), stat.toStdString(), delta)) {
			return Fail(QStringLiteral("gaming_action_failed"),
				    QStringLiteral("Gaming stat or widget is not compatible with this action."));
		}
		return Ok(QStringLiteral("Gaming statistic updated."));
	}

	return Fail(QStringLiteral("unknown_action"), QStringLiteral("Unknown gaming action."));
}

HimotheeOverlayManager *HimotheeActionRegistry::OverlayManager() const
{
	if (!main) return nullptr;
	return main->GetHimotheeOverlayManager();
}

HimotheeActionResult HimotheeActionRegistry::Ok(const QString &message, const QJsonObject &data)
{
	HimotheeActionResult result;
	result.success = true;
	result.code = QStringLiteral("ok");
	result.message = message;
	result.data = data;
	return result;
}

HimotheeActionResult HimotheeActionRegistry::Fail(const QString &code, const QString &message)
{
	HimotheeActionResult result;
	result.success = false;
	result.code = code;
	result.message = message;
	return result;
}

QString HimotheeActionRegistry::RequiredString(const QJsonObject &params, const QString &key)
{
	return params.value(key).toString().trimmed();
}
