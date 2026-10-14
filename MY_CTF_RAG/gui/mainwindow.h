#pragma once

#include <string>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "engine.h"
#include "query_worker.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTextBrowser;
class QThread;
class QTreeWidget;
class QTreeWidgetItem;
class QUrl;

// The Qt front end.
//
// All retrieval, generation and cache policy now lives in RagEngine
// (engine.h) -- this window owns no pipeline state of its own, only the
// widgets and one UI-side copy of the document list. The contract's
// "one thread at a time" rule is upheld by the busy flag: a query
// worker runs while the window refuses corpus mutations, and the
// startup path blocks input behind a modal progress dialog.
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    // Waits for an in-flight query worker: the worker calls into
    // engine_, which this window owns and must therefore outlive it.
    ~MainWindow() override;

private:
    void buildUi();
    void connectUi();

    void chooseDataDirectory();
    // Load the corpus and bring all three sides up. Runs on the UI
    // thread behind a modal progress dialog, which is what stops the
    // freeze the old synchronous embedChunks() caused -- and, being
    // modal, also removes the re-entrancy the old code allowed.
    void startup();
    void rebuildIndex();
    void deleteSelectedDocument();
    void showSelectedDocument(QTreeWidgetItem* item, int column);
    void openSearchResult(int row, int column);
    void updateDeletedVisibility(bool showDeleted);

    // Every query (retrieval-only or full RAG ask) runs the engine's
    // two phases inside a QueryThread; these handlers only launch and
    // render.
    void startRetrieval();
    void startChatQuery();
    void launchWorker(const QString& query, bool withGeneration);
    void fillResultsTable(const QueryOutcome& outcome);
    void setChatBusy(bool busy);
    void onWorkerGenerationStarted();
    void onWorkerFinished(QueryResultPtr result);
    void onAnswerAnchorClicked(const QUrl& url);
    void onSourceRowClicked(int row, int column);

    void refreshDocumentTree();
    void refreshStatusBar();
    void showDocument(int documentId);
    void highlightChunk(const ChunkLocation& location);
    void appendLog(const QString& text);

    // Startup and mutation reporting, reconstructed from the engine's
    // status enums. The engine returns data; wording is ours.
    void appendStartupReport(const EngineStatus& status);
    void appendDenseReport(const EngineStatus& status, const QString& when);

    QLineEdit* dataDirEdit_ = nullptr;
    QPushButton* browseButton_ = nullptr;
    QPushButton* reloadButton_ = nullptr;
    QPushButton* rebuildButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;

    QLineEdit* queryEdit_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QCheckBox* showDeletedCheck_ = nullptr;

    QTextBrowser* answerBrowser_ = nullptr;
    QTableWidget* sourcesTable_ = nullptr;
    QLineEdit* askEdit_ = nullptr;
    QPushButton* askButton_ = nullptr;
    QLabel* chatStageLabel_ = nullptr;

    QTreeWidget* documentTree_ = nullptr;
    QPlainTextEdit* previewEdit_ = nullptr;
    QTableWidget* resultsTable_ = nullptr;
    QPlainTextEdit* logEdit_ = nullptr;

    QLabel* docsLabel_ = nullptr;
    QLabel* chunksLabel_ = nullptr;
    QLabel* denseLabel_ = nullptr;
    QLabel* snapshotLabel_ = nullptr;
    QLabel* versionsLabel_ = nullptr;
    QLabel* avgdlLabel_ = nullptr;

    RagEngine engine_;
    EngineStatus status_;
    bool engineReady_ = false;

    // UI-side snapshot of the document list, refreshed by
    // refreshDocumentTree() whenever the corpus changes. The engine is
    // the source of truth; this exists so the tree, the result filter
    // and the deleted-visibility toggle have something to read.
    std::vector<DocInfo> docs_;

    int previewDocumentId_ = -1;

    QThread* busyThread_ = nullptr;
    QueryResultPtr lastResult_;
    QString lastQuestion_;
    bool chatBusy_ = false;
};
