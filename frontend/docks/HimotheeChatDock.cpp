#include "HimotheeChatDock.hpp"

#include <chat/HimotheeChat.hpp>

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

using namespace std;

namespace {

HimotheeChatPlatform PlatformForTab(int index)
{
	switch (index) {
	case 1:
		return HimotheeChatPlatform::Twitch;
	case 2:
		return HimotheeChatPlatform::YouTube;
	case 3:
		return HimotheeChatPlatform::Kick;
	default:
		return HimotheeChatPlatform::All;
	}
}

QString PlatformBadge(HimotheeChatPlatform platform)
{
	switch (platform) {
	case HimotheeChatPlatform::Twitch:
		return QStringLiteral("TWITCH");
	case HimotheeChatPlatform::YouTube:
		return QStringLiteral("YOUTUBE");
	case HimotheeChatPlatform::Kick:
		return QStringLiteral("KICK");
	case HimotheeChatPlatform::System:
		return QStringLiteral("SYSTEM");
	case HimotheeChatPlatform::All:
		break;
	}
	return QStringLiteral("CHAT");
}

} // namespace

HimotheeChatDock::HimotheeChatDock(QWidget *parent, HimotheeChatManager *manager_)
	: OBSDock(QStringLiteral("Himothee Chat"), parent),
	  manager(manager_)
{
	setObjectName(QStringLiteral("himotheeChatDock"));
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	BuildUi();

	refreshTimer = new QTimer(this);
	refreshTimer->setInterval(250);
	connect(refreshTimer, &QTimer::timeout, this, [this]() { Refresh(); });
	refreshTimer->start();

	Refresh();
}

