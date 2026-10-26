#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "GuiController.h"

class QListWidget;
class QStackedWidget;
class QTableWidget;
class QLabel;
class QTextEdit;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;

// The main application window: a left navigation sidebar and a stacked set of
// pages, each driving the GuiController (which wraps the real data structures).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const std::string& dataDirectory, QWidget* parent = nullptr);

private slots:
    void refreshDashboard();
    void refreshResourceTable();
    void onAddResource();
    void onEditResource();
    void onRemoveResource();
    void onUndo();
    void onSortResources();
    void onNameSearch();
    void onCompareSearch();
    void refreshRequests();
    void onSubmitRequest();
    void onProcessNextNormal();
    void onProcessAllNormal();
    void onProcessNextPriority();
    void onProcessAllPriority();
    void onCancelNormal();
    void onReturnResource();
    void onShowTraversals();
    void onTreeSearch();
    void onHashStats();
    void refreshGraph();
    void onShortestPath();
    void onReachable();
    void onBfs();
    void onDfs();
    void onComponents();
    void onRunBenchmark();
    void onSave();

private:
    QWidget* buildDashboard();
    QWidget* buildResources();
    QWidget* buildRequests();
    QWidget* buildTree();
    QWidget* buildHash();
    QWidget* buildGraph();
    QWidget* buildAnalysis();
    void status(const QString& message, bool warn = false);
    void fillLocationCombos();

    std::unique_ptr<GuiController> controller;

    QListWidget*    sidebar = nullptr;
    QStackedWidget* pages = nullptr;
    QLabel*         statusLabel = nullptr;

    // dashboard
    QLabel* statTotal = nullptr; QLabel* statAvailable = nullptr; QLabel* statBorrowed = nullptr;
    QLabel* statMaintenance = nullptr; QLabel* statBooks = nullptr; QLabel* statRooms = nullptr;
    QLabel* statEquipment = nullptr; QLabel* statNormalQ = nullptr; QLabel* statPriorityQ = nullptr;
    QLabel* statLocations = nullptr; QLabel* statPathways = nullptr;

    // resources
    QTableWidget* resourceTable = nullptr;
    QComboBox* sortField = nullptr; QComboBox* sortAlgo = nullptr; QLabel* sortTiming = nullptr;
    QLineEdit* nameSearchBox = nullptr; QLineEdit* compareIdBox = nullptr;
    QTextEdit* searchOutput = nullptr;

    // requests
    QTableWidget* normalTable = nullptr; QTableWidget* priorityTable = nullptr; QTableWidget* historyTable = nullptr;
    QLineEdit* reqUser = nullptr; QLineEdit* reqResource = nullptr; QComboBox* reqPriority = nullptr;
    QLineEdit* cancelIdBox = nullptr; QLineEdit* returnIdBox = nullptr;
    QTextEdit* requestOutput = nullptr;

    // tree / hash
    QTextEdit* treeOutput = nullptr; QLineEdit* treeIdBox = nullptr; QLabel* treeInfo = nullptr;
    QTextEdit* hashOutput = nullptr;

    // graph
    QTextEdit* graphOutput = nullptr; QComboBox* fromLoc = nullptr; QComboBox* toLoc = nullptr;
    QComboBox* startLoc = nullptr; QTextEdit* graphResult = nullptr;

    // analysis
    QTableWidget* benchTable = nullptr; QTextEdit* benchNote = nullptr;
};

#endif
