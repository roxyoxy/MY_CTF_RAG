#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "embedder.h"
#include "generator.h"
#include "indexer.h"
#include "query_worker.h"
#include "type.h"
#include "vector_index.h"

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

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    // Waits for an in-flight query worker: run() borrows docs_/chunks_/
    // index_ owned by this window, so the members must outlive it.
    ~MainWindow() override;

private:
    void buildUi();
    void connectUi();

    void chooseDataDirectory();
    void loadCorpusAndIndex(const QString& dataDir);
    void embedChunks(const QString& reason);
    void rebuildIndex();
    void deleteSelectedDocument();
    void showSelectedDocument(QTreeWidgetItem* item, int column);
    void openSearchResult(int row, int column);
    void updateDeletedVisibility(bool showDeleted);

    // M4 step 4: every query (retrieval-only or full RAG ask) runs the
    // whole chain off the UI thread (run_query_chain inside a
    // QThread); these handlers only launch and render.
    void startRetrieval();
    void startChatQuery();
    void launchWorker(const QString& query, bool withGeneration);
    void fillResultsTable(const std::vector<SearchResult>& results,
                          bool hybrid,
                          const std::unordered_set<int>& bm25Ids,
                          const std::unordered_set<int>& denseIds);
    void setChatBusy(bool busy);
    void onWorkerGenerationStarted();
    void onWorkerFinished(QueryResultPtr result);
    void onAnswerAnchorClicked(const QUrl& url);
    void onSourceRowClicked(int row, int column);

    void refreshDocumentTree();
    void refreshStatusBar();
    void showDocument(int documentId);
    void highlightChunk(int chunkId);
    void appendLog(const QString& text);

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

    std::vector<Document> docs_;
    std::vector<Chunk> chunks_;
    InvertedIndex index_;

    // M3 steps 4 + 5: dense route (semantic search) with vector.bin
    // caching (path + content hash reuse, T9).
    std::unique_ptr<EmbedProvider> provider_;
    FlatIndex denseIndex_;
    bool denseReady_ = false;
    QString denseStateText_ = QStringLiteral("dense: off");

    std::string snapshotPath_ = "index.bin";
    std::string vectorSnapshotPath_ = "vector.bin";
    QString snapshotStateText_ = QStringLiteral("not loaded");
    int previewDocumentId_ = -1;
    bool corpusLoaded_ = false;

    // M4 step 4: generation provider (factory is zero-network; the
    // first chat call is the health check) plus the in-flight worker
    // bookkeeping. lastResult_ backs the citation clicks ([n] ->
    // used_materials[n-1] -> chunk highlight).
    std::unique_ptr<LLMProvider> generator_;
    QThread* busyThread_ = nullptr;
    QueryResultPtr lastResult_;
    QString lastQuestion_;
    bool chatBusy_ = false;
};
