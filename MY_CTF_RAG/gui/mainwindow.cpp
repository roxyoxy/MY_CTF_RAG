#include "mainwindow.h"

#include <exception>
#include <limits>
#include <utility>

#include <QAbstractItemView>
#include <QByteArray>
#include <QCheckBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "chunker.h"
#include "corpus_diff.h"
#include "loader.h"
#include "persist.h"

namespace {
constexpr int kRoleItemKind = Qt::UserRole;
constexpr int kRoleDocumentId = Qt::UserRole + 1;
constexpr int kRoleChunkId = Qt::UserRole + 2;
constexpr int kCategoryItem = 0;
constexpr int kDocumentItem = 1;
constexpr int kSnippetChars = 180;

QString fromUtf8(const std::string& text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

std::string toUtf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return std::string(bytes.constData(), static_cast<size_t>(bytes.size()));
}

QString snapshotStatusName(SnapshotStatus status) {
    switch (status) {
    case SnapshotStatus::OK:
        return QStringLiteral("ok");
    case SnapshotStatus::NOT_FOUND:
        return QStringLiteral("not found (first run)");
    case SnapshotStatus::IO_ERROR:
        return QStringLiteral("io error");
    case SnapshotStatus::BAD_MAGIC:
        return QStringLiteral("bad magic");
    case SnapshotStatus::BAD_FORMAT:
        return QStringLiteral("corrupt");
    case SnapshotStatus::BAD_PIPELINE:
        return QStringLiteral("pipeline version mismatch");
    case SnapshotStatus::BAD_PARAMS:
        return QStringLiteral("chunk params mismatch");
    case SnapshotStatus::CORPUS_CHANGED:
        return QStringLiteral("corpus changed");
    }
    return QStringLiteral("unknown");
}

QString categoryForPath(const std::string& path) {
    const size_t slash = path.find('/');
    if (slash == std::string::npos || slash == 0U) {
        return QStringLiteral("(root)");
    }
    return QString::fromUtf8(path.data(), static_cast<qsizetype>(slash));
}

QString snippetForChunk(const Chunk& chunk) {
    QString text = fromUtf8(chunk.text);
    text.replace(QChar('\r'), QChar(' '));
    text.replace(QChar('\n'), QChar(' '));
    text.replace(QChar('\t'), QChar(' '));
    text = text.simplified();
    if (text.size() > kSnippetChars) {
        text = text.left(kSnippetChars - 3) + QStringLiteral("...");
    }
    return text;
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    buildUi();
    connectUi();
    refreshStatusBar();

    QTimer::singleShot(0, this, [this]() {
        loadCorpusAndIndex(dataDirEdit_->text());
    });
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("MY_CTF_RAG 管理界面"));
    resize(1280, 820);

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);

    auto* corpusBar = new QHBoxLayout();
    corpusBar->addWidget(new QLabel(QStringLiteral("语料目录:"), central));

    dataDirEdit_ = new QLineEdit(QStringLiteral("./data"), central);
    dataDirEdit_->setClearButtonEnabled(true);
    corpusBar->addWidget(dataDirEdit_, 1);

    browseButton_ = new QPushButton(QStringLiteral("选择..."), central);
    reloadButton_ = new QPushButton(QStringLiteral("加载/刷新"), central);
    rebuildButton_ = new QPushButton(QStringLiteral("重建索引"), central);
    deleteButton_ = new QPushButton(QStringLiteral("软删除"), central);
    showDeletedCheck_ = new QCheckBox(QStringLiteral("显示已删除"), central);
    showDeletedCheck_->setChecked(true);

    corpusBar->addWidget(browseButton_);
    corpusBar->addWidget(reloadButton_);
    corpusBar->addWidget(rebuildButton_);
    corpusBar->addWidget(deleteButton_);
    corpusBar->addWidget(showDeletedCheck_);
    rootLayout->addLayout(corpusBar);

    auto* queryBar = new QHBoxLayout();
    queryBar->addWidget(new QLabel(QStringLiteral("BM25 查询:"), central));
    queryEdit_ = new QLineEdit(central);
    queryEdit_->setPlaceholderText(QStringLiteral("输入关键词，按回车检索"));
    searchButton_ = new QPushButton(QStringLiteral("检索"), central);
    queryBar->addWidget(queryEdit_, 1);
    queryBar->addWidget(searchButton_);
    rootLayout->addLayout(queryBar);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, central);

    documentTree_ = new QTreeWidget(mainSplitter);
    documentTree_->setColumnCount(3);
    documentTree_->setHeaderLabels({QStringLiteral("路径"), QStringLiteral("字节"), QStringLiteral("状态")});
    documentTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    documentTree_->setUniformRowHeights(true);
    documentTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    documentTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    documentTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    auto* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    previewEdit_ = new QPlainTextEdit(rightSplitter);
    previewEdit_->setReadOnly(true);
    previewEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    previewEdit_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    previewEdit_->setPlaceholderText(QStringLiteral("双击左侧文档，或双击检索结果以定位源文档。"));

    resultsTable_ = new QTableWidget(rightSplitter);
    resultsTable_->setColumnCount(5);
    resultsTable_->setHorizontalHeaderLabels({
        QStringLiteral("Rank"),
        QStringLiteral("Score"),
        QStringLiteral("Doc Path"),
        QStringLiteral("Chunk ID"),
        QStringLiteral("Snippet")
    });
    resultsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsTable_->setAlternatingRowColors(true);
    resultsTable_->verticalHeader()->setVisible(false);
    resultsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    resultsTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 2);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);
    rootLayout->addWidget(mainSplitter, 1);

    setCentralWidget(central);

    auto* logDock = new QDockWidget(QStringLiteral("日志"), this);
    logDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    logEdit_ = new QPlainTextEdit(logDock);
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumBlockCount(300);
    logEdit_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    logDock->setWidget(logEdit_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    docsLabel_ = new QLabel(this);
    chunksLabel_ = new QLabel(this);
    snapshotLabel_ = new QLabel(this);
    versionsLabel_ = new QLabel(this);
    avgdlLabel_ = new QLabel(this);

    statusBar()->addPermanentWidget(docsLabel_);
    statusBar()->addPermanentWidget(chunksLabel_);
    statusBar()->addPermanentWidget(snapshotLabel_, 1);
    statusBar()->addPermanentWidget(versionsLabel_);
    statusBar()->addPermanentWidget(avgdlLabel_);
}

