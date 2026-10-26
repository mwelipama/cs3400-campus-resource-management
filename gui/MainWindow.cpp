#include "MainWindow.h"

#include <QApplication>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QFrame>
#include <QScrollArea>

using std::string;
using std::vector;

// ------------------------------------------------------------------ helpers
static QString qs(const string& s) { return QString::fromStdString(s); }

static QWidget* statCard(const QString& title, QLabel*& valueLabel, const QString& accent) {
    auto* card = new QFrame;
    card->setObjectName("statCard");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    valueLabel = new QLabel("0");
    valueLabel->setStyleSheet("font-size:30px; font-weight:700; color:" + accent + ";");
    auto* t = new QLabel(title);
    t->setStyleSheet("font-size:12px; color:#64748b; letter-spacing:0.4px;");
    v->addWidget(valueLabel);
    v->addWidget(t);
    return card;
}

static void styleTable(QTableWidget* table) {
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setHighlightSections(false);
}

static void fillTable(QTableWidget* table, const QStringList& headers,
                      const vector<QStringList>& rows) {
    table->clear();
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(static_cast<int>(rows.size()));
    for (int r = 0; r < static_cast<int>(rows.size()); ++r)
        for (int c = 0; c < headers.size() && c < rows[r].size(); ++c)
            table->setItem(r, c, new QTableWidgetItem(rows[r][c]));
    styleTable(table);
}

