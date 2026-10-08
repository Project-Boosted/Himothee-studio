#include "HimotheeOverlayDock.hpp"

#include <widgets/OBSBasic.hpp>

#include <obs-frontend-api.h>
#include <obs.hpp>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

using namespace std;

namespace {

QString SizeText(const HimotheeOverlayDefinition &overlay)
{
	return QStringLiteral("%1x%2").arg(overlay.width).arg(overlay.height);
}

QString NormalizeColor(const QString &value, const QString &fallback)
{
	QColor color(value.trimmed());
	if (!color.isValid()) {
		color = QColor(fallback);
	}
	return color.name(QColor::HexRgb).toUpper();
}

QString ThemeDisplayName(const string &theme)
{
	if (theme == "minimal") return QStringLiteral("Minimal");
	if (theme == "neon") return QStringLiteral("Neon");
	if (theme == "transparent") return QStringLiteral("Transparent");
	if (theme == "custom") return QStringLiteral("Custom");
	return QStringLiteral("Himothee Dark");
}

QWidget *MakeFormRow(QWidget *parent, const QString &labelText, QWidget *field)
{
	auto *row = new QWidget(parent);
	auto *layout = new QHBoxLayout(row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);

	auto *label = new QLabel(labelText, row);
	label->setMinimumWidth(105);
	layout->addWidget(label);
	layout->addWidget(field, 1);
	return row;
}

} // namespace

HimotheeOverlayDock::HimotheeOverlayDock(OBSBasic *main_) : OBSDock(main_), main(main_)
{
	setObjectName(QStringLiteral("himotheeOverlayDock"));
	setWindowTitle(QStringLiteral("Himothee Overlays"));
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	manager = make_unique<HimotheeOverlayManager>(main);
	BuildUi();

	// OBSBasic creates docks before OBSInit activates the selected profile.
	// Do not read profile state from the dock constructor.
	serverLabel->setText(QStringLiteral("Overlay engine ready — waiting for OBS profile..."));

	refreshTimer = new QTimer(this);
	refreshTimer->setInterval(250);
	connect(refreshTimer, &QTimer::timeout, this, [this]() { Refresh(); });
	refreshTimer->start();
}