void HimotheeChatDock::BuildUi()
{
	auto *root = new QWidget(this);
	auto *layout = new QVBoxLayout(root);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(6);

	connectionLabel = new QLabel(QStringLiteral("Twitch: Offline  |  YouTube: Offline  |  Kick: Offline"), root);
	connectionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(connectionLabel);

	auto *twitchGroup = new QGroupBox(QStringLiteral("Twitch Chat"), root);
	auto *twitchLayout = new QHBoxLayout(twitchGroup);

	twitchChannelModeCombo = new QComboBox(twitchGroup);
	twitchChannelModeCombo->addItem(QStringLiteral("Auto — My Channel"), 0);
	twitchChannelModeCombo->addItem(QStringLiteral("Custom Channel"), 1);
	twitchLayout->addWidget(twitchChannelModeCombo);

	twitchChannelEdit = new QLineEdit(twitchGroup);
	twitchChannelEdit->setPlaceholderText(QStringLiteral("channel login"));
	twitchChannelEdit->setVisible(false);
	twitchLayout->addWidget(twitchChannelEdit, 1);

	twitchConnectButton = new QPushButton(QStringLiteral("Connect Twitch"), twitchGroup);
	twitchLayout->addWidget(twitchConnectButton);
	layout->addWidget(twitchGroup);

	twitchErrorLabel = new QLabel(root);
	twitchErrorLabel->setWordWrap(true);
	twitchErrorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	twitchErrorLabel->setVisible(false);
	layout->addWidget(twitchErrorLabel);

	connect(twitchChannelModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		twitchChannelEdit->setVisible(index == 1);
	});
	connect(twitchConnectButton, &QPushButton::clicked, this, [this]() {
		if (!manager) {
			return;
		}

		const auto status = manager->ProviderStatus(HimotheeChatPlatform::Twitch);
		if (status.state == HimotheeChatConnectionState::Connected ||
		    status.state == HimotheeChatConnectionState::Connecting ||
		    status.state == HimotheeChatConnectionState::Reconnecting) {
			manager->DisconnectProvider(HimotheeChatPlatform::Twitch);
			return;
		}

		const string channelOverride =
			twitchChannelModeCombo->currentIndex() == 1 ? twitchChannelEdit->text().trimmed().toStdString() : "";
		manager->SetChannelOverride(HimotheeChatPlatform::Twitch, channelOverride);
		manager->ConnectProvider(HimotheeChatPlatform::Twitch);
	});

	filterTabs = new QTabBar(root);
	filterTabs->setExpanding(true);
	filterTabs->addTab(QStringLiteral("All"));
	filterTabs->addTab(QStringLiteral("Twitch"));
	filterTabs->addTab(QStringLiteral("YouTube"));
	filterTabs->addTab(QStringLiteral("Kick"));
	layout->addWidget(filterTabs);
	connect(filterTabs, &QTabBar::currentChanged, this, [this](int) { RebuildTimeline(); });

	messageTree = new QTreeWidget(root);
	messageTree->setColumnCount(4);
	messageTree->setHeaderLabels(
		{QStringLiteral("Platform"), QStringLiteral("User"), QStringLiteral("Message"), QStringLiteral("Time")});
	messageTree->setRootIsDecorated(false);
	messageTree->setAlternatingRowColors(true);
	messageTree->setUniformRowHeights(true);
	messageTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	messageTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	messageTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
	messageTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	layout->addWidget(messageTree, 1);

	auto *composer = new QHBoxLayout();
	sendTargetCombo = new QComboBox(root);
	sendTargetCombo->addItem(QStringLiteral("All"), static_cast<int>(HimotheeChatPlatform::All));
	sendTargetCombo->addItem(QStringLiteral("Twitch"), static_cast<int>(HimotheeChatPlatform::Twitch));
	sendTargetCombo->addItem(QStringLiteral("YouTube"), static_cast<int>(HimotheeChatPlatform::YouTube));
	sendTargetCombo->addItem(QStringLiteral("Kick"), static_cast<int>(HimotheeChatPlatform::Kick));
	composer->addWidget(sendTargetCombo);

	messageEdit = new QLineEdit(root);
	messageEdit->setPlaceholderText(QStringLiteral("Type a message..."));
	composer->addWidget(messageEdit, 1);

	sendButton = new QPushButton(QStringLiteral("Send"), root);
	composer->addWidget(sendButton);

	clearButton = new QPushButton(QStringLiteral("Clear"), root);
	composer->addWidget(clearButton);

	layout->addLayout(composer);

	connect(messageEdit, &QLineEdit::returnPressed, this, [this]() {
		if (sendButton->isEnabled()) {
			sendButton->click();
		}
	});

	connect(sendButton, &QPushButton::clicked, this, [this]() {
		if (!manager) {
			return;
		}

		const QString text = messageEdit->text().trimmed();
		if (text.isEmpty()) {
			return;
		}

		const auto target = static_cast<HimotheeChatPlatform>(sendTargetCombo->currentData().toInt());
		if (manager->SendMessage(target, text.toStdString())) {
			messageEdit->clear();
		}
	});

	connect(clearButton, &QPushButton::clicked, this, [this]() {
		if (manager) {
			manager->ClearMessages();
		}
		lastSequence = 0;
		RebuildTimeline();
	});

	setWidget(root);
}

void HimotheeChatDock::Refresh()
{
	UpdateConnectionSummary();
	UpdateTwitchControls();
	UpdateComposerState();

	if (!manager) {
		return;
	}

	const auto messages = manager->Snapshot(PlatformForTab(CurrentFilterIndex()));
	const uint64_t newestSequence = messages.empty() ? 0 : messages.back().sequence;

	if (newestSequence != lastSequence || lastRenderedFilter != CurrentFilterIndex()) {
		RebuildTimeline();
	}
}

void HimotheeChatDock::RebuildTimeline()
{
	messageTree->clear();
	lastSequence = 0;
	lastRenderedFilter = CurrentFilterIndex();

	if (!manager) {
		return;
	}

	const auto messages = manager->Snapshot(PlatformForTab(CurrentFilterIndex()));
	for (const auto &message : messages) {
		auto *item = new QTreeWidgetItem(messageTree);
		item->setText(0, PlatformBadge(message.platform));
		item->setText(1, QString::fromStdString(message.displayName));
		item->setText(2, QString::fromStdString(message.text));
		item->setText(
			3, QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(message.timestampMs)).toString(QStringLiteral("HH:mm:ss")));

		QString badges;
		if (message.broadcaster) {
			badges += QStringLiteral("Broadcaster");
		}
		if (message.moderator) {
			if (!badges.isEmpty())
				badges += QStringLiteral(", ");
			badges += QStringLiteral("Moderator");
		}
		if (message.subscriber) {
			if (!badges.isEmpty())
				badges += QStringLiteral(", ");
			badges += QStringLiteral("Subscriber");
		}
		if (!badges.isEmpty()) {
			item->setToolTip(1, badges);
		}

		lastSequence = message.sequence;
	}

	messageTree->scrollToBottom();
}