void MainWindow::connectUi() {
    connect(browseButton_, &QPushButton::clicked, this, &MainWindow::chooseDataDirectory);
    connect(reloadButton_, &QPushButton::clicked, this, [this]() {
        loadCorpusAndIndex(dataDirEdit_->text());
    });
    connect(rebuildButton_, &QPushButton::clicked, this, &MainWindow::rebuildIndex);
    connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::deleteSelectedDocument);
    connect(searchButton_, &QPushButton::clicked, this, &MainWindow::runSearch);
    connect(queryEdit_, &QLineEdit::returnPressed, this, &MainWindow::runSearch);
    connect(documentTree_, &QTreeWidget::itemDoubleClicked, this, &MainWindow::showSelectedDocument);
    connect(resultsTable_, &QTableWidget::cellDoubleClicked, this, &MainWindow::openSearchResult);
    connect(showDeletedCheck_, &QCheckBox::toggled, this, &MainWindow::updateDeletedVisibility);
}

void MainWindow::chooseDataDirectory() {
    const QString selected = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择语料目录"),
        dataDirEdit_->text());

    if (selected.isEmpty()) {
        return;
    }

    dataDirEdit_->setText(selected);
    loadCorpusAndIndex(selected);
}

void MainWindow::loadCorpusAndIndex(const QString& dataDir) {
    try {
        std::vector<Document> newDocs = load_documents(toUtf8(dataDir));
        std::vector<Chunk> newChunks;
        InvertedIndex newIndex;

        const SnapshotStatus status = validate(snapshotPath_, newDocs);
        bool restored = false;
        if (status == SnapshotStatus::OK) {
            restored = load(snapshotPath_, newDocs, newChunks, newIndex);
        }

        QString branchLog;
        QString newSnapshotState;
        if (restored) {
            branchLog = QStringLiteral("启动: 快照命中，已恢复 index.bin。");
            newSnapshotState = QStringLiteral("ok (restored)");
        } else {
            if (status == SnapshotStatus::OK) {
                branchLog = QStringLiteral("启动: 快照校验通过但 load 失败，已重建。");
            } else {
                branchLog = QStringLiteral("启动: 快照未命中 (%1)，已重建。")
                    .arg(snapshotStatusName(status));
            }

            if (status == SnapshotStatus::CORPUS_CHANGED) {
                std::vector<Document> oldDocs;
                std::vector<Chunk> oldChunks;
                InvertedIndex oldIndex;
                if (load(snapshotPath_, oldDocs, oldChunks, oldIndex)) {
                    const CorpusDiff diff = diff_corpora(oldDocs, newDocs);
                    const int inherited = inherit_tombstones(oldDocs, newDocs);
                    branchLog += QStringLiteral(" 语料变更: %1 新增, %2 移除, %3 编辑")
                        .arg(static_cast<qulonglong>(diff.added.size()))
                        .arg(static_cast<qulonglong>(diff.removed.size()))
                        .arg(static_cast<qulonglong>(diff.edited.size()));
                    if (inherited > 0) {
                        branchLog += QStringLiteral(", %1 个墓碑已继承")
                            .arg(inherited);
                    }
                    branchLog += QStringLiteral("。");
                }
            }

            newChunks = chunk_documents(newDocs);
            newIndex = build_index(newChunks);
            if (save(snapshotPath_, newDocs, newChunks, newIndex)) {
                newSnapshotState = QStringLiteral("ok (rebuilt)");
            } else {
                newSnapshotState = QStringLiteral("save failed (memory only)");
                branchLog += QStringLiteral(" 快照保存失败，当前内存索引仍可用。");
            }
        }

        docs_ = std::move(newDocs);
        chunks_ = std::move(newChunks);
        index_ = std::move(newIndex);
        snapshotStateText_ = newSnapshotState;
        previewDocumentId_ = -1;
        corpusLoaded_ = true;

        refreshDocumentTree();
        resultsTable_->setRowCount(0);
        previewEdit_->clear();
        refreshStatusBar();
        appendLog(branchLog);
    } catch (const std::exception& e) {
        snapshotStateText_ = QStringLiteral("load error");
        refreshStatusBar();
        appendLog(QStringLiteral("加载失败: %1").arg(QString::fromUtf8(e.what())));
        QMessageBox::critical(this, QStringLiteral("加载失败"), QString::fromUtf8(e.what()));
    }
}