void HimotheeOverlayDock::BuildUi()
{
	auto *root = new QWidget(this);
	auto *layout = new QVBoxLayout(root);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(8);

	serverLabel = new QLabel(QStringLiteral("Overlay server starting..."), root);
	serverLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(serverLabel);

	overlayTree = new QTreeWidget(root);
	overlayTree->setColumnCount(7);
	overlayTree->setHeaderLabels({QStringLiteral("Overlay"), QStringLiteral("Type"), QStringLiteral("Visible"),
				     QStringLiteral("Live"), QStringLiteral("Theme"), QStringLiteral("Position"),
				     QStringLiteral("Size")});
	overlayTree->setRootIsDecorated(false);
	overlayTree->setAlternatingRowColors(true);
	overlayTree->setSelectionMode(QAbstractItemView::SingleSelection);
	overlayTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	for (int column = 1; column < 7; column++) {
		overlayTree->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
	}
	layout->addWidget(overlayTree, 1);

	connect(overlayTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
		StoreEditor();
		const auto items = overlayTree->selectedItems();
		if (items.empty()) {
			currentIndex = -1;
			SetEditorEnabled(false);
			return;
		}
		currentIndex = items.front()->data(0, Qt::UserRole).toInt();
		LoadEditor(currentIndex);
	});

	auto *editorGroup = new QGroupBox(QStringLiteral("Widget Settings"), root);
	auto *form = new QFormLayout(editorGroup);

	nameEdit = new QLineEdit(editorGroup);
	form->addRow(QStringLiteral("Name"), nameEdit);

	typeCombo = new QComboBox(editorGroup);
	typeCombo->addItem(QStringLiteral("Text"), static_cast<int>(HimotheeOverlayType::Text));
	typeCombo->addItem(QStringLiteral("Counter"), static_cast<int>(HimotheeOverlayType::Counter));
	typeCombo->addItem(QStringLiteral("Kill Counter"), static_cast<int>(HimotheeOverlayType::KillCounter));
	typeCombo->addItem(QStringLiteral("Streak Counter"), static_cast<int>(HimotheeOverlayType::StreakCounter));
	typeCombo->addItem(QStringLiteral("Challenge Progress"), static_cast<int>(HimotheeOverlayType::Progress));
	typeCombo->addItem(QStringLiteral("Countdown"), static_cast<int>(HimotheeOverlayType::Countdown));
	typeCombo->addItem(QStringLiteral("Stopwatch"), static_cast<int>(HimotheeOverlayType::Stopwatch));
	typeCombo->addItem(QStringLiteral("Stream Uptime"), static_cast<int>(HimotheeOverlayType::StreamUptime));
	typeCombo->addItem(QStringLiteral("Darts 180 Counter"), static_cast<int>(HimotheeOverlayType::Darts180));
	typeCombo->addItem(QStringLiteral("Darts 140+ Counter"), static_cast<int>(HimotheeOverlayType::Darts140Plus));
	typeCombo->addItem(QStringLiteral("Darts 100+ Counter"), static_cast<int>(HimotheeOverlayType::Darts100Plus));
	typeCombo->addItem(QStringLiteral("Darts Legs Won"), static_cast<int>(HimotheeOverlayType::DartsLegs));
	typeCombo->addItem(QStringLiteral("Darts Match Wins"), static_cast<int>(HimotheeOverlayType::DartsWins));
	typeCombo->addItem(QStringLiteral("Darts Average"), static_cast<int>(HimotheeOverlayType::DartsAverage));
	typeCombo->addItem(QStringLiteral("Darts Checkout"), static_cast<int>(HimotheeOverlayType::DartsCheckout));
	form->addRow(QStringLiteral("Type"), typeCombo);

	visibleCheck = new QCheckBox(QStringLiteral("Overlay visible"), editorGroup);
	form->addRow(QString(), visibleCheck);

	titleEdit = new QLineEdit(editorGroup);
	titleEdit->setPlaceholderText(QStringLiteral("Displayed title"));
	form->addRow(QStringLiteral("Title"), titleEdit);

	textEdit = new QLineEdit(editorGroup);
	textEdit->setPlaceholderText(QStringLiteral("Text/value"));
	textRowWidget = MakeFormRow(editorGroup, QStringLiteral("Text"), textEdit);
	form->addRow(textRowWidget);

	valueSpin = new QSpinBox(editorGroup);
	valueSpin->setRange(-999999, 999999);
	counterRowWidget = MakeFormRow(editorGroup, QStringLiteral("Value"), valueSpin);
	form->addRow(counterRowWidget);

	averageSpin = new QDoubleSpinBox(editorGroup);
	averageSpin->setRange(0.0, 200.0);
	averageSpin->setDecimals(2);
	averageSpin->setSingleStep(0.1);
	averageRowWidget = MakeFormRow(editorGroup, QStringLiteral("Average"), averageSpin);
	form->addRow(averageRowWidget);

	checkoutScoreSpin = new QSpinBox(editorGroup);
	checkoutScoreSpin->setRange(0, 170);
	checkoutScoreRowWidget = MakeFormRow(editorGroup, QStringLiteral("Checkout"), checkoutScoreSpin);
	form->addRow(checkoutScoreRowWidget);

	checkoutRouteEdit = new QLineEdit(editorGroup);
	checkoutRouteEdit->setPlaceholderText(QStringLiteral("e.g. T20 T20 D25"));
	checkoutRouteRowWidget = MakeFormRow(editorGroup, QStringLiteral("Route"), checkoutRouteEdit);
	form->addRow(checkoutRouteRowWidget);

	checkoutDurationSpin = new QSpinBox(editorGroup);
	checkoutDurationSpin->setRange(1, 15);
	checkoutDurationSpin->setValue(3);
	checkoutDurationSpin->setSuffix(QStringLiteral(" sec"));
	checkoutDurationRowWidget = MakeFormRow(editorGroup, QStringLiteral("Show for"), checkoutDurationSpin);
	form->addRow(checkoutDurationRowWidget);

	targetSpin = new QSpinBox(editorGroup);
	targetSpin->setRange(1, 999999);
	targetSpin->setValue(100);
	targetRowWidget = MakeFormRow(editorGroup, QStringLiteral("Target"), targetSpin);
	form->addRow(targetRowWidget);

	durationSecondsSpin = new QSpinBox(editorGroup);
	durationSecondsSpin->setRange(1, 604800);
	durationSecondsSpin->setValue(300);
	durationSecondsSpin->setSuffix(QStringLiteral(" sec"));
	durationRowWidget = MakeFormRow(editorGroup, QStringLiteral("Duration"), durationSecondsSpin);
	form->addRow(durationRowWidget);

	widthSpin = new QSpinBox(editorGroup);
	widthSpin->setRange(320, 7680);
	widthSpin->setSingleStep(10);
	form->addRow(QStringLiteral("Browser width"), widthSpin);

	heightSpin = new QSpinBox(editorGroup);
	heightSpin->setRange(180, 4320);
	heightSpin->setSingleStep(10);
	form->addRow(QStringLiteral("Browser height"), heightSpin);

	urlLabel = new QLabel(editorGroup);
	urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	urlLabel->setWordWrap(true);
	form->addRow(QStringLiteral("Browser URL"), urlLabel);

	runtimeValueLabel = new QLabel(QStringLiteral("-"), editorGroup);
	runtimeValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	form->addRow(QStringLiteral("Live value"), runtimeValueLabel);

	layout->addWidget(editorGroup);

	auto *designerGroup = new QGroupBox(QStringLiteral("Overlay Designer"), root);
	auto *designerForm = new QFormLayout(designerGroup);

	themeCombo = new QComboBox(designerGroup);
	themeCombo->addItem(QStringLiteral("Himothee Dark"), QStringLiteral("himothee_dark"));
	themeCombo->addItem(QStringLiteral("Minimal"), QStringLiteral("minimal"));
	themeCombo->addItem(QStringLiteral("Neon"), QStringLiteral("neon"));
	themeCombo->addItem(QStringLiteral("Transparent"), QStringLiteral("transparent"));
	themeCombo->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
	designerForm->addRow(QStringLiteral("Theme"), themeCombo);

	fontCombo = new QComboBox(designerGroup);
	for (const QString &font : {QStringLiteral("Segoe UI"), QStringLiteral("Arial"), QStringLiteral("Impact"),
				    QStringLiteral("Trebuchet MS"), QStringLiteral("Georgia"), QStringLiteral("Consolas")}) {
		fontCombo->addItem(font, font);
	}
	designerForm->addRow(QStringLiteral("Font"), fontCombo);

	fontSizeSpin = new QSpinBox(designerGroup);
	fontSizeSpin->setRange(12, 240);
	fontSizeSpin->setValue(64);
	designerForm->addRow(QStringLiteral("Value size"), fontSizeSpin);

	textColorEdit = new QLineEdit(designerGroup);
	textColorEdit->setPlaceholderText(QStringLiteral("#FFFFFF"));
	designerForm->addRow(QStringLiteral("Text colour"), textColorEdit);

	backgroundColorEdit = new QLineEdit(designerGroup);
	backgroundColorEdit->setPlaceholderText(QStringLiteral("#0A0C12"));
	designerForm->addRow(QStringLiteral("Background"), backgroundColorEdit);

	backgroundOpacitySpin = new QSpinBox(designerGroup);
	backgroundOpacitySpin->setRange(0, 100);
	backgroundOpacitySpin->setSuffix(QStringLiteral("%"));
	backgroundOpacitySpin->setValue(80);
	designerForm->addRow(QStringLiteral("Background opacity"), backgroundOpacitySpin);

	cornerRadiusSpin = new QSpinBox(designerGroup);
	cornerRadiusSpin->setRange(0, 200);
	cornerRadiusSpin->setSuffix(QStringLiteral(" px"));
	cornerRadiusSpin->setValue(18);
	designerForm->addRow(QStringLiteral("Corner radius"), cornerRadiusSpin);

	positionCombo = new QComboBox(designerGroup);
	for (int i = static_cast<int>(HimotheeOverlayPosition::TopLeft);
	     i <= static_cast<int>(HimotheeOverlayPosition::BottomRight); i++) {
		const auto position = static_cast<HimotheeOverlayPosition>(i);
		positionCombo->addItem(HimotheeOverlayPositionName(position), i);
	}
	designerForm->addRow(QStringLiteral("Position"), positionCombo);

	animationCombo = new QComboBox(designerGroup);
	for (int i = static_cast<int>(HimotheeOverlayAnimation::None);
	     i <= static_cast<int>(HimotheeOverlayAnimation::SlideLeft); i++) {
		const auto animation = static_cast<HimotheeOverlayAnimation>(i);
		animationCombo->addItem(HimotheeOverlayAnimationName(animation), i);
	}
	designerForm->addRow(QStringLiteral("Animation"), animationCombo);

	mediaPathEdit = new QLineEdit(designerGroup);
	mediaPathEdit->setPlaceholderText(QStringLiteral("Optional image, GIF, video, or https:// URL"));
	browseMediaButton = new QPushButton(QStringLiteral("Browse"), designerGroup);
	clearMediaButton = new QPushButton(QStringLiteral("Clear"), designerGroup);
	auto *mediaRow = new QWidget(designerGroup);
	auto *mediaLayout = new QHBoxLayout(mediaRow);
	mediaLayout->setContentsMargins(0, 0, 0, 0);
	mediaLayout->addWidget(mediaPathEdit, 1);
	mediaLayout->addWidget(browseMediaButton);
	mediaLayout->addWidget(clearMediaButton);
	designerForm->addRow(QStringLiteral("Media layer"), mediaRow);

	mediaOpacitySpin = new QSpinBox(designerGroup);
	mediaOpacitySpin->setRange(0, 100);
	mediaOpacitySpin->setValue(100);
	mediaOpacitySpin->setSuffix(QStringLiteral("%"));
	designerForm->addRow(QStringLiteral("Media opacity"), mediaOpacitySpin);

	mediaLoopCheck = new QCheckBox(QStringLiteral("Loop video media"), designerGroup);
	mediaLoopCheck->setChecked(true);
	designerForm->addRow(QString(), mediaLoopCheck);

	layout->addWidget(designerGroup);

	counterControlsWidget = new QWidget(root);
	auto *counterControls = new QHBoxLayout(counterControlsWidget);
	counterControls->setContentsMargins(0, 0, 0, 0);
	counterControls->addWidget(new QLabel(QStringLiteral("Counter"), counterControlsWidget));
	decrementButton = new QPushButton(QStringLiteral("-1"), counterControlsWidget);
	incrementButton = new QPushButton(QStringLiteral("+1"), counterControlsWidget);
	resetCounterButton = new QPushButton(QStringLiteral("Reset"), counterControlsWidget);
	counterControls->addWidget(decrementButton);
	counterControls->addWidget(incrementButton);
	counterControls->addWidget(resetCounterButton);
	counterControls->addStretch(1);
	layout->addWidget(counterControlsWidget);

	timerControlsWidget = new QWidget(root);
	auto *timerControls = new QHBoxLayout(timerControlsWidget);
	timerControls->setContentsMargins(0, 0, 0, 0);
	timerControls->addWidget(new QLabel(QStringLiteral("Timer"), timerControlsWidget));
	startPauseTimerButton = new QPushButton(QStringLiteral("Start"), timerControlsWidget);
	resetTimerButton = new QPushButton(QStringLiteral("Reset"), timerControlsWidget);
	timerControls->addWidget(startPauseTimerButton);
	timerControls->addWidget(resetTimerButton);
	timerControls->addStretch(1);
	layout->addWidget(timerControlsWidget);

	checkoutControlsWidget = new QWidget(root);
	auto *checkoutControls = new QHBoxLayout(checkoutControlsWidget);
	checkoutControls->setContentsMargins(0, 0, 0, 0);
	checkoutControls->addWidget(new QLabel(QStringLiteral("Checkout"), checkoutControlsWidget));
	triggerCheckoutButton = new QPushButton(QStringLiteral("Trigger"), checkoutControlsWidget);
	clearCheckoutButton = new QPushButton(QStringLiteral("Clear"), checkoutControlsWidget);
	checkoutControls->addWidget(triggerCheckoutButton);
	checkoutControls->addWidget(clearCheckoutButton);
	checkoutControls->addStretch(1);
	layout->addWidget(checkoutControlsWidget);

	auto *editButtons = new QHBoxLayout();
	addButton = new QPushButton(QStringLiteral("New Widget"), root);
	removeButton = new QPushButton(QStringLiteral("Delete"), root);
	saveButton = new QPushButton(QStringLiteral("Save"), root);
	editButtons->addWidget(addButton);
	editButtons->addWidget(removeButton);
	editButtons->addStretch(1);
	editButtons->addWidget(saveButton);
	layout->addLayout(editButtons);

	auto *actionButtons = new QHBoxLayout();
	showHideButton = new QPushButton(QStringLiteral("Hide"), root);
	previewButton = new QPushButton(QStringLiteral("Preview"), root);
	copyUrlButton = new QPushButton(QStringLiteral("Copy URL"), root);
	createSourceButton = new QPushButton(QStringLiteral("Create OBS Source"), root);
	actionButtons->addWidget(showHideButton);
	actionButtons->addWidget(previewButton);
	actionButtons->addWidget(copyUrlButton);
	actionButtons->addWidget(createSourceButton);
	layout->addLayout(actionButtons);

	connect(typeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
		if (updatingEditor) {
			return;
		}
		StoreEditor();
		UpdateWidgetControls();
		UpdateTreeRow(currentIndex);
	});
	connect(themeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
		if (updatingEditor) {
			return;
		}
		ApplyThemePreset(themeCombo->currentData().toString());
		StoreEditor();
		UpdateTreeRow(currentIndex);
	});
	connect(positionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
		if (!updatingEditor) {
			StoreEditor();
			UpdateTreeRow(currentIndex);
		}
	});
	connect(browseMediaButton, &QPushButton::clicked, this, [this]() { BrowseMedia(); });
	connect(clearMediaButton, &QPushButton::clicked, this, [this]() { ClearMedia(); });
	connect(addButton, &QPushButton::clicked, this, [this]() { AddOverlay(); });
	connect(removeButton, &QPushButton::clicked, this, [this]() { RemoveSelected(); });
	connect(saveButton, &QPushButton::clicked, this, [this]() { SaveChanges(); });
	connect(showHideButton, &QPushButton::clicked, this, [this]() { ToggleSelectedVisibility(); });
	connect(decrementButton, &QPushButton::clicked, this, [this]() { AdjustSelectedValue(-1); });
	connect(incrementButton, &QPushButton::clicked, this, [this]() { AdjustSelectedValue(1); });
	connect(resetCounterButton, &QPushButton::clicked, this, [this]() { ResetSelectedValue(); });
	connect(startPauseTimerButton, &QPushButton::clicked, this, [this]() { ToggleSelectedTimer(); });
	connect(resetTimerButton, &QPushButton::clicked, this, [this]() { ResetSelectedTimer(); });
	connect(triggerCheckoutButton, &QPushButton::clicked, this, [this]() { TriggerSelectedCheckout(); });
	connect(clearCheckoutButton, &QPushButton::clicked, this, [this]() { ClearSelectedCheckout(); });
	connect(previewButton, &QPushButton::clicked, this, [this]() { PreviewSelected(); });
	connect(copyUrlButton, &QPushButton::clicked, this, [this]() { CopySelectedUrl(); });
	connect(createSourceButton, &QPushButton::clicked, this, [this]() { CreateBrowserSource(); });

	setWidget(root);
	SetEditorEnabled(false);
}

