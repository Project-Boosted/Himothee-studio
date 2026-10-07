#pragma once

#include <docks/OBSDock.hpp>

#include <cstdint>

class HimotheeChatManager;
class QLabel;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTabBar;
class QTimer;
class QTreeWidget;

class HimotheeChatDock : public OBSDock {
public:
	HimotheeChatDock(QWidget *parent, HimotheeChatManager *manager);

private:
	HimotheeChatManager *manager = nullptr;

	QLabel *connectionLabel = nullptr;
	QTabBar *filterTabs = nullptr;
	QTreeWidget *messageTree = nullptr;
	QComboBox *sendTargetCombo = nullptr;
	QLineEdit *messageEdit = nullptr;
	QPushButton *sendButton = nullptr;
	QPushButton *clearButton = nullptr;
	QTimer *refreshTimer = nullptr;

	uint64_t lastSequence = 0;
	int lastRenderedFilter = -1;

	void BuildUi();
	void Refresh();
	void RebuildTimeline();
	void UpdateConnectionSummary();
	void UpdateComposerState();

	int CurrentFilterIndex() const;
};
