#pragma once

#include <docks/OBSDock.hpp>
#include <overlays/HimotheeOverlayEngine.hpp>

#include <memory>
#include <string>
#include <vector>

class OBSBasic;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;
class QTreeWidget;

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
	QCheckBox *visibleCheck = nullptr;
	QLineEdit *titleEdit = nullptr;
	QLineEdit *textEdit = nullptr;
	QSpinBox *widthSpin = nullptr;
	QSpinBox *heightSpin = nullptr;
	QLabel *urlLabel = nullptr;

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
	void RebuildTree();
	void LoadEditor(int index);
	void StoreEditor();
	bool SaveChanges();
	void SetEditorEnabled(bool enabled);
	void AddOverlay();
	void RemoveSelected();
	void ToggleSelectedVisibility();
	void PreviewSelected();
	void CopySelectedUrl();
	void CreateBrowserSource();
	std::string SelectedId() const;
};