// ------------------------------------------------------------------ construction
MainWindow::MainWindow(const string& dataDirectory, QWidget* parent)
: QMainWindow(parent), controller(new GuiController(dataDirectory)) {
    string report;
    bool ok = controller->load(report);

    setWindowTitle("CS3400 Campus Resource & Activity Management System");
    resize(1180, 760);

    auto* central = new QWidget;
    auto* outer = new QHBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // sidebar
    sidebar = new QListWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(230);
    const char* items[] = {"  Dashboard", "  Resources", "  Requests", "  AVL Tree",
                           "  Hash Table", "  Campus Map", "  DS & Algorithms"};
    for (auto* label : items) sidebar->addItem(label);
    outer->addWidget(sidebar);

    auto* right = new QVBoxLayout;
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(0);

    auto* header = new QLabel("Campus Resource & Activity Management System");
    header->setObjectName("appHeader");
    header->setContentsMargins(22, 16, 22, 16);
    right->addWidget(header);

    pages = new QStackedWidget;
    pages->addWidget(buildDashboard());
    pages->addWidget(buildResources());
    pages->addWidget(buildRequests());
    pages->addWidget(buildTree());
    pages->addWidget(buildHash());
    pages->addWidget(buildGraph());
    pages->addWidget(buildAnalysis());
    right->addWidget(pages, 1);

    statusLabel = new QLabel;
    statusLabel->setObjectName("statusBar");
    statusLabel->setContentsMargins(22, 8, 22, 8);
    right->addWidget(statusLabel);

    auto* rightWrap = new QWidget;
    rightWrap->setLayout(right);
    outer->addWidget(rightWrap, 1);

    setCentralWidget(central);

    connect(sidebar, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    connect(sidebar, &QListWidget::currentRowChanged, this, [this](int) {
        refreshDashboard(); refreshResourceTable(); refreshRequests(); refreshGraph();
    });
    sidebar->setCurrentRow(0);

    fillLocationCombos();
    refreshDashboard();
    refreshResourceTable();
    refreshRequests();

    if (!ok)
        status("Note: resources.txt could not be opened in the data folder — starting empty.", true);
    else
        status("Loaded " + QString::number(controller->resourceCount()) + " resources, " +
               QString::number(controller->normalQueueSize()) + " normal and " +
               QString::number(controller->priorityHeapSize()) + " priority requests.");

    setStyleSheet(R"(
        QWidget { font-family: 'Segoe UI', Arial; font-size: 13px; color: #1e293b; }
        QMainWindow, QStackedWidget { background: #f1f5f9; }
        #appHeader { background: #0f172a; color: #f8fafc; font-size: 17px; font-weight: 600; }
        #sidebar { background: #1e293b; border: none; outline: 0; padding-top: 10px; }
        #sidebar::item { color: #cbd5e1; padding: 13px 18px; font-size: 14px; }
        #sidebar::item:selected { background: #2563eb; color: white; }
        #sidebar::item:hover:!selected { background: #334155; }
        #statusBar { background: #e2e8f0; color: #334155; font-size: 12px; }
        #statCard { background: white; border: 1px solid #e2e8f0; border-radius: 10px; }
        QGroupBox { background: white; border: 1px solid #e2e8f0; border-radius: 10px;
                    margin-top: 12px; padding: 14px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #334155; }
        QTableWidget { background: white; border: 1px solid #e2e8f0; border-radius: 8px; gridline-color: #eef2f7; }
        QHeaderView::section { background: #f8fafc; color: #475569; padding: 8px; border: none;
                               border-bottom: 2px solid #e2e8f0; font-weight: 600; }
        QPushButton { background: #2563eb; color: white; border: none; border-radius: 7px;
                      padding: 8px 14px; font-weight: 600; }
        QPushButton:hover { background: #1d4ed8; }
        QPushButton:disabled { background: #94a3b8; }
        QPushButton#ghost { background: #e2e8f0; color: #1e293b; }
        QPushButton#ghost:hover { background: #cbd5e1; }
        QLineEdit, QComboBox, QSpinBox { background: white; border: 1px solid #cbd5e1;
                      border-radius: 6px; padding: 6px 8px; }
        QTextEdit { background: white; border: 1px solid #e2e8f0; border-radius: 8px;
                    font-family: 'Consolas','Courier New',monospace; font-size: 12px; }
    )");
}

void MainWindow::status(const QString& message, bool warn) {
    statusLabel->setText(message);
    statusLabel->setStyleSheet(warn ? "color:#b45309;" : "color:#334155;");
}

// ------------------------------------------------------------------ dashboard
QWidget* MainWindow::buildDashboard() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);

    auto* title = new QLabel("Overview");
    title->setStyleSheet("font-size:20px; font-weight:700;");
    v->addWidget(title);

    auto* grid = new QGridLayout;
    grid->setSpacing(14);
    grid->addWidget(statCard("TOTAL RESOURCES", statTotal, "#2563eb"), 0, 0);
    grid->addWidget(statCard("AVAILABLE", statAvailable, "#16a34a"), 0, 1);
    grid->addWidget(statCard("BORROWED", statBorrowed, "#d97706"), 0, 2);
    grid->addWidget(statCard("UNDER MAINTENANCE", statMaintenance, "#dc2626"), 0, 3);
    grid->addWidget(statCard("BOOKS", statBooks, "#0f172a"), 1, 0);
    grid->addWidget(statCard("STUDY ROOMS", statRooms, "#0f172a"), 1, 1);
    grid->addWidget(statCard("EQUIPMENT", statEquipment, "#0f172a"), 1, 2);
    grid->addWidget(statCard("LOCATIONS", statLocations, "#0f172a"), 1, 3);
    grid->addWidget(statCard("NORMAL QUEUE (FIFO)", statNormalQ, "#2563eb"), 2, 0);
    grid->addWidget(statCard("PRIORITY HEAP", statPriorityQ, "#2563eb"), 2, 1);
    grid->addWidget(statCard("PATHWAYS", statPathways, "#0f172a"), 2, 2);
    v->addLayout(grid);

    auto* note = new QLabel("Every figure above is read live from the program's own data structures "
                            "(linked list, AVL tree, hash table, FIFO queue, priority heap and graph).");
    note->setWordWrap(true);
    note->setStyleSheet("color:#64748b; margin-top:8px;");
    v->addWidget(note);
    v->addStretch();
    return page;
}

void MainWindow::refreshDashboard() {
    if (!statTotal) return;
    statTotal->setText(QString::number(controller->resourceCount()));
    statAvailable->setText(QString::number(controller->countByStatus(AvailabilityStatus::AVAILABLE)));
    statBorrowed->setText(QString::number(controller->countByStatus(AvailabilityStatus::BORROWED)));
    statMaintenance->setText(QString::number(controller->countByStatus(AvailabilityStatus::MAINTENANCE)));
    statBooks->setText(QString::number(controller->countByTypeCode("BOOK")));
    statRooms->setText(QString::number(controller->countByTypeCode("ROOM")));
    statEquipment->setText(QString::number(controller->countByTypeCode("EQUIPMENT")));
    statLocations->setText(QString::number(controller->locationCount()));
    statPathways->setText(QString::number(controller->pathwayCount()));
    statNormalQ->setText(QString::number(controller->normalQueueSize()));
    statPriorityQ->setText(QString::number(controller->priorityHeapSize()));
}

// ------------------------------------------------------------------ resources
QWidget* MainWindow::buildResources() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);

    auto* bar = new QHBoxLayout;
    auto* addBtn = new QPushButton("Add");
    auto* editBtn = new QPushButton("Edit selected");
    auto* removeBtn = new QPushButton("Remove selected");
    auto* undoBtn = new QPushButton("Undo"); undoBtn->setObjectName("ghost");
    auto* saveBtn = new QPushButton("Save to files"); saveBtn->setObjectName("ghost");
    bar->addWidget(addBtn); bar->addWidget(editBtn); bar->addWidget(removeBtn);
    bar->addWidget(undoBtn); bar->addStretch(); bar->addWidget(saveBtn);
    v->addLayout(bar);

    resourceTable = new QTableWidget;
    v->addWidget(resourceTable, 1);

    auto* tools = new QHBoxLayout;
    auto* sortBox = new QGroupBox("Sort (your real insertion / merge / quick sort)");
    auto* sv = new QHBoxLayout(sortBox);
    sortField = new QComboBox; sortField->addItems({"by ID", "by Name", "by Capacity", "by Type"});
    sortAlgo = new QComboBox; sortAlgo->addItems({"Insertion sort", "Merge sort", "Quick sort"});
    auto* sortBtn = new QPushButton("Sort");
    sortTiming = new QLabel("—"); sortTiming->setStyleSheet("color:#64748b;");
    sv->addWidget(sortField); sv->addWidget(sortAlgo); sv->addWidget(sortBtn); sv->addWidget(sortTiming);
    tools->addWidget(sortBox, 1);
    v->addLayout(tools);

    auto* searchBox = new QGroupBox("Search");
    auto* sg = new QGridLayout(searchBox);
    nameSearchBox = new QLineEdit; nameSearchBox->setPlaceholderText("name contains… (linear search)");
    auto* nameBtn = new QPushButton("Linear search by name");
    compareIdBox = new QLineEdit; compareIdBox->setPlaceholderText("resource ID, e.g. B005");
    auto* cmpBtn = new QPushButton("Compare linear / binary / AVL / hash");
    sg->addWidget(nameSearchBox, 0, 0); sg->addWidget(nameBtn, 0, 1);
    sg->addWidget(compareIdBox, 1, 0); sg->addWidget(cmpBtn, 1, 1);
    v->addWidget(searchBox);

    searchOutput = new QTextEdit; searchOutput->setReadOnly(true); searchOutput->setFixedHeight(130);
    v->addWidget(searchOutput);

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddResource);
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::onEditResource);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveResource);
    connect(undoBtn, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(sortBtn, &QPushButton::clicked, this, &MainWindow::onSortResources);
    connect(nameBtn, &QPushButton::clicked, this, &MainWindow::onNameSearch);
    connect(cmpBtn, &QPushButton::clicked, this, &MainWindow::onCompareSearch);
    return page;
}

void MainWindow::refreshResourceTable() {
    if (!resourceTable) return;
    vector<QStringList> rows;
    for (const auto& r : controller->listResources())
        rows.push_back({qs(r.id), qs(r.name), qs(r.type), qs(r.location),
                        QString::number(r.capacity), qs(r.status),
                        qs(r.extraLabel) + ": " + qs(r.extraValue)});
    fillTable(resourceTable, {"ID", "Name", "Type", "Location", "Capacity", "Status", "Detail"}, rows);
    refreshDashboard();
}

void MainWindow::onAddResource() {
    QStringList types{"BOOK", "ROOM", "EQUIPMENT"};
    bool ok = false;
    QString type = QInputDialog::getItem(this, "Add resource", "Type:", types, 0, false, &ok);
    if (!ok) return;
    QString id = QInputDialog::getText(this, "Add resource", "ID (e.g. B010):", QLineEdit::Normal, "", &ok);
    if (!ok || id.trimmed().isEmpty()) return;
    QString name = QInputDialog::getText(this, "Add resource", "Name:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString loc = QInputDialog::getText(this, "Add resource", "Location:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    int cap = QInputDialog::getInt(this, "Add resource", "Capacity:", 1, 0, 100000, 1, &ok);
    if (!ok) return;
    QStringList statuses{"Available", "Borrowed", "Maintenance"};
    QString st = QInputDialog::getItem(this, "Add resource", "Status:", statuses, 0, false, &ok);
    if (!ok) return;
    QString extraLabel = type == "BOOK" ? "Author" : type == "ROOM" ? "Floor number" : "Condition";
    QString extra = QInputDialog::getText(this, "Add resource", extraLabel + ":", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    AvailabilityStatus availStatus = st == "Available" ? AvailabilityStatus::AVAILABLE
                              : st == "Borrowed" ? AvailabilityStatus::BORROWED
                              : AvailabilityStatus::MAINTENANCE;
    string message;
    bool done = controller->addResource(type.toStdString(), id.toStdString(), name.toStdString(),
                                        loc.toStdString(), cap, availStatus, extra.toStdString(),
                                        controller->resourceCount(), message);
    refreshResourceTable();
    status(qs(message), !done);
}

void MainWindow::onEditResource() {
    int row = resourceTable->currentRow();
    if (row < 0) { status("Select a resource row first.", true); return; }
    QString id = resourceTable->item(row, 0)->text();
    bool ok = false;
    QString name = QInputDialog::getText(this, "Edit " + id, "New name (blank = keep):",
                                         QLineEdit::Normal, "", &ok); if (!ok) return;
    QString loc = QInputDialog::getText(this, "Edit " + id, "New location (blank = keep):",
                                        QLineEdit::Normal, "", &ok); if (!ok) return;
    QString capText = QInputDialog::getText(this, "Edit " + id, "New capacity (blank = keep):",
                                            QLineEdit::Normal, "", &ok); if (!ok) return;
    QStringList statuses{"(keep)", "Available", "Borrowed", "Maintenance"};
    QString st = QInputDialog::getItem(this, "Edit " + id, "New status:", statuses, 0, false, &ok);
    if (!ok) return;
    QString extra = QInputDialog::getText(this, "Edit " + id, "New detail (blank = keep):",
                                          QLineEdit::Normal, "", &ok); if (!ok) return;

    int cap = -1; if (!capText.trimmed().isEmpty()) cap = capText.toInt();
    int statusChoice = statuses.indexOf(st); // 0 keep, 1 avail, 2 borrowed, 3 maint
    string message;
    bool done = controller->updateResource(id.toStdString(), name.toStdString(), loc.toStdString(),
                                           cap, statusChoice, extra.toStdString(), message);
    refreshResourceTable();
    status(qs(message), !done);
}

void MainWindow::onRemoveResource() {
    int row = resourceTable->currentRow();
    if (row < 0) { status("Select a resource row first.", true); return; }
    QString id = resourceTable->item(row, 0)->text();
    if (QMessageBox::question(this, "Remove", "Remove resource " + id + "?") != QMessageBox::Yes) return;
    string message;
    bool done = controller->removeResource(id.toStdString(), message);
    refreshResourceTable();
    status(qs(message), !done);
}

void MainWindow::onUndo() {
    string message;
    controller->undo(message);
    refreshResourceTable();
    status(qs(message) + "  (undo depth: " + QString::number(controller->undoDepth()) + ")");
}

void MainWindow::onSortResources() {
    long long micros = 0;
    auto rows = controller->sortedResources(sortField->currentIndex(), sortAlgo->currentIndex() + 1, micros);
    vector<QStringList> out;
    for (const auto& r : rows)
        out.push_back({qs(r.id), qs(r.name), qs(r.type), qs(r.location),
                       QString::number(r.capacity), qs(r.status),
                       qs(r.extraLabel) + ": " + qs(r.extraValue)});
    fillTable(resourceTable, {"ID", "Name", "Type", "Location", "Capacity", "Status", "Detail"}, out);
    sortTiming->setText(QString("sorted %1 items in %2 µs").arg(rows.size()).arg(micros));
    status("Sorted with " + sortAlgo->currentText() + " (view only — the stored order is unchanged).");
}

void MainWindow::onNameSearch() {
    int examined = 0;
    auto rows = controller->searchByName(nameSearchBox->text().toStdString(), examined);
    QString out = QString("Linear search examined %1 resource(s) [O(n)] — %2 match(es):\n")
                      .arg(examined).arg(rows.size());
    for (const auto& r : rows) out += "  " + qs(r.id) + "  " + qs(r.name) + "  (" + qs(r.status) + ")\n";
    searchOutput->setPlainText(out);
}

void MainWindow::onCompareSearch() {
    QString id = compareIdBox->text().trimmed();
    if (id.isEmpty()) { status("Enter a resource ID to compare.", true); return; }
    bool found = false;
    auto rows = controller->compareSearch(id.toStdString(), found);
    QString out = QString("Looking up %1 with four methods:\n\n").arg(id.toUpper());
    out += QString("  %1 %2 %3 %4\n").arg("Method", -20).arg("Found", -7).arg("Comparisons", -13).arg("Complexity");
    for (const auto& c : rows)
        out += QString("  %1 %2 %3 %4\n").arg(qs(c.method), -20).arg(c.found ? "yes" : "no", -7)
                   .arg(QString::number(c.comparisons), -13).arg(qs(c.complexity));
    searchOutput->setPlainText(out);
}

// ------------------------------------------------------------------ requests
QWidget* MainWindow::buildRequests() {
    auto* page = new QWidget;
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget; scroll->setWidget(inner);
    auto* v = new QVBoxLayout(inner);
    v->setContentsMargins(22, 22, 22, 22);

    auto* submitBox = new QGroupBox("Submit a request");
    auto* sg = new QGridLayout(submitBox);
    reqUser = new QLineEdit; reqUser->setPlaceholderText("user name");
    reqResource = new QLineEdit; reqResource->setPlaceholderText("resource ID (e.g. R101)");
    reqPriority = new QComboBox;
    reqPriority->addItems({"Normal queue (FIFO)", "Emergency (heap)", "High (heap)", "Normal (heap)", "Low (heap)"});
    auto* submitBtn = new QPushButton("Submit");
    sg->addWidget(new QLabel("User"), 0, 0); sg->addWidget(reqUser, 0, 1);
    sg->addWidget(new QLabel("Resource"), 0, 2); sg->addWidget(reqResource, 0, 3);
    sg->addWidget(new QLabel("Route"), 1, 0); sg->addWidget(reqPriority, 1, 1);
    sg->addWidget(submitBtn, 1, 3);
    v->addWidget(submitBox);

    auto* nBox = new QGroupBox("Normal requests — FIFO queue");
    auto* nv = new QVBoxLayout(nBox);
    normalTable = new QTableWidget; nv->addWidget(normalTable);
    auto* nbar = new QHBoxLayout;
    auto* pnBtn = new QPushButton("Process next (dequeue)");
    auto* paBtn = new QPushButton("Process all");
    cancelIdBox = new QLineEdit; cancelIdBox->setPlaceholderText("request # to cancel"); cancelIdBox->setFixedWidth(150);
    auto* cBtn = new QPushButton("Cancel"); cBtn->setObjectName("ghost");
    nbar->addWidget(pnBtn); nbar->addWidget(paBtn); nbar->addStretch();
    nbar->addWidget(cancelIdBox); nbar->addWidget(cBtn);
    nv->addLayout(nbar);
    v->addWidget(nBox);

    auto* pBox = new QGroupBox("Priority requests — binary min-heap (most urgent first)");
    auto* pv = new QVBoxLayout(pBox);
    priorityTable = new QTableWidget; pv->addWidget(priorityTable);
    auto* pbar = new QHBoxLayout;
    auto* ppBtn = new QPushButton("Process highest (extract-min)");
    auto* ppaBtn = new QPushButton("Process all by priority");
    returnIdBox = new QLineEdit; returnIdBox->setPlaceholderText("resource ID to return"); returnIdBox->setFixedWidth(170);
    auto* rBtn = new QPushButton("Return resource"); rBtn->setObjectName("ghost");
    pbar->addWidget(ppBtn); pbar->addWidget(ppaBtn); pbar->addStretch();
    pbar->addWidget(returnIdBox); pbar->addWidget(rBtn);
    pv->addLayout(pbar);
    v->addWidget(pBox);

    auto* hBox = new QGroupBox("Processed history");
    auto* hv = new QVBoxLayout(hBox);
    historyTable = new QTableWidget; hv->addWidget(historyTable);
    v->addWidget(hBox);

    requestOutput = new QTextEdit; requestOutput->setReadOnly(true); requestOutput->setFixedHeight(120);
    v->addWidget(requestOutput);

    auto* outerV = new QVBoxLayout(page);
    outerV->setContentsMargins(0, 0, 0, 0);
    outerV->addWidget(scroll);

    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onSubmitRequest);
    connect(pnBtn, &QPushButton::clicked, this, &MainWindow::onProcessNextNormal);
    connect(paBtn, &QPushButton::clicked, this, &MainWindow::onProcessAllNormal);
    connect(ppBtn, &QPushButton::clicked, this, &MainWindow::onProcessNextPriority);
    connect(ppaBtn, &QPushButton::clicked, this, &MainWindow::onProcessAllPriority);
    connect(cBtn, &QPushButton::clicked, this, &MainWindow::onCancelNormal);
    connect(rBtn, &QPushButton::clicked, this, &MainWindow::onReturnResource);
    return page;
}

void MainWindow::refreshRequests() {
    if (!normalTable) return;
    vector<QStringList> n, p, h;
    for (const auto& r : controller->normalQueueRows())
        n.push_back({QString::number(r.id), qs(r.user), qs(r.resourceId), qs(r.priority), qs(r.status)});
    for (const auto& r : controller->priorityRows())
        p.push_back({QString::number(r.id), qs(r.user), qs(r.resourceId), qs(r.priority), qs(r.status)});
    for (const auto& r : controller->historyRows())
        h.push_back({QString::number(r.id), qs(r.user), qs(r.resourceId), qs(r.priority), qs(r.status)});
    fillTable(normalTable, {"Req #", "User", "Resource", "Priority", "Status"}, n);
    fillTable(priorityTable, {"Req #", "User", "Resource", "Priority", "Status"}, p);
    fillTable(historyTable, {"Req #", "User", "Resource", "Priority", "Status"}, h);
    refreshDashboard();
}

void MainWindow::onSubmitRequest() {
    QString user = reqUser->text().trimmed(), res = reqResource->text().trimmed();
    if (user.isEmpty() || res.isEmpty()) { status("Enter a user and resource ID.", true); return; }
    string message;
    int idx = reqPriority->currentIndex();
    if (idx == 0) controller->submitNormal(user.toStdString(), res.toStdString(), message);
    else {
        PriorityLevel level = idx == 1 ? PriorityLevel::EMERGENCY : idx == 2 ? PriorityLevel::HIGH
                            : idx == 3 ? PriorityLevel::NORMAL : PriorityLevel::LOW;
        controller->submitPriority(user.toStdString(), res.toStdString(), level, message);
    }
    reqUser->clear(); reqResource->clear();
    refreshRequests();
    status(qs(message));
}

void MainWindow::onProcessNextNormal() {
    string outcome; controller->processNextNormal(outcome);
    requestOutput->append(qs(outcome)); refreshRequests(); refreshResourceTable();
}
void MainWindow::onProcessAllNormal() {
    vector<string> outs; int n = controller->processAllNormal(outs);
    for (auto& o : outs) requestOutput->append(qs(o));
    refreshRequests(); refreshResourceTable();
    status(QString("Processed %1 normal request(s).").arg(n));
}
void MainWindow::onProcessNextPriority() {
    string outcome; controller->processNextPriority(outcome);
    requestOutput->append(qs(outcome)); refreshRequests(); refreshResourceTable();
}
void MainWindow::onProcessAllPriority() {
    vector<string> outs; int n = controller->processAllPriority(outs);
    for (auto& o : outs) requestOutput->append(qs(o));
    refreshRequests(); refreshResourceTable();
    status(QString("Processed %1 priority request(s).").arg(n));
}
void MainWindow::onCancelNormal() {
    bool ok = false; int id = cancelIdBox->text().toInt(&ok);
    if (!ok) { status("Enter a numeric request number.", true); return; }
    string message; bool done = controller->cancelNormal(id, message);
    cancelIdBox->clear(); refreshRequests(); status(qs(message), !done);
}
void MainWindow::onReturnResource() {
    QString id = returnIdBox->text().trimmed();
    if (id.isEmpty()) { status("Enter a resource ID to return.", true); return; }
    string message; bool done = controller->returnResource(id.toStdString(), message);
    returnIdBox->clear(); refreshRequests(); refreshResourceTable(); status(qs(message), !done);
}

// ------------------------------------------------------------------ AVL tree
QWidget* MainWindow::buildTree() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);
    auto* title = new QLabel("AVL Tree (self-balancing BST on resource ID)");
    title->setStyleSheet("font-size:18px; font-weight:700;");
    v->addWidget(title);
    treeInfo = new QLabel("—"); treeInfo->setStyleSheet("color:#475569;"); v->addWidget(treeInfo);

    auto* bar = new QHBoxLayout;
    auto* travBtn = new QPushButton("Show four traversals");
    treeIdBox = new QLineEdit; treeIdBox->setPlaceholderText("search ID, e.g. B003"); treeIdBox->setFixedWidth(200);
    auto* searchBtn = new QPushButton("Search tree");
    bar->addWidget(travBtn); bar->addStretch(); bar->addWidget(treeIdBox); bar->addWidget(searchBtn);
    v->addLayout(bar);

    treeOutput = new QTextEdit; treeOutput->setReadOnly(true); v->addWidget(treeOutput, 1);
    connect(travBtn, &QPushButton::clicked, this, &MainWindow::onShowTraversals);
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onTreeSearch);
    return page;
}

