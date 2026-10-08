#include "HimotheeOverlayDock.hpp"

#include <widgets/OBSBasic.hpp>

#include <obs-frontend-api.h>
#include <obs.hpp>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
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

QString TypeText(HimotheeOverlayType)
{
	return QStringLiteral("Text");
}

QString SizeText(const HimotheeOverlayDefinition &overlay)
{
	return QStringLiteral("%1x%2").arg(overlay.width).arg(overlay.height);
}

} // namespace

HimotheeOverlayDock::HimotheeOverlayDock(OBSBasic *main_) : OBSDock(main_), main(main_)
{
	setObjectName(QStringLiteral("himotheeOverlayDock"));
	setWindowTitle(QStringLiteral("Himothee Overlays"));
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	manager = make_unique<HimotheeOverlayManager>(main);
	BuildUi();
	ReloadFromManager();

	refreshTimer = new QTimer(this);
	refreshTimer->setInterval(750);
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
	overlayTree->setColumnCount(4);
	overlayTree->setHeaderLabels({QStringLiteral("Overlay"), QStringLiteral("Type"), QStringLiteral("Visible"),
				     QStringLiteral("Size")});
	overlayTree->setRootIsDecorated(false);
	overlayTree->setAlternatingRowColors(true);
	overlayTree->setSelectionMode(QAbstractItemView::SingleSelection);
	overlayTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	overlayTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	overlayTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	overlayTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
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

	auto *editorGroup = new QGroupBox(QStringLiteral("Overlay Settings"), root);
	auto *form = new QFormLayout(editorGroup);

	nameEdit = new QLineEdit(editorGroup);
	form->addRow(QStringLiteral("Name"), nameEdit);

	visibleCheck = new QCheckBox(QStringLiteral("Overlay visible"), editorGroup);
	form->addRow(QString(), visibleCheck);

	titleEdit = new QLineEdit(editorGroup);
	titleEdit->setPlaceholderText(QStringLiteral("Overlay title"));
	form->addRow(QStringLiteral("Title"), titleEdit);

	textEdit = new QLineEdit(editorGroup);
	textEdit->setPlaceholderText(QStringLiteral("Text/value"));
	form->addRow(QStringLiteral("Text"), textEdit);

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

	layout->addWidget(editorGroup);

	auto *editButtons = new QHBoxLayout();
	addButton = new QPushButton(QStringLiteral("New Overlay"), root);
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

	connect(addButton, &QPushButton::clicked, this, [this]() { AddOverlay(); });
	connect(removeButton, &QPushButton::clicked, this, [this]() { RemoveSelected(); });
	connect(saveButton, &QPushButton::clicked, this, [this]() { SaveChanges(); });
	connect(showHideButton, &QPushButton::clicked, this, [this]() { ToggleSelectedVisibility(); });
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

	const string currentProfilePath = main->GetCurrentProfile().path.u8string();
	if (currentProfilePath != loadedProfilePath) {
		manager->LoadForCurrentProfile();
		ReloadFromManager();
	}

	if (!manager->ServerRunning()) {
		manager->StartServer();
	}

	if (manager->ServerRunning()) {
		serverLabel->setText(QStringLiteral("Overlay server: %1  |  Overlays: %2")
					     .arg(manager->BaseUrl())
					     .arg(manager->Overlays().size()));
	} else {
		serverLabel->setText(QStringLiteral("Overlay server error: %1")
					     .arg(QString::fromStdString(manager->ServerError())));
	}

	const bool selected = currentIndex >= 0 && currentIndex < static_cast<int>(workingOverlays.size());
	if (selected) {
		showHideButton->setText(workingOverlays[static_cast<size_t>(currentIndex)].visible
					    ? QStringLiteral("Hide")
					    : QStringLiteral("Show"));
	}
	previewButton->setEnabled(selected && manager->ServerRunning());
	copyUrlButton->setEnabled(selected && manager->ServerRunning());
	createSourceButton->setEnabled(selected && manager->ServerRunning());
}

