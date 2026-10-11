#include "mainwindow.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <QAbstractItemView>
#include <QApplication>
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
#include "embedder.h"
#include "loader.h"
#include "persist.h"
#include "rrf.h"
#include "vector_index.h"
#include "vector_persist.h"

namespace {
constexpr int kRoleItemKind = Qt::UserRole;
constexpr int kRoleDocumentId = Qt::UserRole + 1;
constexpr int kRoleChunkId = Qt::UserRole + 2;
constexpr int kCategoryItem = 0;
constexpr int kDocumentItem = 1;
constexpr int kSnippetChars = 180;
// Caller-side wiring constants for the hybrid query chain (M3 step 4),
// registered in docs/PARAMS.md; mirrored in src/main.cpp.
constexpr int kHybridRouteK = 20;
constexpr int kEmbedSlice = 32;
// Dense model decided by the A/B experiment (M3 step 2, 2026-10-11):
// qwen3-embedding:0.6b beat bge-m3 on every metric; see docs/PARAMS.md.
constexpr const char* kDenseModel = "qwen3-embedding:0.6b";

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
    queryBar->addWidget(new QLabel(QStringLiteral("混合检索:"), central));
    queryEdit_ = new QLineEdit(central);
    queryEdit_->setPlaceholderText(
        QStringLiteral("关键词或自然语言问句，回车检索（BM25+语义双路）"));
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
    resultsTable_->setColumnCount(6);
    resultsTable_->setHorizontalHeaderLabels({
        QStringLiteral("Rank"),
        QStringLiteral("Score"),
        QStringLiteral("Routes"),
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
    resultsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    resultsTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

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
    denseLabel_ = new QLabel(this);
    snapshotLabel_ = new QLabel(this);
    versionsLabel_ = new QLabel(this);
    avgdlLabel_ = new QLabel(this);

    statusBar()->addPermanentWidget(docsLabel_);
    statusBar()->addPermanentWidget(chunksLabel_);
    statusBar()->addPermanentWidget(denseLabel_);
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
        embedChunks(QStringLiteral("启动"));
    } catch (const std::exception& e) {
        snapshotStateText_ = QStringLiteral("load error");
        refreshStatusBar();
        appendLog(QStringLiteral("加载失败: %1").arg(QString::fromUtf8(e.what())));
        QMessageBox::critical(this, QStringLiteral("加载失败"), QString::fromUtf8(e.what()));
    }
}

// M3 steps 4 + 5 (T9): build the dense side through the local Ollama
// provider with vector.bin caching. A document whose path + content
// hash + chunk count all match the snapshot reuses its vectors, so an
// unchanged corpus needs zero network and comes up instantly; a
// snapshot from a different model re-embeds everything. The embed
// still runs on the GUI thread (a worker thread is an M4 hardening
// item). Any failure degrades to BM25-only, never blocks.
void MainWindow::embedChunks(const QString& reason) {
    denseIndex_ = FlatIndex{};
    denseReady_ = false;

    if (chunks_.empty()) {
        denseStateText_ = QStringLiteral("dense: idle");
        refreshStatusBar();
        return;
    }

    if (!provider_)
        provider_ = make_ollama_provider(
            EmbedderConfig{std::string(), std::string(kDenseModel), 0});

    const auto t0 = std::chrono::steady_clock::now();
    try {
        std::vector<DocVectors> cached;
        const VectorIdentity expected{provider_->model_id(),
                                      provider_->embedding_policy(), 0};
        const VectorSnapshotStatus st = load_vector_snapshot(
            vectorSnapshotPath_, expected, cached);
        if (st == VectorSnapshotStatus::BAD_MODEL) {
            appendLog(QStringLiteral("%1: 向量快照是别的模型建的，全量重嵌。")
                .arg(reason));
        } else if (st != VectorSnapshotStatus::OK &&
                   st != VectorSnapshotStatus::NOT_FOUND) {
            appendLog(QStringLiteral("%1: 向量快照不可用，全量重嵌。")
                .arg(reason));
        }

        std::unordered_map<std::string, const DocVectors*> old;
        for (const DocVectors& d : cached)
            old.emplace(d.path, &d);

        std::vector<std::vector<size_t>> perDoc(docs_.size());
        for (size_t i = 0; i < chunks_.size(); ++i)
            perDoc[static_cast<size_t>(
                chunks_[i].document_id)].push_back(i);

        std::vector<DocVectors> records;
        records.reserve(docs_.size());
        std::vector<std::string> toEmbed;
        std::vector<std::pair<size_t, size_t>> pending;
        size_t reused = 0;
        for (const Document& doc : docs_) {
            if (doc.deleted)
                continue;
            DocVectors rec;
            rec.path = doc.path;
            rec.content_hash = vector_content_hash(doc.content);
            const size_t n = perDoc[static_cast<size_t>(doc.id)].size();
            const auto it = old.find(doc.path);
            if (it != old.end() &&
                it->second->content_hash == rec.content_hash &&
                it->second->vectors.size() == n) {
                rec.vectors = it->second->vectors;
                reused += n;
            } else {
                for (size_t ci : perDoc[static_cast<size_t>(doc.id)])
                    toEmbed.push_back(chunks_[ci].text);
                pending.emplace_back(records.size(), n);
            }
            records.push_back(std::move(rec));
        }

        if (!toEmbed.empty()) {
            appendLog(QStringLiteral("%1: 正在嵌入 %2 个块（%3，本地 Ollama）...")
                .arg(reason)
                .arg(static_cast<qulonglong>(toEmbed.size()))
                .arg(QString::fromUtf8(kDenseModel)));
            QApplication::processEvents();

            std::vector<std::vector<float>> embeds;
            embeds.reserve(toEmbed.size());
            const size_t total = toEmbed.size();
            for (size_t begin = 0; begin < total; begin += kEmbedSlice) {
                const size_t end = std::min(begin + kEmbedSlice, total);
                std::vector<std::string> texts;
                texts.reserve(end - begin);
                for (size_t i = begin; i < end; ++i)
                    texts.push_back(toEmbed[i]);
                const std::vector<std::vector<float>> part =
                    provider_->embed_documents(texts);
                embeds.insert(embeds.end(), part.begin(), part.end());
                QApplication::processEvents();
            }
            size_t k = 0;
            for (const auto& p : pending) {
                DocVectors& rec = records[p.first];
                for (size_t j = 0; j < p.second; ++j)
                    rec.vectors.push_back(embeds[k++]);
            }
        }

        std::vector<std::vector<float>> all;
        for (const DocVectors& dv : records)
            all.insert(all.end(), dv.vectors.begin(), dv.vectors.end());
        denseIndex_ = build_flat_index(all);
        if (!vector_save(vectorSnapshotPath_,
                VectorIdentity{provider_->model_id(),
                               provider_->embedding_policy(),
                               denseIndex_.dimension},
                records)) {
            appendLog(QStringLiteral("%1: 向量快照保存失败（内存向量仍可用）。")
                .arg(reason));
        }
        denseReady_ = true;
        const double secs = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        denseStateText_ = QStringLiteral("dense: %1 (%2x%3, %4s)")
            .arg(QString::fromUtf8(kDenseModel))
            .arg(static_cast<qulonglong>(denseIndex_.vectors.size()))
            .arg(denseIndex_.dimension)
            .arg(secs, 0, 'f', 1);
        appendLog(QStringLiteral(
            "%1: 语义路就绪 -- %2 复用 / %3 新嵌，共 %4 向量（%5 秒）。")
            .arg(reason)
            .arg(static_cast<qulonglong>(reused))
            .arg(static_cast<qulonglong>(toEmbed.size()))
            .arg(static_cast<qulonglong>(denseIndex_.vectors.size()))
            .arg(secs, 0, 'f', 1));
    } catch (const std::exception& e) {
        denseIndex_ = FlatIndex{};
        denseReady_ = false;
        denseStateText_ = QStringLiteral("dense: off");
        appendLog(QStringLiteral("%1: 语义路不可用（%2），检索退回仅 BM25。")
            .arg(reason)
            .arg(QString::fromUtf8(e.what())));
    }
    refreshStatusBar();
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
    embedChunks(QStringLiteral("重建"));
}

void MainWindow::runSearch() {
    resultsTable_->setRowCount(0);

    const QString query = queryEdit_->text().trimmed();
    if (query.isEmpty()) {
        return;
    }

    // M3 step 4: BM25 (lexical) + vector (semantic) routes fused by
    // RRF while the dense side is alive; plain BM25 otherwise.
    std::vector<SearchResult> bm25 =
        search(index_, toUtf8(query), kHybridRouteK);
    std::vector<SearchResult> dense;
    if (denseReady_) {
        try {
            dense = search_flat(denseIndex_,
                                provider_->embed_query(toUtf8(query)),
                                kHybridRouteK);
        } catch (const std::exception& e) {
            appendLog(QStringLiteral("查询: 语义路失败（%1），本次退回 BM25。")
                .arg(QString::fromUtf8(e.what())));
        }
    }

    const bool hybrid = !dense.empty();
    std::unordered_set<int> bm25Ids;
    std::unordered_set<int> denseIds;
    for (const SearchResult& r : bm25)
        bm25Ids.insert(r.chunk_id);
    for (const SearchResult& r : dense)
        denseIds.insert(r.chunk_id);
    const std::vector<SearchResult> results =
        hybrid ? rrf_fuse({bm25, dense}) : bm25;

    const int shown = static_cast<int>(
        std::min<size_t>(results.size(), static_cast<size_t>(TOP_K)));
    int visibleRank = 0;

    for (int i = 0; i < shown; ++i) {
        const SearchResult& result = results[static_cast<size_t>(i)];
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

        QString routes;
        if (hybrid) {
            const bool inBm25 = bm25Ids.count(result.chunk_id) != 0;
            const bool inDense = denseIds.count(result.chunk_id) != 0;
            routes = inBm25 && inDense ? QStringLiteral("bm25+vec")
                  : inBm25 ? QStringLiteral("bm25")
                  : QStringLiteral("vec");
        } else {
            routes = QStringLiteral("bm25");
        }

        const int row = resultsTable_->rowCount();
        resultsTable_->insertRow(row);
        ++visibleRank;

        auto* rankItem = new QTableWidgetItem(QString::number(visibleRank));
        rankItem->setData(kRoleChunkId, result.chunk_id);
        resultsTable_->setItem(row, 0, rankItem);
        resultsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(result.score, 'g', 8)));
        resultsTable_->setItem(row, 2, new QTableWidgetItem(routes));
        resultsTable_->setItem(row, 3, new QTableWidgetItem(fromUtf8(doc.path)));
        resultsTable_->setItem(row, 4, new QTableWidgetItem(QString::number(result.chunk_id)));
        resultsTable_->setItem(row, 5, new QTableWidgetItem(snippetForChunk(chunk)));
    }

    appendLog(QStringLiteral("查询: \"%1\" -> %2 条结果（%3）。")
        .arg(query)
        .arg(visibleRank)
        .arg(hybrid ? QStringLiteral("BM25+语义 RRF 混合")
                    : QStringLiteral("仅 BM25")));
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
    embedChunks(QStringLiteral("删除重建"));
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
    denseLabel_->setText(denseStateText_);
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

