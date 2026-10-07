#include "HimotheeMultistreamDock.hpp"

#include <widgets/OBSBasic.hpp>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <unordered_map>

using namespace std;

namespace {

QString StateText(HimotheeDestinationState state)
{
	switch (state) {
	case HimotheeDestinationState::Disabled:
		return QStringLiteral("Disabled");
	case HimotheeDestinationState::Idle:
		return QStringLiteral("Idle");
	case HimotheeDestinationState::Prepared:
		return QStringLiteral("Prepared");
	case HimotheeDestinationState::Starting:
		return QStringLiteral("Starting");
	case HimotheeDestinationState::Active:
		return QStringLiteral("Live");
	case HimotheeDestinationState::Reconnecting:
		return QStringLiteral("Reconnecting");
	case HimotheeDestinationState::Stopping:
		return QStringLiteral("Stopping");
	case HimotheeDestinationState::Error:
		return QStringLiteral("Error");
	}

	return QStringLiteral("Unknown");
}

QString HealthText(const HimotheeDestinationStatus &status)
{
	if (status.state == HimotheeDestinationState::Error) {
		return QStringLiteral("Error");
	}
	if (status.state == HimotheeDestinationState::Reconnecting) {
		return QStringLiteral("Reconnect");
	}
	if (status.state != HimotheeDestinationState::Active) {
		return QStringLiteral("-");
	}
	if (status.congestion >= 0.65f) {
		return QStringLiteral("Congested");
	}
	if (status.congestion >= 0.25f) {
		return QStringLiteral("Busy");
	}
	return QStringLiteral("Good");
}

QString FormatDuration(uint64_t seconds)
{
	const uint64_t hours = seconds / 3600;
	const uint64_t minutes = (seconds % 3600) / 60;
	const uint64_t secs = seconds % 60;

	if (hours > 0) {
		return QStringLiteral("%1:%2:%3")
			.arg(hours)
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(secs, 2, 10, QLatin1Char('0'));
	}
	return QStringLiteral("%1:%2").arg(minutes).arg(secs, 2, 10, QLatin1Char('0'));
}

QString FormatBytes(uint64_t bytes)
{
	static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	double value = static_cast<double>(bytes);
	int unit = 0;
	while (value >= 1024.0 && unit < 4) {
		value /= 1024.0;
		unit++;
	}

	return QStringLiteral("%1 %2").arg(value, 0, unit == 0 ? 'f' : 'f', unit == 0 ? 0 : 1).arg(units[unit]);
}

} // namespace

HimotheeMultistreamDock::HimotheeMultistreamDock(OBSBasic *main_) : OBSDock(QStringLiteral("Multistream"), main_), main(main_)
{
	setObjectName(QStringLiteral("himotheeMultistreamDock"));
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	BuildUi();

	statusTimer = new QTimer(this);
	statusTimer->setInterval(750);
	connect(statusTimer, &QTimer::timeout, this, [this]() { RefreshStatus(); });
	statusTimer->start();

	ReloadFromManager();
}