void MainWindow::rebuildIndex() {
    if (!corpusLoaded_) {
        appendLog(QStringLiteral("重建: 当前没有已加载语料。"));
        return;
    }

    chunks_ = chunk_documents(docs_);
    index_ = build_index(chunks_);
    const bool saved = save(snapshotPath_, docs_, chunks_, index_);
    snapshotStateText_ = saved
        ? QStringLiteral("ok (rebuilt)")
        : QStringLiteral("save failed (memory only)");

    resultsTable_->setRowCount(0);
    refreshStatusBar();
    appendLog(saved
        ? QStringLiteral("重建: chunk + build_index + save 完成。")
        : QStringLiteral("重建: 内存索引已更新，但快照保存失败。"));
}

void MainWindow::runSearch() {
    resultsTable_->setRowCount(0);

    const QString query = queryEdit_->text().trimmed();
    if (query.isEmpty()) {
        return;
    }

    const std::vector<SearchResult> results = search(index_, toUtf8(query), TOP_K);
    int visibleRank = 0;

    for (const SearchResult& result : results) {
        if (result.chunk_id < 0 || static_cast<size_t>(result.chunk_id) >= chunks_.size()) {
            continue;
        }

        const Chunk& chunk = chunks_[static_cast<size_t>(result.chunk_id)];
        if (chunk.document_id < 0 || static_cast<size_t>(chunk.document_id) >= docs_.size()) {
            continue;
        }

        const Document& doc = docs_[static_cast<size_t>(chunk.document_id)];
        if (doc.deleted && !showDeletedCheck_->isChecked()) {
            continue;
        }

        const int row = resultsTable_->rowCount();
        resultsTable_->insertRow(row);
        ++visibleRank;

        auto* rankItem = new QTableWidgetItem(QString::number(visibleRank));
        rankItem->setData(kRoleChunkId, result.chunk_id);
        resultsTable_->setItem(row, 0, rankItem);
        resultsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(result.score, 'g', 8)));
        resultsTable_->setItem(row, 2, new QTableWidgetItem(fromUtf8(doc.path)));
        resultsTable_->setItem(row, 3, new QTableWidgetItem(QString::number(result.chunk_id)));
        resultsTable_->setItem(row, 4, new QTableWidgetItem(snippetForChunk(chunk)));
    }

    appendLog(QStringLiteral("查询: \"%1\" -> %2 条结果。")
        .arg(query)
        .arg(visibleRank));
}