void HimotheeOverlayDock::Refresh()
{
	if (!manager) {
		return;
	}

	if (!manager->ServerRunning()) {
		manager->StartServer();
	}

	if (!manager->LoadForCurrentProfile()) {
		if (manager->ServerRunning()) {
			serverLabel->setText(
				QStringLiteral("Overlay server: %1  |  Waiting for OBS profile...")
					.arg(manager->BaseUrl()));
		} else {
			serverLabel->setText(QStringLiteral("Overlay server error: %1")
						     .arg(QString::fromStdString(manager->ServerError())));
		}
		return;
	}

	if (manager->LoadedProfilePath() != loadedProfilePath) {
		ReloadFromManager();
		return;
	}

	if (manager->ServerRunning()) {
		serverLabel->setText(QStringLiteral("Overlay server: %1  |  Widgets: %2")
					     .arg(manager->BaseUrl())
					     .arg(manager->Overlays().size()));
	} else {
		serverLabel->setText(QStringLiteral("Overlay server error: %1")
					     .arg(QString::fromStdString(manager->ServerError())));
	}

	for (int row = 0; row < overlayTree->topLevelItemCount(); row++) {
		auto *item = overlayTree->topLevelItem(row);
		if (!item) {
			continue;
		}
		const string id = item->data(0, Qt::UserRole + 1).toString().toStdString();
		item->setText(3, manager->RuntimeDisplay(id));
	}

	const bool selected = currentIndex >= 0 && currentIndex < static_cast<int>(workingOverlays.size());
	if (selected) {
		const auto &overlay = workingOverlays[static_cast<size_t>(currentIndex)];
		showHideButton->setText(overlay.visible ? QStringLiteral("Hide") : QStringLiteral("Show"));
		runtimeValueLabel->setText(manager->RuntimeDisplay(overlay.id));

		const auto *liveOverlay = manager->Find(overlay.id);
		if (liveOverlay && HimotheeOverlayIsTimer(liveOverlay->type)) {
			startPauseTimerButton->setText(liveOverlay->running ? QStringLiteral("Pause") : QStringLiteral("Start"));
		}
	}

	previewButton->setEnabled(selected && manager->ServerRunning());
	copyUrlButton->setEnabled(selected && manager->ServerRunning());
	createSourceButton->setEnabled(selected && manager->ServerRunning());
}

