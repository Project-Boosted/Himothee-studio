#pragma once

#include <docks/OBSDock.hpp>
#include <overlays/HimotheeOverlayEngine.hpp>

#include <memory>
#include <string>
#include <vector>

class OBSBasic;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;
class QTreeWidget;
class QWidget;

class HimotheeOverlayDock : public OBSDock {
public:
	explicit HimotheeOverlayDock(OBSBasic *main);

private:
	OBSBasic *main = nullptr;
	std::unique_ptr<HimotheeOverlayManager> manager;
	std::vector<HimotheeOverlayDefinition> workingOverlays;
	std::string loadedProfilePath;
	int currentIndex = -1;
	bool updatingEditor = false;

	QLabel *serverLabel = nullptr;
	QTreeWidget *overlayTree = nullptr;
	QLineEdit *nameEdit = nullptr;
	QComboBox *typeCombo = nullptr;
	QCheckBox *visibleCheck = nullptr;
	QLineEdit *titleEdit = nullptr;
	QLineEdit *textEdit = nullptr;

	QComboBox *themeCombo = nullptr;
	QComboBox *fontCombo = nullptr;
	QSpinBox *fontSizeSpin = nullptr;
	QLineEdit *textColorEdit = nullptr;
	QLineEdit *backgroundColorEdit = nullptr;
	QSpinBox *backgroundOpacitySpin = nullptr;
	QSpinBox *cornerRadiusSpin = nullptr;
	QComboBox *positionCombo = nullptr;
	QComboBox *animationCombo = nullptr;
	QLineEdit *mediaPathEdit = nullptr;
	QPushButton *browseMediaButton = nullptr;
	QPushButton *clearMediaButton = nullptr;
	QSpinBox *mediaOpacitySpin = nullptr;
	QCheckBox *mediaLoopCheck = nullptr;

	QSpinBox *valueSpin = nullptr;
	QSpinBox *targetSpin = nullptr;
	QSpinBox *durationSecondsSpin = nullptr;
	QSpinBox *widthSpin = nullptr;
	QSpinBox *heightSpin = nullptr;
	QLabel *urlLabel = nullptr;
	QLabel *runtimeValueLabel = nullptr;

	QWidget *textRowWidget = nullptr;
	QWidget *counterRowWidget = nullptr;
	QWidget *targetRowWidget = nullptr;
	QWidget *durationRowWidget = nullptr;
	QWidget *counterControlsWidget = nullptr;
	QWidget *timerControlsWidget = nullptr;

	QPushButton *decrementButton = nullptr;
	QPushButton *incrementButton = nullptr;
	QPushButton *resetCounterButton = nullptr;
	QPushButton *startPauseTimerButton = nullptr;
	QPushButton *resetTimerButton = nullptr;

	QPushButton *addButton = nullptr;
	QPushButton *removeButton = nullptr;
	QPushButton *saveButton = nullptr;
	QPushButton *showHideButton = nullptr;
	QPushButton *previewButton = nullptr;
	QPushButton *copyUrlButton = nullptr;
	QPushButton *createSourceButton = nullptr;
	QTimer *refreshTimer = nullptr;

	void BuildUi();
	void Refresh();
	void ReloadFromManager();
	void SyncWorkingFromManager();
	void RebuildTree();
	void UpdateTreeRow(int index);
	void LoadEditor(int index);
	void StoreEditor();
	bool SaveChanges();
	void SetEditorEnabled(bool enabled);
	void UpdateWidgetControls();
	void ApplyThemePreset(const QString &themeId);
	void BrowseMedia();
	void ClearMedia();

	void AddOverlay();
	void RemoveSelected();
	void ToggleSelectedVisibility();
	void AdjustSelectedValue(int delta);
	void ResetSelectedValue();
	void ToggleSelectedTimer();
	void ResetSelectedTimer();
	void PreviewSelected();
	void CopySelectedUrl();
	void CreateBrowserSource();
	std::string SelectedId() const;
};