void MainWindow::deleteSelectedDocument() {
    QTreeWidgetItem* item = documentTree_->currentItem();
    if (item == nullptr || item->data(0, kRoleItemKind).toInt() != kDocumentItem) {
        QMessageBox::information(this, QStringLiteral("软删除"), QStringLiteral("请先选中一个文档。"));
        return;
    }

    const int documentId = item->data(0, kRoleDocumentId).toInt();
    if (documentId < 0 || static_cast<size_t>(documentId) >= docs_.size()) {
        appendLog(QStringLiteral("删除失败: 文档 ID 越界。"));
        return;
    }

    Document& doc = docs_[static_cast<size_t>(documentId)];
    if (doc.deleted) {
        appendLog(QStringLiteral("删除: %1 已经是墓碑状态。").arg(fromUtf8(doc.path)));
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("确认软删除"),
        QStringLiteral("将文档标记为已删除并立即重建索引：\n%1").arg(fromUtf8(doc.path)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    doc.deleted = true;
    chunks_ = chunk_documents(docs_);
    index_ = build_index(chunks_);
    const bool saved = save(snapshotPath_, docs_, chunks_, index_);
    snapshotStateText_ = saved
        ? QStringLiteral("ok (deleted + rebuilt)")
        : QStringLiteral("save failed (memory only)");

    refreshDocumentTree();
    resultsTable_->setRowCount(0);
    refreshStatusBar();

    appendLog(saved
        ? QStringLiteral("删除: %1 -> 墓碑 + 重建 + 保存完成。").arg(fromUtf8(doc.path))
        : QStringLiteral("删除: %1 -> 内存已更新，但快照保存失败。").arg(fromUtf8(doc.path)));
}

void MainWindow::showSelectedDocument(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item == nullptr || item->data(0, kRoleItemKind).toInt() != kDocumentItem) {
        return;
    }
    showDocument(item->data(0, kRoleDocumentId).toInt());
}

void MainWindow::openSearchResult(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* rankItem = resultsTable_->item(row, 0);
    if (rankItem == nullptr) {
        return;
    }
    highlightChunk(rankItem->data(kRoleChunkId).toInt());
}

void MainWindow::updateDeletedVisibility(bool showDeleted) {
    for (int i = 0; i < documentTree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = documentTree_->topLevelItem(i);
        int visibleChildren = 0;
        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem* item = category->child(j);
            const int documentId = item->data(0, kRoleDocumentId).toInt();
            const bool validId = documentId >= 0 && static_cast<size_t>(documentId) < docs_.size();
            const bool hide = validId && docs_[static_cast<size_t>(documentId)].deleted && !showDeleted;
            item->setHidden(hide);
            if (!hide) {
                ++visibleChildren;
            }
        }
        category->setHidden(visibleChildren == 0);
    }

    if (!queryEdit_->text().trimmed().isEmpty()) {
        runSearch();
    }
}