void HimotheeMultistreamDock::BuildUi()
{
	auto *root = new QWidget(this);
	auto *layout = new QVBoxLayout(root);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(8);

	summaryLabel = new QLabel(QStringLiteral("Primary stream: Stopped"), root);
	layout->addWidget(summaryLabel);

	destinationTree = new QTreeWidget(root);
	destinationTree->setColumnCount(12);
	destinationTree->setHeaderLabels(
		{QStringLiteral("Destination"), QStringLiteral("Platform"), QStringLiteral("Enabled"),
		 QStringLiteral("Mode"), QStringLiteral("State"), QStringLiteral("Bitrate"),
		 QStringLiteral("Dropped"), QStringLiteral("Connect"), QStringLiteral("Health"),
		 QStringLiteral("Uptime"), QStringLiteral("R/E"), QStringLiteral("Data")});
	destinationTree->setRootIsDecorated(false);
	destinationTree->setAlternatingRowColors(true);
	destinationTree->setSelectionMode(QAbstractItemView::SingleSelection);
	destinationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	for (int column = 1; column < 12; column++) {
		destinationTree->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
	}
	layout->addWidget(destinationTree, 1);

	connect(destinationTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
		StoreEditor();
		auto items = destinationTree->selectedItems();
		if (items.empty()) {
			currentIndex = -1;
			SetEditorEnabled(false);
			return;
		}

		currentIndex = items.front()->data(0, Qt::UserRole).toInt();
		LoadEditor(currentIndex);
	});

	auto *editorGroup = new QGroupBox(QStringLiteral("Destination Settings"), root);
	auto *form = new QFormLayout(editorGroup);

	platformCombo = new QComboBox(editorGroup);
	platformCombo->addItems(
		{QStringLiteral("Custom RTMP"), QStringLiteral("Twitch"), QStringLiteral("YouTube"), QStringLiteral("Kick")});
	form->addRow(QStringLiteral("Platform"), platformCombo);

	nameEdit = new QLineEdit(editorGroup);
	form->addRow(QStringLiteral("Name"), nameEdit);

	enabledCheck = new QCheckBox(QStringLiteral("Use this destination when streaming"), editorGroup);
	form->addRow(QString(), enabledCheck);

	encoderModeCombo = new QComboBox(editorGroup);
	encoderModeCombo->addItems({QStringLiteral("Shared Encoder"), QStringLiteral("Independent Encoder")});
	form->addRow(QStringLiteral("Encoder mode"), encoderModeCombo);

	videoBitrateSpin = new QSpinBox(editorGroup);
	videoBitrateSpin->setRange(0, 100000);
	videoBitrateSpin->setSpecialValueText(QStringLiteral("Match primary"));
	videoBitrateSpin->setSuffix(QStringLiteral(" kbps"));
	form->addRow(QStringLiteral("Video bitrate"), videoBitrateSpin);

	audioBitrateSpin = new QSpinBox(editorGroup);
	audioBitrateSpin->setRange(0, 2048);
	audioBitrateSpin->setSpecialValueText(QStringLiteral("Match primary"));
	audioBitrateSpin->setSuffix(QStringLiteral(" kbps"));
	form->addRow(QStringLiteral("Audio bitrate"), audioBitrateSpin);

	outputWidthSpin = new QSpinBox(editorGroup);
	outputWidthSpin->setRange(0, 7680);
	outputWidthSpin->setSpecialValueText(QStringLiteral("Match primary"));
	form->addRow(QStringLiteral("Output width"), outputWidthSpin);

	outputHeightSpin = new QSpinBox(editorGroup);
	outputHeightSpin->setRange(0, 4320);
	outputHeightSpin->setSpecialValueText(QStringLiteral("Match primary"));
	form->addRow(QStringLiteral("Output height"), outputHeightSpin);

	connect(encoderModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
		[this](int) { UpdateEncoderControls(); });

	serverEdit = new QLineEdit(editorGroup);
	serverEdit->setPlaceholderText(QStringLiteral("rtmps://server.example/app"));
	form->addRow(QStringLiteral("Server"), serverEdit);

	keyEdit = new QLineEdit(editorGroup);
	keyEdit->setEchoMode(QLineEdit::Password);
	form->addRow(QStringLiteral("Stream key"), keyEdit);

	showSecretsCheck = new QCheckBox(QStringLiteral("Show stream key/password"), editorGroup);
	form->addRow(QString(), showSecretsCheck);
	connect(showSecretsCheck, &QCheckBox::toggled, this, [this](bool checked) {
		keyEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
		passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
	});

	useAuthCheck = new QCheckBox(QStringLiteral("Server requires username/password"), editorGroup);
	form->addRow(QString(), useAuthCheck);

	usernameEdit = new QLineEdit(editorGroup);
	form->addRow(QStringLiteral("Username"), usernameEdit);

	passwordEdit = new QLineEdit(editorGroup);
	passwordEdit->setEchoMode(QLineEdit::Password);
	form->addRow(QStringLiteral("Password"), passwordEdit);

	reconnectPolicyCombo = new QComboBox(editorGroup);
	reconnectPolicyCombo->addItems(
		{QStringLiteral("Inherit OBS"), QStringLiteral("Always reconnect"), QStringLiteral("Never reconnect")});
	form->addRow(QStringLiteral("Reconnect"), reconnectPolicyCombo);
	connect(reconnectPolicyCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
		[this](int) { UpdateReconnectControls(); });

	maxRetriesSpin = new QSpinBox(editorGroup);
	maxRetriesSpin->setRange(-1, 1000);
	maxRetriesSpin->setSpecialValueText(QStringLiteral("Inherit"));
	form->addRow(QStringLiteral("Max retries"), maxRetriesSpin);

	retryDelaySpin = new QSpinBox(editorGroup);
	retryDelaySpin->setRange(-1, 3600);
	retryDelaySpin->setSpecialValueText(QStringLiteral("Inherit"));
	retryDelaySpin->setSuffix(QStringLiteral(" sec"));
	form->addRow(QStringLiteral("Retry delay"), retryDelaySpin);

	connect(useAuthCheck, &QCheckBox::toggled, this, [this](bool checked) {
		usernameEdit->setEnabled(checked && !main->StreamingActive());
		passwordEdit->setEnabled(checked && !main->StreamingActive());
	});

	layout->addWidget(editorGroup);

	auto *editButtons = new QHBoxLayout();
	addButton = new QPushButton(QStringLiteral("Add"), root);
	removeButton = new QPushButton(QStringLiteral("Remove"), root);
	reloadButton = new QPushButton(QStringLiteral("Reload"), root);
	saveButton = new QPushButton(QStringLiteral("Save"), root);
	editButtons->addWidget(addButton);
	editButtons->addWidget(removeButton);
	editButtons->addStretch(1);
	editButtons->addWidget(reloadButton);
	editButtons->addWidget(saveButton);
	layout->addLayout(editButtons);

	connect(addButton, &QPushButton::clicked, this, [this]() { AddDestination(); });
	connect(removeButton, &QPushButton::clicked, this, [this]() { RemoveSelectedDestination(); });
	connect(reloadButton, &QPushButton::clicked, this, [this]() { ReloadFromManager(); });
	connect(saveButton, &QPushButton::clicked, this, [this]() { SaveChanges(); });

	auto *streamButtons = new QHBoxLayout();
	streamButton = new QPushButton(QStringLiteral("Start Streaming"), root);
	startSelectedButton = new QPushButton(QStringLiteral("Start Selected"), root);
	stopSelectedButton = new QPushButton(QStringLiteral("Stop Selected"), root);
	streamButtons->addWidget(streamButton);
	streamButtons->addWidget(startSelectedButton);
	streamButtons->addWidget(stopSelectedButton);
	layout->addLayout(streamButtons);

	connect(streamButton, &QPushButton::clicked, this, [this]() {
		if (!main->StreamingActive() && !SaveChanges()) {
			return;
		}
		QMetaObject::invokeMethod(main, "StreamActionTriggered", Qt::QueuedConnection);
	});
	connect(startSelectedButton, &QPushButton::clicked, this, [this]() { ToggleSelectedDestination(true); });
	connect(stopSelectedButton, &QPushButton::clicked, this, [this]() { ToggleSelectedDestination(false); });

	setWidget(root);
	SetEditorEnabled(false);
}