void MainWindow::onShowTraversals() {
    auto join = [](const vector<string>& ids) {
        QString s; for (auto& id : ids) s += qs(id) + " "; return s;
    };
    QString out;
    out += "Inorder   (sorted): " + join(controller->traversal(0)) + "\n\n";
    out += "Preorder          : " + join(controller->traversal(1)) + "\n\n";
    out += "Postorder         : " + join(controller->traversal(2)) + "\n\n";
    out += "Level-order (BFS) : " + join(controller->traversal(3)) + "\n";
    treeOutput->setPlainText(out);
    treeInfo->setText(QString("Nodes: %1    Height: %2    (AVL keeps height ≈ log₂n)")
                          .arg(controller->treeSize()).arg(controller->treeHeight()));
}

void MainWindow::onTreeSearch() {
    QString id = treeIdBox->text().trimmed();
    if (id.isEmpty()) return;
    int comparisons = 0; ResourceRow row;
    bool found = controller->treeSearch(id.toStdString(), comparisons, row);
    if (found)
        treeOutput->setPlainText(QString("Found %1 — %2 (%3) after %4 comparison(s).")
            .arg(qs(row.id), qs(row.name), qs(row.status)).arg(comparisons));
    else
        treeOutput->setPlainText(QString("%1 is not in the tree (%2 comparison(s)).")
            .arg(id.toUpper()).arg(comparisons));
}