void HimotheeChatDock::UpdateConnectionSummary()
{
	if (!manager) {
		connectionLabel->setText(QStringLiteral("Unified chat manager unavailable"));
		return;
	}

	QString twitch = QStringLiteral("Offline");
	QString youtube = QStringLiteral("Offline");
	QString kick = QStringLiteral("Offline");

	for (const auto &status : manager->ProviderStatuses()) {
		QString label = QString::fromUtf8(HimotheeChatConnectionStateName(status.state));
		if (!status.accountName.empty()) {
			label += QStringLiteral(" (%1").arg(QString::fromStdString(status.accountName));
			if (!status.channelName.empty() && status.channelName != status.accountName) {
				label += QStringLiteral(" → #%1").arg(QString::fromStdString(status.channelName));
			}
			label += QStringLiteral(")");
		} else if (!status.channelName.empty()) {
			label += QStringLiteral(" (#%1)").arg(QString::fromStdString(status.channelName));
		}

		switch (status.platform) {
		case HimotheeChatPlatform::Twitch:
			twitch = label;
			break;
		case HimotheeChatPlatform::YouTube:
			youtube = label;
			break;
		case HimotheeChatPlatform::Kick:
			kick = label;
			break;
		default:
			break;
		}
	}

	connectionLabel->setText(
		QStringLiteral("Twitch: %1  |  YouTube: %2  |  Kick: %3").arg(twitch, youtube, kick));
}

void HimotheeChatDock::UpdateTwitchControls()
{
	if (!manager) {
		return;
	}

	const auto status = manager->ProviderStatus(HimotheeChatPlatform::Twitch);
	const bool busy = status.state == HimotheeChatConnectionState::Connecting ||
			  status.state == HimotheeChatConnectionState::Reconnecting;
	const bool connected = status.state == HimotheeChatConnectionState::Connected;

	if (status.automaticChannel) {
		if (twitchChannelModeCombo->currentIndex() != 0) {
			twitchChannelModeCombo->setCurrentIndex(0);
		}
	} else {
		if (twitchChannelModeCombo->currentIndex() != 1) {
			twitchChannelModeCombo->setCurrentIndex(1);
		}
		if (!status.channelName.empty() && twitchChannelEdit->text().isEmpty()) {
			twitchChannelEdit->setText(QString::fromStdString(status.channelName));
		}
	}

	twitchChannelModeCombo->setEnabled(!busy && !connected);
	twitchChannelEdit->setEnabled(!busy && !connected);
	twitchChannelEdit->setVisible(twitchChannelModeCombo->currentIndex() == 1);
	twitchConnectButton->setText(connected || busy ? QStringLiteral("Disconnect Twitch")
						       : QStringLiteral("Connect Twitch"));

	if (!status.lastError.empty()) {
		twitchErrorLabel->setText(QStringLiteral("Twitch: %1").arg(QString::fromStdString(status.lastError)));
		twitchErrorLabel->setVisible(true);
	} else {
		twitchErrorLabel->clear();
		twitchErrorLabel->setVisible(false);
	}
}

void HimotheeChatDock::UpdateComposerState()
{
	const bool enabled = manager && manager->AnyConnected();
	sendTargetCombo->setEnabled(enabled);
	messageEdit->setEnabled(enabled);
	sendButton->setEnabled(enabled && !messageEdit->text().trimmed().isEmpty());
	messageEdit->setPlaceholderText(enabled ? QStringLiteral("Type a message...")
						    : QStringLiteral("Connect a chat provider to send messages"));
}

int HimotheeChatDock::CurrentFilterIndex() const
{
	return filterTabs ? filterTabs->currentIndex() : 0;
}