void HimotheeMultistreamDock::ReloadFromManager()
{
	auto *manager = main->GetHimotheeMultistreamManager();
	boundManager = manager;

	if (!manager) {
		workingDestinations.clear();
		currentIndex = -1;
		RebuildDestinationTree();
		SetEditorEnabled(false);
		return;
	}

	if (!main->StreamingActive()) {
		manager->Load();
	}

	workingDestinations = manager->Destinations();
	currentIndex = -1;
	RebuildDestinationTree();

	if (!workingDestinations.empty()) {
		destinationTree->setCurrentItem(destinationTree->topLevelItem(0));
	}
}

void HimotheeMultistreamDock::RebuildDestinationTree()
{
	QSignalBlocker blocker(destinationTree);
	destinationTree->clear();

	for (size_t i = 0; i < workingDestinations.size(); i++) {
		const auto &config = workingDestinations[i];
		auto *item = new QTreeWidgetItem(destinationTree);
		item->setData(0, Qt::UserRole, static_cast<int>(i));
		item->setData(0, Qt::UserRole + 1, QString::fromStdString(config.id));
		item->setText(0, QString::fromStdString(config.name));
		item->setText(1, QString::fromStdString(config.platform));
		item->setText(2, config.enabled ? QStringLiteral("On") : QStringLiteral("Off"));
		item->setText(3, config.encoderMode == HimotheeEncoderMode::Independent
					 ? QStringLiteral("Independent")
					 : QStringLiteral("Shared"));
		item->setText(3, config.encoderMode == HimotheeEncoderMode::Independent
					 ? QStringLiteral("Independent")
					 : QStringLiteral("Shared"));
		item->setText(4, config.enabled ? QStringLiteral("Ready") : QStringLiteral("Disabled"));
		item->setText(5, QStringLiteral("0 kbps"));
		item->setText(6, QStringLiteral("0"));
		item->setText(7, QStringLiteral("-"));
		item->setText(8, QStringLiteral("-"));
		item->setText(9, QStringLiteral("0 B"));
	}
}

