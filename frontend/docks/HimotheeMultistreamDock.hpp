#pragma once

#include <docks/OBSDock.hpp>
#include <utility/HimotheeMultistream.hpp>

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

class HimotheeMultistreamDock : public OBSDock {
public:
	explicit HimotheeMultistreamDock(OBSBasic *main);

	void ReloadFromManager();
	void RefreshStatus();

private:
	OBSBasic *main = nullptr;
	HimotheeMultistreamManager *boundManager = nullptr;

	std::vector<HimotheeDestinationConfig> workingDestinations;
	int currentIndex = -1;
	bool updatingEditor = false;

	QTreeWidget *destinationTree = nullptr;
	QLabel *summaryLabel = nullptr;
	QComboBox *platformCombo = nullptr;
	QLineEdit *nameEdit = nullptr;
	QCheckBox *enabledCheck = nullptr;
	QLineEdit *serverEdit = nullptr;
	QLineEdit *keyEdit = nullptr;
	QCheckBox *showSecretsCheck = nullptr;
	QCheckBox *useAuthCheck = nullptr;
	QLineEdit *usernameEdit = nullptr;
	QLineEdit *passwordEdit = nullptr;
	QSpinBox *maxRetriesSpin = nullptr;
	QSpinBox *retryDelaySpin = nullptr;
	QPushButton *addButton = nullptr;
	QPushButton *removeButton = nullptr;
	QPushButton *saveButton = nullptr;
	QPushButton *reloadButton = nullptr;
	QPushButton *streamButton = nullptr;
	QPushButton *startSelectedButton = nullptr;
	QPushButton *stopSelectedButton = nullptr;
	QTimer *statusTimer = nullptr;

	void BuildUi();
	void RebuildDestinationTree();
	void LoadEditor(int index);
	void StoreEditor();
	void SetEditorEnabled(bool enabled);
	void AddDestination();
	void RemoveSelectedDestination();
	bool SaveChanges();
	void ToggleSelectedDestination(bool start);
	std::string MakeDestinationId() const;
};