void HimotheeOverlayDock::ReloadFromManager()
{
	if (!manager || !manager->LoadForCurrentProfile()) {
		return;
	}

	workingOverlays = manager->Overlays();
	loadedProfilePath = manager->LoadedProfilePath();
	currentIndex = -1;
	RebuildTree();

	if (!workingOverlays.empty()) {
		overlayTree->setCurrentItem(overlayTree->topLevelItem(0));
	} else {
		SetEditorEnabled(false);
	}
}

void HimotheeOverlayDock::SyncWorkingFromManager()
{
	if (!manager) {
		return;
	}

	const string selectedId = SelectedId();
	updatingEditor = true;
	workingOverlays = manager->Overlays();
	currentIndex = -1;
	RebuildTree();
	updatingEditor = false;

	for (int row = 0; row < overlayTree->topLevelItemCount(); row++) {
		auto *item = overlayTree->topLevelItem(row);
		if (item && item->data(0, Qt::UserRole + 1).toString().toStdString() == selectedId) {
			overlayTree->setCurrentItem(item);
			return;
		}
	}

	if (!workingOverlays.empty()) {
		overlayTree->setCurrentItem(overlayTree->topLevelItem(0));
	} else {
		SetEditorEnabled(false);
	}
}

void HimotheeOverlayDock::RebuildTree()
{
	overlayTree->clear();
	for (size_t i = 0; i < workingOverlays.size(); i++) {
		const auto &overlay = workingOverlays[i];
		auto *item = new QTreeWidgetItem(overlayTree);
		item->setData(0, Qt::UserRole, static_cast<int>(i));
		item->setData(0, Qt::UserRole + 1, QString::fromStdString(overlay.id));
		item->setText(0, QString::fromStdString(overlay.name));
		item->setText(1, HimotheeOverlayTypeName(overlay.type));
		item->setText(2, overlay.visible ? QStringLiteral("Yes") : QStringLiteral("No"));
		item->setText(3, manager ? manager->RuntimeDisplay(overlay.id) : QStringLiteral("-"));
		item->setText(4, ThemeDisplayName(overlay.theme));
		item->setText(5, HimotheeOverlayPositionName(overlay.position));
		item->setText(6, SizeText(overlay));
	}
}