void HimotheeMultistreamDock::LoadEditor(int index)
{
	if (index < 0 || index >= static_cast<int>(workingDestinations.size())) {
		SetEditorEnabled(false);
		return;
	}

	updatingEditor = true;
	const auto &config = workingDestinations[static_cast<size_t>(index)];

	int platformIndex = platformCombo->findText(QString::fromStdString(config.platform));
	if (platformIndex < 0) {
		platformIndex = 0;
	}
	platformCombo->setCurrentIndex(platformIndex);
	nameEdit->setText(QString::fromStdString(config.name));
	enabledCheck->setChecked(config.enabled);
	encoderModeCombo->setCurrentIndex(config.encoderMode == HimotheeEncoderMode::Independent ? 1 : 0);
	videoBitrateSpin->setValue(config.videoBitrateKbps);
	audioBitrateSpin->setValue(config.audioBitrateKbps);
	outputWidthSpin->setValue(static_cast<int>(config.outputWidth));
	outputHeightSpin->setValue(static_cast<int>(config.outputHeight));
	serverEdit->setText(QString::fromStdString(config.server));
	keyEdit->setText(QString::fromStdString(config.key));
	useAuthCheck->setChecked(config.useAuth);
	usernameEdit->setText(QString::fromStdString(config.username));
	passwordEdit->setText(QString::fromStdString(config.password));
	maxRetriesSpin->setValue(config.maxRetries);
	retryDelaySpin->setValue(config.retryDelaySeconds);

	updatingEditor = false;
	SetEditorEnabled(!main->StreamingActive());
}

void HimotheeMultistreamDock::StoreEditor()
{
	if (updatingEditor || currentIndex < 0 || currentIndex >= static_cast<int>(workingDestinations.size())) {
		return;
	}

	auto &config = workingDestinations[static_cast<size_t>(currentIndex)];
	config.platform = platformCombo->currentText().toStdString();
	config.name = nameEdit->text().trimmed().toStdString();
	config.enabled = enabledCheck->isChecked();
	config.encoderMode =
		encoderModeCombo->currentIndex() == 1 ? HimotheeEncoderMode::Independent : HimotheeEncoderMode::Shared;
	config.videoBitrateKbps = videoBitrateSpin->value();
	config.audioBitrateKbps = audioBitrateSpin->value();
	config.outputWidth = static_cast<uint32_t>(outputWidthSpin->value());
	config.outputHeight = static_cast<uint32_t>(outputHeightSpin->value());
	config.server = serverEdit->text().trimmed().toStdString();
	config.key = keyEdit->text().toStdString();
	config.useAuth = useAuthCheck->isChecked();
	config.username = usernameEdit->text().toStdString();
	config.password = passwordEdit->text().toStdString();
	config.maxRetries = maxRetriesSpin->value();
	config.retryDelaySeconds = retryDelaySpin->value();

	if (auto *item = destinationTree->topLevelItem(currentIndex)) {
		item->setText(0, QString::fromStdString(config.name));
		item->setText(1, QString::fromStdString(config.platform));
		item->setText(2, config.enabled ? QStringLiteral("On") : QStringLiteral("Off"));
	}
}