// ------------------------------------------------------------------ hash
QWidget* MainWindow::buildHash() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);
    auto* title = new QLabel("Hash Table (djb2 + separate chaining)");
    title->setStyleSheet("font-size:18px; font-weight:700;");
    v->addWidget(title);
    auto* btn = new QPushButton("Show hash statistics");
    v->addWidget(btn, 0, Qt::AlignLeft);
    hashOutput = new QTextEdit; hashOutput->setReadOnly(true); v->addWidget(hashOutput, 1);
    connect(btn, &QPushButton::clicked, this, &MainWindow::onHashStats);
    return page;
}

void MainWindow::onHashStats() {
    hashOutput->setPlainText(qs(controller->hashStatistics()));
}

// ------------------------------------------------------------------ graph
QWidget* MainWindow::buildGraph() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);
    auto* title = new QLabel("Campus Navigation (weighted graph)");
    title->setStyleSheet("font-size:18px; font-weight:700;");
    v->addWidget(title);

    auto* box = new QGroupBox("Operations");
    auto* g = new QGridLayout(box);
    fromLoc = new QComboBox; toLoc = new QComboBox; startLoc = new QComboBox;
    auto* spBtn = new QPushButton("Shortest path (Dijkstra)");
    auto* reachBtn = new QPushButton("Reachable?");
    auto* bfsBtn = new QPushButton("BFS from");
    auto* dfsBtn = new QPushButton("DFS from");
    auto* compBtn = new QPushButton("Connected components");
    g->addWidget(new QLabel("From"), 0, 0); g->addWidget(fromLoc, 0, 1);
    g->addWidget(new QLabel("To"), 0, 2); g->addWidget(toLoc, 0, 3);
    g->addWidget(spBtn, 0, 4); g->addWidget(reachBtn, 0, 5);
    g->addWidget(new QLabel("Start"), 1, 0); g->addWidget(startLoc, 1, 1);
    g->addWidget(bfsBtn, 1, 2); g->addWidget(dfsBtn, 1, 3); g->addWidget(compBtn, 1, 4);
    v->addWidget(box);

    graphResult = new QTextEdit; graphResult->setReadOnly(true); graphResult->setFixedHeight(120);
    v->addWidget(graphResult);
    graphOutput = new QTextEdit; graphOutput->setReadOnly(true); v->addWidget(graphOutput, 1);

    connect(spBtn, &QPushButton::clicked, this, &MainWindow::onShortestPath);
    connect(reachBtn, &QPushButton::clicked, this, &MainWindow::onReachable);
    connect(bfsBtn, &QPushButton::clicked, this, &MainWindow::onBfs);
    connect(dfsBtn, &QPushButton::clicked, this, &MainWindow::onDfs);
    connect(compBtn, &QPushButton::clicked, this, &MainWindow::onComponents);
    return page;
}