void HimotheeOverlayDock::UpdateTreeRow(int index)
{
	if (index < 0 || index >= static_cast<int>(workingOverlays.size())) {
		return;
	}
	auto *item = overlayTree->topLevelItem(index);
	if (!item) {
		return;
	}

	const auto &overlay = workingOverlays[static_cast<size_t>(index)];
	item->setText(0, QString::fromStdString(overlay.name));
	item->setText(1, HimotheeOverlayTypeName(overlay.type));
	item->setText(2, overlay.visible ? QStringLiteral("Yes") : QStringLiteral("No"));
	item->setText(3, manager ? manager->RuntimeDisplay(overlay.id) : QStringLiteral("-"));
	item->setText(4, ThemeDisplayName(overlay.theme));
	item->setText(5, HimotheeOverlayPositionName(overlay.position));
	item->setText(6, SizeText(overlay));
}

void HimotheeOverlayDock::LoadEditor(int index)
{
	if (index < 0 || index >= static_cast<int>(workingOverlays.size())) {
		SetEditorEnabled(false);
		return;
	}

	updatingEditor = true;
	const auto &overlay = workingOverlays[static_cast<size_t>(index)];
	nameEdit->setText(QString::fromStdString(overlay.name));
	const int typeIndex = typeCombo->findData(static_cast<int>(overlay.type));
	typeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
	visibleCheck->setChecked(overlay.visible);
	titleEdit->setText(QString::fromStdString(overlay.title));
	textEdit->setText(QString::fromStdString(overlay.text));
	valueSpin->setValue(static_cast<int>(clamp<int64_t>(overlay.value, -999999, 999999)));
	targetSpin->setValue(static_cast<int>(clamp<int64_t>(overlay.target, 1, 999999)));
	durationSecondsSpin->setValue(
		static_cast<int>(clamp<int64_t>(overlay.durationMs / 1000, 1, 604800)));

	int themeIndex = themeCombo->findData(QString::fromStdString(overlay.theme));
	if (themeIndex < 0) themeIndex = themeCombo->findData(QStringLiteral("custom"));
	themeCombo->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);

	int fontIndex = fontCombo->findData(QString::fromStdString(overlay.fontFamily));
	if (fontIndex < 0 && !overlay.fontFamily.empty()) {
		fontCombo->addItem(QString::fromStdString(overlay.fontFamily), QString::fromStdString(overlay.fontFamily));
		fontIndex = fontCombo->count() - 1;
	}
	fontCombo->setCurrentIndex(fontIndex >= 0 ? fontIndex : 0);
	fontSizeSpin->setValue(static_cast<int>(clamp<uint32_t>(overlay.fontSize, 12, 240)));
	textColorEdit->setText(QString::fromStdString(overlay.textColor));
	backgroundColorEdit->setText(QString::fromStdString(overlay.backgroundColor));
	backgroundOpacitySpin->setValue(static_cast<int>(min<uint32_t>(overlay.backgroundOpacity, 100)));
	cornerRadiusSpin->setValue(static_cast<int>(min<uint32_t>(overlay.cornerRadius, 200)));

	const int positionIndex = positionCombo->findData(static_cast<int>(overlay.position));
	positionCombo->setCurrentIndex(positionIndex >= 0 ? positionIndex : 4);
	const int animationIndex = animationCombo->findData(static_cast<int>(overlay.animation));
	animationCombo->setCurrentIndex(animationIndex >= 0 ? animationIndex : 0);
	mediaPathEdit->setText(QString::fromStdString(overlay.mediaPath));
	mediaOpacitySpin->setValue(static_cast<int>(min<uint32_t>(overlay.mediaOpacity, 100)));
	mediaLoopCheck->setChecked(overlay.mediaLoop);

	widthSpin->setValue(static_cast<int>(overlay.width));
	heightSpin->setValue(static_cast<int>(overlay.height));
	urlLabel->setText(manager ? manager->OverlayUrl(overlay.id) : QString());
	runtimeValueLabel->setText(manager ? manager->RuntimeDisplay(overlay.id) : QStringLiteral("-"));
	updatingEditor = false;
	SetEditorEnabled(true);
	UpdateWidgetControls();
	Refresh();
}