void HimotheeMultistreamDock::SetEditorEnabled(bool enabled)
{
	platformCombo->setEnabled(enabled);
	nameEdit->setEnabled(enabled);
	enabledCheck->setEnabled(enabled);
	encoderModeCombo->setEnabled(enabled);
	serverEdit->setEnabled(enabled);
	keyEdit->setEnabled(enabled);
	showSecretsCheck->setEnabled(enabled);
	useAuthCheck->setEnabled(enabled);
	usernameEdit->setEnabled(enabled && useAuthCheck->isChecked());
	passwordEdit->setEnabled(enabled && useAuthCheck->isChecked());
	maxRetriesSpin->setEnabled(enabled);
	retryDelaySpin->setEnabled(enabled);
	UpdateEncoderControls();

	const bool canEditList = enabled && !main->StreamingActive();
	addButton->setEnabled(!main->StreamingActive());
	removeButton->setEnabled(canEditList && currentIndex >= 0);
	saveButton->setEnabled(!main->StreamingActive());
	reloadButton->setEnabled(!main->StreamingActive());
}

void HimotheeMultistreamDock::UpdateEncoderControls()
{
	const bool editorAvailable = encoderModeCombo->isEnabled();
	const bool independent = encoderModeCombo->currentIndex() == 1;

	videoBitrateSpin->setEnabled(editorAvailable && independent);
	audioBitrateSpin->setEnabled(editorAvailable && independent);
	outputWidthSpin->setEnabled(editorAvailable && independent);
	outputHeightSpin->setEnabled(editorAvailable && independent);
}

void HimotheeMultistreamDock::AddDestination()
{
	if (main->StreamingActive()) {
		return;
	}

	StoreEditor();

	HimotheeDestinationConfig config;
	config.id = MakeDestinationId();
	config.name = "Destination " + to_string(workingDestinations.size() + 1);
	config.platform = "Custom RTMP";
	config.enabled = true;
	config.encoderMode = HimotheeEncoderMode::Shared;
	config.videoBitrateKbps = 0;
	config.audioBitrateKbps = 0;
	config.outputWidth = 0;
	config.outputHeight = 0;
	config.maxRetries = -1;
	config.retryDelaySeconds = -1;
	workingDestinations.emplace_back(std::move(config));

	RebuildDestinationTree();
	destinationTree->setCurrentItem(destinationTree->topLevelItem(destinationTree->topLevelItemCount() - 1));
}

void HimotheeMultistreamDock::RemoveSelectedDestination()
{
	if (main->StreamingActive() || currentIndex < 0 ||
	    currentIndex >= static_cast<int>(workingDestinations.size())) {
		return;
	}

	const auto answer = QMessageBox::question(
		this, QStringLiteral("Remove Destination"),
		QStringLiteral("Remove '%1' from the multistream list?")
			.arg(QString::fromStdString(workingDestinations[static_cast<size_t>(currentIndex)].name)));
	if (answer != QMessageBox::Yes) {
		return;
	}

	workingDestinations.erase(workingDestinations.begin() + currentIndex);
	currentIndex = -1;
	RebuildDestinationTree();

	if (!workingDestinations.empty()) {
		destinationTree->setCurrentItem(destinationTree->topLevelItem(0));
	} else {
		SetEditorEnabled(false);
	}
}