void HimotheeOverlayDock::ReloadFromManager()
{
	if (!manager) {
		workingOverlays.clear();
		loadedProfilePath.clear();
		RebuildTree();
		return;
	}

	manager->LoadForCurrentProfile();
	workingOverlays = manager->Overlays();
	loadedProfilePath = main->GetCurrentProfile().path.u8string();
	currentIndex = -1;
	RebuildTree();

	if (!workingOverlays.empty()) {
		overlayTree->setCurrentItem(overlayTree->topLevelItem(0));
	}
	Refresh();
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
		item->setText(1, TypeText(overlay.type));
		item->setText(2, overlay.visible ? QStringLiteral("Yes") : QStringLiteral("No"));
		item->setText(3, SizeText(overlay));
	}
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
	visibleCheck->setChecked(overlay.visible);
	titleEdit->setText(QString::fromStdString(overlay.title));
	textEdit->setText(QString::fromStdString(overlay.text));
	widthSpin->setValue(static_cast<int>(overlay.width));
	heightSpin->setValue(static_cast<int>(overlay.height));
	urlLabel->setText(manager ? manager->OverlayUrl(overlay.id) : QString());
	updatingEditor = false;
	SetEditorEnabled(true);
	Refresh();
}

void HimotheeOverlayDock::StoreEditor()
{
	if (updatingEditor || currentIndex < 0 || currentIndex >= static_cast<int>(workingOverlays.size())) {
		return;
	}

	auto &overlay = workingOverlays[static_cast<size_t>(currentIndex)];
	overlay.name = nameEdit->text().trimmed().toStdString();
	if (overlay.name.empty()) {
		overlay.name = "Overlay";
	}
	overlay.visible = visibleCheck->isChecked();
	overlay.title = titleEdit->text().toStdString();
	overlay.text = textEdit->text().toStdString();
	overlay.width = static_cast<uint32_t>(widthSpin->value());
	overlay.height = static_cast<uint32_t>(heightSpin->value());

	if (auto *item = overlayTree->topLevelItem(currentIndex)) {
		item->setText(0, QString::fromStdString(overlay.name));
		item->setText(2, overlay.visible ? QStringLiteral("Yes") : QStringLiteral("No"));
		item->setText(3, SizeText(overlay));
	}
}

bool HimotheeOverlayDock::SaveChanges()
{
	if (!manager) {
		return false;
	}

	StoreEditor();
	if (!manager->ReplaceOverlays(workingOverlays)) {
		QMessageBox::warning(this, QStringLiteral("Himothee Overlays"),
				     QStringLiteral("Could not save overlay settings for this profile."));
		return false;
	}
	return true;
}

void HimotheeOverlayDock::SetEditorEnabled(bool enabled)
{
	nameEdit->setEnabled(enabled);
	visibleCheck->setEnabled(enabled);
	titleEdit->setEnabled(enabled);
	textEdit->setEnabled(enabled);
	widthSpin->setEnabled(enabled);
	heightSpin->setEnabled(enabled);
	removeButton->setEnabled(enabled);
	saveButton->setEnabled(enabled);
	showHideButton->setEnabled(enabled);
	previewButton->setEnabled(enabled && manager && manager->ServerRunning());
	copyUrlButton->setEnabled(enabled && manager && manager->ServerRunning());
	createSourceButton->setEnabled(enabled && manager && manager->ServerRunning());
}

void HimotheeOverlayDock::AddOverlay()
{
	if (!manager) {
		return;
	}

	StoreEditor();
	const string id = manager->AddOverlay();
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
		this, QStringLiteral("Delete Overlay"),
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

	if (auto *item = overlayTree->topLevelItem(currentIndex)) {
		item->setText(2, overlay.visible ? QStringLiteral("Yes") : QStringLiteral("No"));
	}
	Refresh();
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

	SaveChanges();
	QApplication::clipboard()->setText(manager->OverlayUrl(id));
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

	OBSSceneItemAutoRelease sceneItem = obs_scene_add(scene, source);
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