void HimotheeOverlayDock::StoreEditor()
{
	if (updatingEditor || currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return;
	}

	auto &overlay = workingOverlays[static_cast<size_t>(currentIndex)];
	const HimotheeOverlayType previousType = overlay.type;
	const HimotheeOverlayType newType =
		static_cast<HimotheeOverlayType>(typeCombo->currentData().toInt());

	overlay.name = nameEdit->text().trimmed().toStdString();
	if (overlay.name.empty()) {
		overlay.name = "Widget";
	}
	overlay.type = newType;
	overlay.visible = visibleCheck->isChecked();
	overlay.title = titleEdit->text().toStdString();
	overlay.text = textEdit->text().toStdString();
	overlay.value = valueSpin->value();
	overlay.target = targetSpin->value();
	overlay.durationMs = static_cast<int64_t>(durationSecondsSpin->value()) * 1000;
	overlay.theme = themeCombo->currentData().toString().toStdString();
	overlay.fontFamily = fontCombo->currentData().toString().toStdString();
	overlay.fontSize = static_cast<uint32_t>(fontSizeSpin->value());
	overlay.textColor = NormalizeColor(textColorEdit->text(), QStringLiteral("#FFFFFF")).toStdString();
	overlay.backgroundColor =
		NormalizeColor(backgroundColorEdit->text(), QStringLiteral("#0A0C12")).toStdString();
	overlay.backgroundOpacity = static_cast<uint32_t>(backgroundOpacitySpin->value());
	overlay.cornerRadius = static_cast<uint32_t>(cornerRadiusSpin->value());
	overlay.position = static_cast<HimotheeOverlayPosition>(positionCombo->currentData().toInt());
	overlay.animation = static_cast<HimotheeOverlayAnimation>(animationCombo->currentData().toInt());
	overlay.mediaPath = mediaPathEdit->text().trimmed().toStdString();
	overlay.mediaOpacity = static_cast<uint32_t>(mediaOpacitySpin->value());
	overlay.mediaLoop = mediaLoopCheck->isChecked();
	overlay.width = static_cast<uint32_t>(widthSpin->value());
	overlay.height = static_cast<uint32_t>(heightSpin->value());

	if (overlay.type == HimotheeOverlayType::KillCounter || overlay.type == HimotheeOverlayType::StreakCounter ||
	    overlay.type == HimotheeOverlayType::Progress) {
		overlay.value = max<int64_t>(0, overlay.value);
	}

	if (previousType != newType) {
		overlay.running = false;
		overlay.startedAtMs = 0;
		overlay.elapsedMs = 0;
	}

	UpdateTreeRow(currentIndex);
}

bool HimotheeOverlayDock::SaveChanges()
{
	if (!manager) {
		return false;
	}

	StoreEditor();
	if (!manager->ReplaceOverlays(workingOverlays)) {
		QMessageBox::warning(this, QStringLiteral("Himothee Overlays"),
				     QStringLiteral("Could not save widget settings for this profile."));
		return false;
	}

	workingOverlays = manager->Overlays();
	UpdateTreeRow(currentIndex);
	return true;
}

void HimotheeOverlayDock::SetEditorEnabled(bool enabled)
{
	nameEdit->setEnabled(enabled);
	typeCombo->setEnabled(enabled);
	visibleCheck->setEnabled(enabled);
	titleEdit->setEnabled(enabled);
	textEdit->setEnabled(enabled);
	valueSpin->setEnabled(enabled);
	targetSpin->setEnabled(enabled);
	durationSecondsSpin->setEnabled(enabled);
	themeCombo->setEnabled(enabled);
	fontCombo->setEnabled(enabled);
	fontSizeSpin->setEnabled(enabled);
	textColorEdit->setEnabled(enabled);
	backgroundColorEdit->setEnabled(enabled);
	backgroundOpacitySpin->setEnabled(enabled);
	cornerRadiusSpin->setEnabled(enabled);
	positionCombo->setEnabled(enabled);
	animationCombo->setEnabled(enabled);
	mediaPathEdit->setEnabled(enabled);
	browseMediaButton->setEnabled(enabled);
	clearMediaButton->setEnabled(enabled);
	mediaOpacitySpin->setEnabled(enabled);
	mediaLoopCheck->setEnabled(enabled);
	widthSpin->setEnabled(enabled);
	heightSpin->setEnabled(enabled);
	removeButton->setEnabled(enabled);
	saveButton->setEnabled(enabled);
	showHideButton->setEnabled(enabled);
	previewButton->setEnabled(enabled && manager && manager->ServerRunning());
	copyUrlButton->setEnabled(enabled && manager && manager->ServerRunning());
	createSourceButton->setEnabled(enabled && manager && manager->ServerRunning());
	counterControlsWidget->setEnabled(enabled);
	timerControlsWidget->setEnabled(enabled);
	if (enabled) {
		UpdateWidgetControls();
	} else {
		textRowWidget->setVisible(false);
		counterRowWidget->setVisible(false);
		targetRowWidget->setVisible(false);
		durationRowWidget->setVisible(false);
		counterControlsWidget->setVisible(false);
		timerControlsWidget->setVisible(false);
		runtimeValueLabel->setText(QStringLiteral("-"));
	}
}