bool HimotheeMultistreamDock::SaveChanges()
{
	if (main->StreamingActive()) {
		QMessageBox::information(this, QStringLiteral("Multistream"),
					 QStringLiteral("Stop streaming before changing destination settings."));
		return false;
	}

	StoreEditor();

	for (const auto &config : workingDestinations) {
		if (!config.enabled) {
			continue;
		}
		if (config.server.empty()) {
			QMessageBox::warning(this, QStringLiteral("Multistream"),
					     QStringLiteral("Destination '%1' needs an RTMP/RTMPS server.")
						     .arg(QString::fromStdString(config.name)));
			return false;
		}

		if (config.encoderMode == HimotheeEncoderMode::Independent) {
			const bool widthSet = config.outputWidth > 0;
			const bool heightSet = config.outputHeight > 0;
			if (widthSet != heightSet) {
				QMessageBox::warning(
					this, QStringLiteral("Multistream"),
					QStringLiteral("Destination '%1' must set both output width and height, or leave both as Match primary.")
						.arg(QString::fromStdString(config.name)));
				return false;
			}
			if (widthSet && ((config.outputWidth % 2) != 0 || (config.outputHeight % 2) != 0)) {
				QMessageBox::warning(
					this, QStringLiteral("Multistream"),
					QStringLiteral("Destination '%1' needs an even output width and height.")
						.arg(QString::fromStdString(config.name)));
				return false;
			}
		}

		const QString server = QString::fromStdString(config.server);
		if (!server.startsWith(QStringLiteral("rtmp://"), Qt::CaseInsensitive) &&
		    !server.startsWith(QStringLiteral("rtmps://"), Qt::CaseInsensitive)) {
			QMessageBox::warning(this, QStringLiteral("Multistream"),
					     QStringLiteral("Destination '%1' must use an rtmp:// or rtmps:// server.")
						     .arg(QString::fromStdString(config.name)));
			return false;
		}
	}

	auto *manager = main->GetHimotheeMultistreamManager();
	if (!manager || !manager->ReplaceDestinations(workingDestinations)) {
		QMessageBox::warning(this, QStringLiteral("Multistream"),
				     QStringLiteral("Himothee Studio could not save the multistream destinations."));
		return false;
	}

	boundManager = manager;
	workingDestinations = manager->Destinations();
	RebuildDestinationTree();
	if (!workingDestinations.empty()) {
		const int selectIndex = std::clamp(currentIndex, 0, static_cast<int>(workingDestinations.size()) - 1);
		destinationTree->setCurrentItem(destinationTree->topLevelItem(selectIndex));
	}
	return true;
}

void HimotheeMultistreamDock::ToggleSelectedDestination(bool start)
{
	if (!main->StreamingActive() || currentIndex < 0 ||
	    currentIndex >= static_cast<int>(workingDestinations.size())) {
		return;
	}

	auto *manager = main->GetHimotheeMultistreamManager();
	if (!manager) {
		return;
	}

	const string id = workingDestinations[static_cast<size_t>(currentIndex)].id;
	if (start) {
		manager->StartDestination(id);
	} else {
		manager->StopDestination(id);
	}

	RefreshStatus();
}

string HimotheeMultistreamDock::MakeDestinationId() const
{
	for (size_t candidate = 1;; candidate++) {
		const string id = "destination-" + to_string(candidate);
		const bool used = any_of(workingDestinations.begin(), workingDestinations.end(),
					 [&](const auto &config) { return config.id == id; });
		if (!used) {
			return id;
		}
	}
}