void MainWindow::fillLocationCombos() {
    if (!fromLoc) return;
    QStringList names;
    for (const auto& n : controller->locationList()) names << qs(n);
    for (auto* combo : {fromLoc, toLoc, startLoc}) { combo->clear(); combo->addItems(names); }
    if (toLoc->count() > 1) toLoc->setCurrentIndex(1);
}

void MainWindow::refreshGraph() {
    if (!graphOutput) return;
    if (fromLoc && fromLoc->count() == 0) fillLocationCombos();
    graphOutput->setPlainText(qs(controller->graphDescription()));
}

void MainWindow::onShortestPath() {
    vector<string> path;
    int metres = controller->shortestPath(fromLoc->currentIndex(), toLoc->currentIndex(), path);
    if (metres < 0) { graphResult->setPlainText(toLoc->currentText() + " is not reachable from " + fromLoc->currentText() + "."); return; }
    QString route; for (size_t i = 0; i < path.size(); ++i) { if (i) route += " → "; route += qs(path[i]); }
    graphResult->setPlainText(QString("Shortest path (%1 m):\n  %2").arg(metres).arg(route));
}
void MainWindow::onReachable() {
    bool ok = controller->reachable(fromLoc->currentIndex(), toLoc->currentIndex());
    graphResult->setPlainText(QString("%1 is %2reachable from %3.")
        .arg(toLoc->currentText(), ok ? "" : "NOT ", fromLoc->currentText()));
}
void MainWindow::onBfs() {
    QString out = "BFS from " + startLoc->currentText() + ":\n  ";
    for (const auto& n : controller->bfs(startLoc->currentIndex())) out += qs(n) + " → ";
    if (out.endsWith(" → ")) out.chop(3);
    graphResult->setPlainText(out);
}
void MainWindow::onDfs() {
    QString out = "DFS from " + startLoc->currentText() + ":\n  ";
    for (const auto& n : controller->dfs(startLoc->currentIndex())) out += qs(n) + " → ";
    if (out.endsWith(" → ")) out.chop(3);
    graphResult->setPlainText(out);
}
void MainWindow::onComponents() {
    graphResult->setPlainText(qs(controller->connectedComponents()));
}