void HimotheeOverlayDock::UpdateWidgetControls()
{
	if (currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return;
	}

	const auto type = static_cast<HimotheeOverlayType>(typeCombo->currentData().toInt());
	const bool isText = type == HimotheeOverlayType::Text;
	const bool isCounter = HimotheeOverlayIsCounter(type);
	const bool isProgress = type == HimotheeOverlayType::Progress;
	const bool isCountdown = type == HimotheeOverlayType::Countdown;
	const bool isTimer = HimotheeOverlayIsTimer(type);

	textRowWidget->setVisible(isText);
	counterRowWidget->setVisible(isCounter);
	targetRowWidget->setVisible(isProgress);
	durationRowWidget->setVisible(isCountdown);
	counterControlsWidget->setVisible(isCounter);
	timerControlsWidget->setVisible(isTimer);

	const string id = SelectedId();
	const auto *liveOverlay = manager ? manager->Find(id) : nullptr;
	startPauseTimerButton->setText(liveOverlay && liveOverlay->running ? QStringLiteral("Pause")
									 : QStringLiteral("Start"));
}

void HimotheeOverlayDock::ApplyThemePreset(const QString &themeId)
{
	if (themeId == QStringLiteral("minimal")) {
		fontCombo->setCurrentText(QStringLiteral("Arial"));
		fontSizeSpin->setValue(58);
		textColorEdit->setText(QStringLiteral("#FFFFFF"));
		backgroundColorEdit->setText(QStringLiteral("#000000"));
		backgroundOpacitySpin->setValue(45);
		cornerRadiusSpin->setValue(8);
		animationCombo->setCurrentIndex(animationCombo->findData(static_cast<int>(HimotheeOverlayAnimation::Fade)));
	} else if (themeId == QStringLiteral("neon")) {
		fontCombo->setCurrentText(QStringLiteral("Trebuchet MS"));
		fontSizeSpin->setValue(68);
		textColorEdit->setText(QStringLiteral("#7DF9FF"));
		backgroundColorEdit->setText(QStringLiteral("#06070A"));
		backgroundOpacitySpin->setValue(78);
		cornerRadiusSpin->setValue(22);
		animationCombo->setCurrentIndex(animationCombo->findData(static_cast<int>(HimotheeOverlayAnimation::Pop)));
	} else if (themeId == QStringLiteral("transparent")) {
		fontCombo->setCurrentText(QStringLiteral("Segoe UI"));
		fontSizeSpin->setValue(64);
		textColorEdit->setText(QStringLiteral("#FFFFFF"));
		backgroundColorEdit->setText(QStringLiteral("#000000"));
		backgroundOpacitySpin->setValue(0);
		cornerRadiusSpin->setValue(0);
		animationCombo->setCurrentIndex(animationCombo->findData(static_cast<int>(HimotheeOverlayAnimation::Fade)));
	} else if (themeId == QStringLiteral("himothee_dark")) {
		fontCombo->setCurrentText(QStringLiteral("Segoe UI"));
		fontSizeSpin->setValue(64);
		textColorEdit->setText(QStringLiteral("#FFFFFF"));
		backgroundColorEdit->setText(QStringLiteral("#0A0C12"));
		backgroundOpacitySpin->setValue(80);
		cornerRadiusSpin->setValue(18);
		animationCombo->setCurrentIndex(animationCombo->findData(static_cast<int>(HimotheeOverlayAnimation::Pop)));
	}
}

void HimotheeOverlayDock::BrowseMedia()
{
	const QString file = QFileDialog::getOpenFileName(
		this, QStringLiteral("Choose Overlay Media"), QString(),
		QStringLiteral("Overlay Media (*.png *.jpg *.jpeg *.webp *.gif *.mp4 *.webm *.mov *.m4v);;"
			       "Images (*.png *.jpg *.jpeg *.webp *.gif);;"
			       "Video (*.mp4 *.webm *.mov *.m4v);;All Files (*)"));
	if (!file.isEmpty()) {
		mediaPathEdit->setText(file);
		if (themeCombo->currentData().toString() != QStringLiteral("custom")) {
			themeCombo->setCurrentIndex(themeCombo->findData(QStringLiteral("custom")));
		}
	}
}

void HimotheeOverlayDock::ClearMedia()
{
	mediaPathEdit->clear();
}

void HimotheeOverlayDock::AddOverlay()
{
	if (!manager || !SaveChanges()) {
		return;
	}

	const QStringList types = {
		QStringLiteral("Text"),
		QStringLiteral("Counter"),
		QStringLiteral("Kill Counter"),
		QStringLiteral("Streak Counter"),
		QStringLiteral("Challenge Progress"),
		QStringLiteral("Countdown"),
		QStringLiteral("Stopwatch"),
		QStringLiteral("Stream Uptime"),
	};

	bool ok = false;
	const QString selected = QInputDialog::getItem(this, QStringLiteral("New Widget"),
						       QStringLiteral("Widget type"), types, 0, false, &ok);
	if (!ok) {
		return;
	}

	const int index = types.indexOf(selected);
	const HimotheeOverlayType type =
		index >= 0 ? static_cast<HimotheeOverlayType>(index) : HimotheeOverlayType::Text;

	const string id = manager->AddOverlay(type);
	workingOverlays = manager->Overlays();
	RebuildTree();

	for (int row = 0; row < overlayTree->topLevelItemCount(); row++) {
		if (overlayTree->topLevelItem(row)->data(0, Qt::UserRole + 1).toString().toStdString() == id) {
			overlayTree->setCurrentItem(overlayTree->topLevelItem(row));
			break;
		}
	}
}