void HimotheeMultistreamDock::RefreshStatus()
{
	auto *manager = main->GetHimotheeMultistreamManager();
	if (manager != boundManager) {
		ReloadFromManager();
		manager = boundManager;
	}

	const bool primaryActive = main->StreamingActive();
	streamButton->setText(primaryActive ? QStringLiteral("Stop Streaming") : QStringLiteral("Start Streaming"));

	if (!manager) {
		summaryLabel->setText(QStringLiteral("Multistream engine unavailable"));
		startSelectedButton->setEnabled(false);
		stopSelectedButton->setEnabled(false);
		return;
	}

	unordered_map<string, HimotheeDestinationStatus> statusById;
	for (auto &status : manager->Status()) {
		statusById.emplace(status.id, std::move(status));
	}

	const uint64_t nowMs = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
	int activeCount = 0;
	int independentActiveCount = 0;
	int errorCount = 0;
	int reconnectingCount = 0;
	for (int row = 0; row < destinationTree->topLevelItemCount(); row++) {
		auto *item = destinationTree->topLevelItem(row);
		const string id = item->data(0, Qt::UserRole + 1).toString().toStdString();

		auto it = statusById.find(id);
		if (it == statusById.end()) {
			const int index = item->data(0, Qt::UserRole).toInt();
			if (index >= 0 && index < static_cast<int>(workingDestinations.size()) &&
			    !workingDestinations[static_cast<size_t>(index)].enabled) {
				item->setText(4, QStringLiteral("Disabled"));
			} else {
				item->setText(4, primaryActive ? QStringLiteral("Waiting") : QStringLiteral("Ready"));
			}
			item->setText(5, QStringLiteral("0 kbps"));
			item->setText(6, QStringLiteral("0"));
			item->setText(7, QStringLiteral("-"));
			item->setText(8, QStringLiteral("-"));
			item->setText(9, QStringLiteral("0 B"));
			item->setToolTip(4, QString());
			lastBytesById.erase(id);
			lastSampleMsById.erase(id);
			continue;
		}

		const auto &status = it->second;
		double bitrateKbps = 0.0;
		auto bytesIt = lastBytesById.find(id);
		auto timeIt = lastSampleMsById.find(id);
		if (bytesIt != lastBytesById.end() && timeIt != lastSampleMsById.end() && nowMs > timeIt->second &&
		    status.totalBytes >= bytesIt->second) {
			const uint64_t elapsedMs = nowMs - timeIt->second;
			const uint64_t byteDelta = status.totalBytes - bytesIt->second;
			bitrateKbps = (static_cast<double>(byteDelta) * 8.0) / static_cast<double>(elapsedMs);
		}
		lastBytesById[id] = status.totalBytes;
		lastSampleMsById[id] = nowMs;

		item->setText(3, status.encoderMode == HimotheeEncoderMode::Independent
					 ? QStringLiteral("Independent")
					 : QStringLiteral("Shared"));
		item->setText(4, StateText(status.state));
		item->setText(5, QStringLiteral("%1 kbps").arg(bitrateKbps, 0, 'f', 0));
		item->setText(6, QString::number(status.droppedFrames));
		item->setText(7, status.connectTimeMs >= 0 ? QStringLiteral("%1 ms").arg(status.connectTimeMs)
						       : QStringLiteral("-"));
		item->setText(8, HealthText(status));
		item->setText(9, FormatBytes(status.totalBytes));

		QString tooltip = status.encoderMode == HimotheeEncoderMode::Independent
					  ? QStringLiteral("Mode: Independent Encoder")
					  : QStringLiteral("Mode: Shared Encoder");
		if (!status.videoCodec.empty() || !status.audioCodec.empty()) {
			tooltip += QStringLiteral("\nCodecs: %1 / %2")
					   .arg(QString::fromStdString(status.videoCodec),
						QString::fromStdString(status.audioCodec));
		}
		if (!status.lastError.empty()) {
			tooltip += QStringLiteral("\nLast error: %1").arg(QString::fromStdString(status.lastError));
		}
		item->setToolTip(4, tooltip);

		if (status.state == HimotheeDestinationState::Active ||
		    status.state == HimotheeDestinationState::Reconnecting) {
			activeCount++;
			if (status.encoderMode == HimotheeEncoderMode::Independent) {
				independentActiveCount++;
			}
		}
		if (status.state == HimotheeDestinationState::Error) {
			errorCount++;
		}
		if (status.state == HimotheeDestinationState::Reconnecting) {
			reconnectingCount++;
		}
	}

	summaryLabel->setText(
		QStringLiteral("Primary: %1  |  Secondary live: %2/%3  |  Independent: %4  |  Reconnecting: %5  |  Errors: %6")
			.arg(primaryActive ? QStringLiteral("Live") : QStringLiteral("Stopped"))
			.arg(activeCount)
			.arg(manager->EnabledCount())
			.arg(independentActiveCount)
			.arg(reconnectingCount)
			.arg(errorCount));

	const bool selected = currentIndex >= 0 && currentIndex < static_cast<int>(workingDestinations.size());
	bool selectedActive = false;
	bool selectedCanStart = false;
	if (selected) {
		const string &id = workingDestinations[static_cast<size_t>(currentIndex)].id;
		auto it = statusById.find(id);
		if (it != statusById.end()) {
			selectedActive = it->second.state == HimotheeDestinationState::Active ||
					 it->second.state == HimotheeDestinationState::Reconnecting ||
					 it->second.state == HimotheeDestinationState::Starting;
			selectedCanStart = !selectedActive && it->second.state != HimotheeDestinationState::Stopping;
		}
	}

	startSelectedButton->setEnabled(primaryActive && selected && selectedCanStart &&
					 workingDestinations[static_cast<size_t>(currentIndex)].enabled);
	stopSelectedButton->setEnabled(primaryActive && selected && selectedActive);

	SetEditorEnabled(selected && !primaryActive);
}
