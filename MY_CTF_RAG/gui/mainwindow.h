#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "embedder.h"
#include "indexer.h"
#include "type.h"
#include "vector_index.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void connectUi();

    void chooseDataDirectory();
    void loadCorpusAndIndex(const QString& dataDir);
    void embedChunks(const QString& reason);
    void rebuildIndex();
    void runSearch();
    void deleteSelectedDocument();
    void showSelectedDocument(QTreeWidgetItem* item, int column);
    void openSearchResult(int row, int column);
    void updateDeletedVisibility(bool showDeleted);

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
};