// ------------------------------------------------------------------ analysis
QWidget* MainWindow::buildAnalysis() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(22, 22, 22, 22);
    auto* title = new QLabel("Data Structures & Algorithms — live benchmark");
    title->setStyleSheet("font-size:18px; font-weight:700;");
    v->addWidget(title);
    auto* desc = new QLabel("Runs your real insertion / merge / quick sort and linear / binary / AVL / hash "
                            "lookups over generated catalogues of 1,000, 5,000 and 10,000 items. "
                            "Times are measured at run time (microseconds).");
    desc->setWordWrap(true); desc->setStyleSheet("color:#64748b;"); v->addWidget(desc);
    auto* btn = new QPushButton("Run benchmark");
    v->addWidget(btn, 0, Qt::AlignLeft);
    benchTable = new QTableWidget; v->addWidget(benchTable);
    benchNote = new QTextEdit; benchNote->setReadOnly(true); benchNote->setFixedHeight(90); v->addWidget(benchNote);
    connect(btn, &QPushButton::clicked, this, &MainWindow::onRunBenchmark);
    return page;
}

void MainWindow::onRunBenchmark() {
    status("Running benchmark…");
    QApplication::processEvents();
    auto rows = controller->runBenchmark();
    vector<QStringList> out;
    for (const auto& r : rows)
        out.push_back({QString::number(r.n), QString::number(r.insertion), QString::number(r.merge),
                       QString::number(r.quick), QString::number(r.linear), QString::number(r.binary),
                       QString::number(r.bst), QString::number(r.hash)});
    fillTable(benchTable, {"n", "Insertion", "Merge", "Quick", "Linear ×1000", "Binary ×1000",
                           "AVL ×1000", "Hash ×1000"}, out);
    benchNote->setPlainText("Observation: insertion sort grows ~n², merge and quick ~n log n, "
                            "linear search ~n, while binary, AVL and hash lookups stay almost flat.");
    status("Benchmark complete — timings measured on this machine.");
}

void MainWindow::onSave() {
    string report;
    bool ok = controller->save(report);
    status(qs(report), !ok);
}