void MainWindow::refreshDocumentTree() {
    documentTree_->clear();

    QHash<QString, QTreeWidgetItem*> categories;
    for (const Document& doc : docs_) {
        const QString categoryName = categoryForPath(doc.path);
        QTreeWidgetItem* category = categories.value(categoryName, nullptr);
        if (category == nullptr) {
            category = new QTreeWidgetItem(documentTree_);
            category->setText(0, categoryName);
            category->setData(0, kRoleItemKind, kCategoryItem);
            QFont font = category->font(0);
            font.setBold(true);
            category->setFont(0, font);
            categories.insert(categoryName, category);
        }

        auto* item = new QTreeWidgetItem(category);
        item->setText(0, fromUtf8(doc.path));
        item->setText(1, QString::number(static_cast<qulonglong>(doc.content.size())));
        item->setText(2, doc.deleted ? QStringLiteral("已删除") : QStringLiteral("有效"));
        item->setData(0, kRoleItemKind, kDocumentItem);
        item->setData(0, kRoleDocumentId, doc.id);
        item->setToolTip(0, fromUtf8(doc.path));

        if (doc.deleted) {
            for (int col = 0; col < documentTree_->columnCount(); ++col) {
                QFont font = item->font(col);
                font.setStrikeOut(true);
                item->setFont(col, font);
            }
        }
    }

    documentTree_->sortItems(0, Qt::AscendingOrder);
    documentTree_->expandAll();
    updateDeletedVisibility(showDeletedCheck_->isChecked());
}

void MainWindow::refreshStatusBar() {
    docsLabel_->setText(QStringLiteral("docs: %1").arg(static_cast<qulonglong>(docs_.size())));
    chunksLabel_->setText(QStringLiteral("chunks: %1").arg(static_cast<qulonglong>(chunks_.size())));
    snapshotLabel_->setText(QStringLiteral("snapshot: %1").arg(snapshotStateText_));
    versionsLabel_->setText(QStringLiteral("format/pipeline: %1/%2")
        .arg(FORMAT_VERSION)
        .arg(PIPELINE_VERSION));
    avgdlLabel_->setText(QStringLiteral("avgdl: %1").arg(index_.avg_chunk_length, 0, 'f', 2));
}

void MainWindow::showDocument(int documentId) {
    if (documentId < 0 || static_cast<size_t>(documentId) >= docs_.size()) {
        return;
    }

    const Document& doc = docs_[static_cast<size_t>(documentId)];
    previewEdit_->setPlainText(fromUtf8(doc.content));
    previewDocumentId_ = documentId;
    previewEdit_->moveCursor(QTextCursor::Start);
    statusBar()->showMessage(fromUtf8(doc.path), 4000);
}

void MainWindow::highlightChunk(int chunkId) {
    if (chunkId < 0 || chunkId >= static_cast<int>(chunks_.size())) {
        return;
    }

    const Chunk& chunk = chunks_[static_cast<size_t>(chunkId)];
    if (chunk.document_id < 0 || static_cast<size_t>(chunk.document_id) >= docs_.size()) {
        return;
    }

    const Document& doc = docs_[static_cast<size_t>(chunk.document_id)];
    if (chunk.begin > chunk.end || chunk.end > doc.content.size()) {
        appendLog(QStringLiteral("定位失败: chunk 字节区间无效。"));
        return;
    }

    if (previewDocumentId_ != chunk.document_id) {
        showDocument(chunk.document_id);
    }

    const QString prefix = QString::fromUtf8(
        doc.content.data(),
        static_cast<qsizetype>(chunk.begin));
    const QString selected = QString::fromUtf8(
        doc.content.data() + chunk.begin,
        static_cast<qsizetype>(chunk.end - chunk.begin));

    if (prefix.size() > static_cast<qsizetype>(std::numeric_limits<int>::max()) ||
        selected.size() > static_cast<qsizetype>(std::numeric_limits<int>::max()) - prefix.size()) {
        appendLog(QStringLiteral("定位失败: 文档过大，超出 QTextCursor 范围。"));
        return;
    }

    QTextCursor cursor(previewEdit_->document());
    const int beginPosition = static_cast<int>(prefix.size());
    const int endPosition = beginPosition + static_cast<int>(selected.size());
    cursor.setPosition(beginPosition);
    cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
    previewEdit_->setTextCursor(cursor);
    previewEdit_->ensureCursorVisible();
    previewEdit_->setFocus();
}

void MainWindow::appendLog(const QString& text) {
    logEdit_->appendPlainText(text);
}