void HimotheeOverlayDock::RemoveSelected()
{
	const string id = SelectedId();
	if (id.empty() || !manager) {
		return;
	}

	const auto answer = QMessageBox::question(
		this, QStringLiteral("Delete Widget"),
		QStringLiteral("Delete '%1'? Existing browser sources using its URL will stop rendering.")
			.arg(QString::fromStdString(workingOverlays[static_cast<size_t>(currentIndex)].name)));
	if (answer != QMessageBox::Yes) {
		return;
	}

	manager->RemoveOverlay(id);
	workingOverlays = manager->Overlays();
	currentIndex = -1;
	RebuildTree();
	if (!workingOverlays.empty()) {
		overlayTree->setCurrentItem(overlayTree->topLevelItem(0));
	} else {
		SetEditorEnabled(false);
		urlLabel->clear();
	}
}

void HimotheeOverlayDock::ToggleSelectedVisibility()
{
	if (!manager || currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return;
	}

	StoreEditor();
	auto &overlay = workingOverlays[static_cast<size_t>(currentIndex)];
	overlay.visible = !overlay.visible;
	visibleCheck->setChecked(overlay.visible);
	manager->ReplaceOverlays(workingOverlays);
	workingOverlays = manager->Overlays();
	UpdateTreeRow(currentIndex);
	Refresh();
}

void HimotheeOverlayDock::AdjustSelectedValue(int delta)
{
	if (!manager || !SaveChanges()) {
		return;
	}
	const string id = SelectedId();
	if (!id.empty() && manager->AdjustValue(id, delta)) {
		SyncWorkingFromManager();
	}
}

void HimotheeOverlayDock::ResetSelectedValue()
{
	if (!manager || !SaveChanges()) {
		return;
	}
	const string id = SelectedId();
	if (!id.empty() && manager->ResetValue(id)) {
		SyncWorkingFromManager();
	}
}

void HimotheeOverlayDock::ToggleSelectedTimer()
{
	if (!manager || !SaveChanges()) {
		return;
	}
	const string id = SelectedId();
	const auto *overlay = manager->Find(id);
	if (!overlay || !HimotheeOverlayIsTimer(overlay->type)) {
		return;
	}

	const bool ok = overlay->running ? manager->PauseTimer(id) : manager->StartTimer(id);
	if (ok) {
		SyncWorkingFromManager();
	}
}

void HimotheeOverlayDock::ResetSelectedTimer()
{
	if (!manager || !SaveChanges()) {
		return;
	}
	const string id = SelectedId();
	if (!id.empty() && manager->ResetTimer(id)) {
		SyncWorkingFromManager();
	}
}

void HimotheeOverlayDock::PreviewSelected()
{
	if (!SaveChanges()) {
		return;
	}
	const string id = SelectedId();
	if (!id.empty() && manager && manager->ServerRunning()) {
		QDesktopServices::openUrl(QUrl(manager->OverlayUrl(id)));
	}
}

void HimotheeOverlayDock::CopySelectedUrl()
{
	const string id = SelectedId();
	if (id.empty() || !manager || !manager->ServerRunning()) {
		return;
	}

	if (SaveChanges()) {
		QApplication::clipboard()->setText(manager->OverlayUrl(id));
	}
}

void HimotheeOverlayDock::CreateBrowserSource()
{
	if (!SaveChanges() || !manager || !manager->ServerRunning()) {
		return;
	}
	if (currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return;
	}

	const auto &overlay = workingOverlays[static_cast<size_t>(currentIndex)];
	OBSSourceAutoRelease sceneSource = obs_frontend_get_current_scene();
	if (!sceneSource) {
		QMessageBox::warning(this, QStringLiteral("Himothee Overlays"),
				     QStringLiteral("No current OBS scene is available."));
		return;
	}

	obs_scene_t *scene = obs_scene_from_source(sceneSource);
	if (!scene) {
		QMessageBox::warning(this, QStringLiteral("Himothee Overlays"),
				     QStringLiteral("The current OBS source is not a normal scene."));
		return;
	}

	QString baseName = QStringLiteral("Himothee - %1").arg(QString::fromStdString(overlay.name));
	QString sourceName = baseName;
	for (int suffix = 2;; suffix++) {
		OBSSourceAutoRelease existing = obs_get_source_by_name(sourceName.toUtf8().constData());
		if (!existing) {
			break;
		}
		sourceName = QStringLiteral("%1 %2").arg(baseName).arg(suffix);
	}

	OBSDataAutoRelease settings = obs_data_create();
	const QByteArray url = manager->OverlayUrl(overlay.id).toUtf8();
	obs_data_set_string(settings, "url", url.constData());
	obs_data_set_int(settings, "width", overlay.width);
	obs_data_set_int(settings, "height", overlay.height);
	obs_data_set_bool(settings, "is_local_file", false);
	obs_data_set_bool(settings, "shutdown", false);
	obs_data_set_bool(settings, "restart_when_active", false);

	OBSSourceAutoRelease source =
		obs_source_create("browser_source", sourceName.toUtf8().constData(), settings, nullptr);
	if (!source) {
		QMessageBox::warning(
			this, QStringLiteral("Himothee Overlays"),
			QStringLiteral("Could not create an OBS Browser Source. Make sure the OBS Browser plugin is available."));
		return;
	}

	OBSSceneItemAutoRelease sceneItem{obs_scene_add(scene, source)};
	if (!sceneItem) {
		QMessageBox::warning(this, QStringLiteral("Himothee Overlays"),
				     QStringLiteral("Could not add the Browser Source to the current scene."));
		return;
	}

	QMessageBox::information(
		this, QStringLiteral("Himothee Overlays"),
		QStringLiteral("Created Browser Source '%1' in the current scene.").arg(sourceName));
}

string HimotheeOverlayDock::SelectedId() const
{
	if (currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return {};
	}
	return workingOverlays[static_cast<size_t>(currentIndex)].id;
}
