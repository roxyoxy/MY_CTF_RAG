#include "mainwindow.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <limits>
#include <string>
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
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextCursor>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "persist.h"

namespace {
constexpr int kRoleItemKind = Qt::UserRole;
constexpr int kRoleDocumentId = Qt::UserRole + 1;
constexpr int kRoleChunkId = Qt::UserRole + 2;
const int kCategoryItem = 0;
const int kDocumentItem = 1;
constexpr int kSnippetChars = 180;

QString fromUtf8(const std::string& text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

std::string toUtf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return std::string(bytes.constData(), static_cast<size_t>(bytes.size()));
}

// Wording for the engine's status enums. The engine returns codes; the
// console says English, the GUI says Chinese, and neither belongs in
// the engine.
QString snapshotStatusName(SnapshotStatus status) {
    switch (status) {
    case SnapshotStatus::OK:             return QStringLiteral("正常");
    case SnapshotStatus::NOT_FOUND:      return QStringLiteral("不存在（首次运行）");
    case SnapshotStatus::IO_ERROR:       return QStringLiteral("无法读取");
    case SnapshotStatus::BAD_MAGIC:      return QStringLiteral("不是本程序的快照");
    case SnapshotStatus::BAD_FORMAT:     return QStringLiteral("已损坏");
    case SnapshotStatus::BAD_PIPELINE:   return QStringLiteral("分词口径已变");
    case SnapshotStatus::BAD_PARAMS:     return QStringLiteral("切块参数不匹配");
    case SnapshotStatus::CORPUS_CHANGED: return QStringLiteral("语料已变动");
    }
    return QStringLiteral("未知");
}

QString vectorStatusName(VectorSnapshotStatus status) {
    switch (status) {
    case VectorSnapshotStatus::OK:         return QStringLiteral("正常");
    case VectorSnapshotStatus::NOT_FOUND:  return QStringLiteral("不存在（首次运行）");
    case VectorSnapshotStatus::IO_ERROR:   return QStringLiteral("无法读取");
    case VectorSnapshotStatus::BAD_MAGIC:  return QStringLiteral("不是本程序的快照");
    case VectorSnapshotStatus::BAD_FORMAT: return QStringLiteral("已损坏");
    case VectorSnapshotStatus::BAD_MODEL:  return QStringLiteral("由别的模型建立");
    }
    return QStringLiteral("未知");
}

QString categoryForPath(const std::string& path) {
    const size_t slash = path.find('/');
    if (slash == std::string::npos || slash == 0U) {
        return QStringLiteral("(root)");
    }
    return QString::fromUtf8(path.data(), static_cast<qsizetype>(slash));
}

QString snippetForText(const std::string& text) {
    QString flattened = fromUtf8(text);
    flattened.replace(QChar('\r'), QChar(' '));
    flattened.replace(QChar('\n'), QChar(' '));
    flattened.replace(QChar('\t'), QChar(' '));
    flattened = flattened.simplified();
    if (flattened.size() > kSnippetChars) {
        flattened = flattened.left(kSnippetChars - 3) + QStringLiteral("...");
    }
    return flattened;
}

// The citation ledger carries the position, not the text; the snippet
// comes from the hit with the same chunk id (used_materials is always a
// subset of hits -- the context builder only drops from the tail).
QString snippetForChunkId(const QueryOutcome& outcome, int chunkId) {
    for (const QueryHit& hit : outcome.hits) {
        if (hit.chunk_id == chunkId) {
            return snippetForText(hit.text);
        }
    }
    return QString();
}

QString outcomeDenseState(const EngineStatus& status) {
    if (status.dense_ready) {
        return QStringLiteral("语义路 %1 (%2x%3)")
            .arg(fromUtf8(status.dense_model_id))
            .arg(static_cast<qulonglong>(status.vectors_total))
            .arg(status.dimension);
    }
    return QStringLiteral("语义路未启用");
}

// The "you are about to spend real time" gate. Returns false when the
// user chose to quit instead of updating.
bool confirmUpdate(QWidget* parent, const EnginePlan& plan) {
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QStringLiteral("需要更新索引"));
    box.setText(QStringLiteral("语料或索引快照与当前状态不一致，需要建立索引。"));

    const QStringList details = {
        QStringLiteral("· 词法索引：%1%2")
            .arg(snapshotStatusName(plan.index_state),
                 plan.index_needs_rebuild ? QStringLiteral(" → 将重建")
                                          : QStringLiteral(" → 可复用")),
        QStringLiteral("· 向量索引：%1%2")
            .arg(vectorStatusName(plan.vector_state),
                 plan.vector_needs_work ? QStringLiteral(" → 需嵌入")
                                        : QStringLiteral(" → 可复用")),
    };
    box.setInformativeText(QStringLiteral(
        "本地模型推理可能耗时数十秒，期间界面不可操作。\n\n%1")
        .arg(details.join(QStringLiteral("\n"))));

    QPushButton* update = box.addButton(QStringLiteral("更新"), QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("退出"), QMessageBox::RejectRole);
    box.exec();
    return box.clickedButton() == update;
}

// A modal progress dialog driven by the engine's progress callback.
// The engine reports on its calling thread -- the UI thread here -- so
// this is a plain adapter with no cross-thread plumbing.
// setMinimumDuration keeps a fast rebuild from flashing a dialog; the
// modality is what removes the re-entrancy the old code allowed (its
// processEvents() let the user click "rebuild" mid-embed).
class ProgressRelay {
public:
    ProgressRelay(QWidget* parent, QString model)
        : dialog_(QStringLiteral("准备中…"), QString(), 0, 100, parent),
          model_(std::move(model)) {
        dialog_.setWindowTitle(QStringLiteral("建立索引"));
        dialog_.setWindowModality(Qt::ApplicationModal);
        dialog_.setMinimumDuration(400);
        dialog_.setAutoClose(false);
        dialog_.setAutoReset(false);
        dialog_.setMinimumWidth(460);
    }

    // Every stage except "plan" (which the caller turns into a
    // question, not a progress bar).
    bool operator()(const EngineProgress& progress) {
        if (progress.stage == "index") {
            dialog_.setLabelText(QStringLiteral("正在重建词法索引…"));
            dialog_.setValue(0);
        } else if (progress.stage == "vectors") {
            dialog_.setLabelText(QStringLiteral(
                "正在嵌入向量（%1）\n第 %2 / %3 块")
                .arg(model_)
                .arg(static_cast<qulonglong>(progress.done))
                .arg(static_cast<qulonglong>(progress.total)));
            if (progress.total > 0) {
                dialog_.setValue(static_cast<int>(
                    progress.done * 100 / progress.total));
            }
        }
        return !dialog_.wasCanceled();
    }

    void show() { dialog_.show(); }
    void hide() { dialog_.hide(); }
    bool wasCanceled() const { return dialog_.wasCanceled(); }

private:
    QProgressDialog dialog_;
    QString model_;
};

// One QThread per query. Deliberately NOT a QObject with Q_OBJECT --
// this GUI avoids moc by design (gui/README.md); the two UI-thread
// notifications are marshaled through QMetaObject::invokeMethod
// functors, which need no Q_OBJECT and no metatype registration. The
// context object (the window) guarantees the functors run on the UI
// thread or not at all; the callbacks themselves are MainWindow member
// lambdas, so they die with the window and never dangle.
class QueryThread final : public QThread {
public:
    using StartedCallback = std::function<void()>;
    using FinishedCallback = std::function<void(QueryResultPtr)>;

    QueryThread(QueryRequest request, QObject* context,
                StartedCallback onStarted, FinishedCallback onFinished)
        : request_(std::move(request)), context_(context),
          onStarted_(std::move(onStarted)),
          onFinished_(std::move(onFinished)) {}

    void run() override {
        const QueryResultPtr result = run_query_chain(request_, [this]() {
            if (onStarted_) {
                const StartedCallback callback = onStarted_;
                QMetaObject::invokeMethod(
                    context_, [callback]() { callback(); },
                    Qt::QueuedConnection);
            }
        });
        if (onFinished_) {
            const FinishedCallback callback = onFinished_;
            QMetaObject::invokeMethod(
                context_, [callback, result]() { callback(result); },
                Qt::QueuedConnection);
        }
    }

private:
    QueryRequest request_;
    QObject* context_ = nullptr;
    StartedCallback onStarted_;
    FinishedCallback onFinished_;
};
}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    buildUi();
    connectUi();
    refreshStatusBar();

    QTimer::singleShot(0, this, [this]() {
        startup();
    });
}

MainWindow::~MainWindow() {
    // QueryThread::run calls into engine_, which this window owns, and
    // its HTTP calls are not interruptible: the only safe shutdown is
    // to wait. This blocks only when the window is closed mid-answer.
    if (chatBusy_ && busyThread_ != nullptr) {
        busyThread_->wait();
    }
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

    // M4 step 4: the chat panel. One window, everything visible -- the
    // answer with clickable [n] citations, the sources ledger, and the
    // shared source preview above for the jump-to-chunk highlight.
    auto* chatPanel = new QWidget(rightSplitter);
    auto* chatLayout = new QVBoxLayout(chatPanel);

    auto* chatHeader = new QHBoxLayout();
    chatHeader->addWidget(new QLabel(QStringLiteral("RAG 问答（单轮）"), chatPanel));
    chatStageLabel_ = new QLabel(chatPanel);
    chatStageLabel_->setText(QStringLiteral("就绪"));
    chatHeader->addWidget(chatStageLabel_, 1);
    chatLayout->addLayout(chatHeader);

    auto* chatSplitter = new QSplitter(Qt::Horizontal, chatPanel);
    answerBrowser_ = new QTextBrowser(chatSplitter);
    answerBrowser_->setOpenLinks(false);
    answerBrowser_->setOpenExternalLinks(false);
    answerBrowser_->setPlaceholderText(QStringLiteral(
        "输入问题并点“生成回答”。答案中的 [n] 引用可点击，跳转到来源"
        "片段并高亮；越界引用会标红。"));

    sourcesTable_ = new QTableWidget(chatSplitter);
    sourcesTable_->setColumnCount(4);
    sourcesTable_->setHorizontalHeaderLabels({
        QStringLiteral("[n]"),
        QStringLiteral("路径"),
        QStringLiteral("Chunk"),
        QStringLiteral("摘要")
    });
    sourcesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sourcesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    sourcesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    sourcesTable_->setAlternatingRowColors(true);
    sourcesTable_->verticalHeader()->setVisible(false);
    sourcesTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    sourcesTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    sourcesTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    sourcesTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    chatSplitter->setStretchFactor(0, 3);
    chatSplitter->setStretchFactor(1, 2);
    chatLayout->addWidget(chatSplitter, 1);

    auto* askBar = new QHBoxLayout();
    askBar->addWidget(new QLabel(QStringLiteral("提问:"), chatPanel));
    askEdit_ = new QLineEdit(chatPanel);
    askEdit_->setPlaceholderText(QStringLiteral(
        "自然语言问句，回车生成（检索 + 本地模型作答，约 10 秒）"));
    askButton_ = new QPushButton(QStringLiteral("生成回答"), chatPanel);
    askBar->addWidget(askEdit_, 1);
    askBar->addWidget(askButton_);
    chatLayout->addLayout(askBar);

    rightSplitter->addWidget(chatPanel);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 2);
    rightSplitter->setStretchFactor(2, 3);
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
    connect(reloadButton_, &QPushButton::clicked, this, [this]() { startup(); });
    connect(rebuildButton_, &QPushButton::clicked, this, &MainWindow::rebuildIndex);
    connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::deleteSelectedDocument);
    connect(searchButton_, &QPushButton::clicked, this, &MainWindow::startRetrieval);
    connect(queryEdit_, &QLineEdit::returnPressed, this, &MainWindow::startRetrieval);
    connect(askButton_, &QPushButton::clicked, this, &MainWindow::startChatQuery);
    connect(askEdit_, &QLineEdit::returnPressed, this, &MainWindow::startChatQuery);
    connect(answerBrowser_, &QTextBrowser::anchorClicked, this, &MainWindow::onAnswerAnchorClicked);
    connect(sourcesTable_, &QTableWidget::cellClicked, this, &MainWindow::onSourceRowClicked);
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
    startup();
}

// Load the corpus and bring all three sides up (contract clause 5: the
// BM25 stage lands before the vector stage, so cancelling the vector
// stage still leaves a working lexical engine).
//
// The whole thing runs on the UI thread behind a modal progress dialog.
// That is deliberate: the work is not interruptible anyway, the dialog
// is honest about it, and modality removes the re-entrancy the previous
// implementation allowed -- embedChunks() called processEvents() inside
// its loop, so a user clicking "rebuild" during the 20-second cold
// embed could nest a second embedding on top of the first.
void MainWindow::startup() {
    if (chatBusy_) {
        appendLog(QStringLiteral("加载: 问答仍在进行，请等本轮结束后再切换语料。"));
        return;
    }

    EngineConfig config;
    config.data_dir = toUtf8(dataDirEdit_->text());

    ProgressRelay relay(this, fromUtf8(config.dense_model));
    bool quitRequested = false;

    const EngineProgressFn onProgress = [&](const EngineProgress& progress) {
        if (progress.stage == "plan") {
            const EnginePlan& plan = engine_.status().plan;
            if (!plan.needs_work()) {
                return true;   // nothing to ask about; every side restores
            }
            if (!confirmUpdate(this, plan)) {
                quitRequested = true;
                return false;  // stop before doing any work
            }
            relay.show();
            return true;
        }
        return relay(progress);
    };

    try {
        status_ = engine_.open(config, onProgress);
    } catch (const std::exception& e) {
        relay.hide();
        engineReady_ = false;
        refreshStatusBar();
        appendLog(QStringLiteral("加载失败: %1").arg(QString::fromUtf8(e.what())));
        QMessageBox::critical(this, QStringLiteral("加载失败"),
            QString::fromUtf8(e.what()));
        return;
    }
    relay.hide();

    if (quitRequested) {
        appendLog(QStringLiteral("启动: 用户选择退出。"));
        close();
        return;
    }

    engineReady_ = status_.loaded;
    previewDocumentId_ = -1;
    refreshDocumentTree();
    resultsTable_->setRowCount(0);
    previewEdit_->clear();
    refreshStatusBar();
    appendStartupReport(status_);
    appendDenseReport(status_, QStringLiteral("启动"));
}

void MainWindow::appendStartupReport(const EngineStatus& status) {
    if (status.index_restored) {
        appendLog(QStringLiteral("启动: 快照命中，已恢复 index.bin。"));
    } else if (status.index_state == SnapshotStatus::OK) {
        appendLog(QStringLiteral("启动: 快照校验通过但 load 失败，已重建。"));
    } else {
        QString line = QStringLiteral("启动: 快照未命中 (%1)，已重建。")
            .arg(snapshotStatusName(status.index_state));
        if (status.corpus_change.detected) {
            line += QStringLiteral(" 语料变更: %1 新增, %2 移除, %3 编辑")
                .arg(static_cast<qulonglong>(status.corpus_change.added))
                .arg(static_cast<qulonglong>(status.corpus_change.removed))
                .arg(static_cast<qulonglong>(status.corpus_change.edited));
            if (status.corpus_change.tombstones_inherited > 0) {
                line += QStringLiteral(", %1 个墓碑已继承")
                    .arg(static_cast<qulonglong>(
                        status.corpus_change.tombstones_inherited));
            }
            line += QStringLiteral("。");
        }
        if (!status.index_saved) {
            line += QStringLiteral(" 快照保存失败，当前内存索引仍可用。");
        }
        appendLog(line);
    }
}

void MainWindow::appendDenseReport(const EngineStatus& status, const QString& when) {
    if (status.dense_ready) {
        appendLog(QStringLiteral(
            "%1: 语义路就绪 -- %2 复用 / %3 新嵌，共 %4 向量（%5 秒）。")
            .arg(when)
            .arg(static_cast<qulonglong>(status.vectors_reused))
            .arg(static_cast<qulonglong>(status.vectors_embedded))
            .arg(static_cast<qulonglong>(status.vectors_total))
            .arg(status.dense_seconds, 0, 'f', 1));
    } else if (status.cancelled) {
        appendLog(QStringLiteral(
            "%1: 已取消，语义路未启用；检索退回仅 BM25。").arg(when));
    } else if (!status.dense_error.empty()) {
        appendLog(QStringLiteral("%1: 语义路不可用（%2），检索退回仅 BM25。")
            .arg(when)
            .arg(fromUtf8(status.dense_error)));
    }
}

void MainWindow::rebuildIndex() {
    if (chatBusy_) {
        appendLog(QStringLiteral("重建: 问答仍在进行，请等本轮结束后再操作。"));
        return;
    }
    if (!engineReady_) {
        appendLog(QStringLiteral("重建: 当前没有已加载语料。"));
        return;
    }

    ProgressRelay relay(this, fromUtf8(engine_.config().dense_model));
    status_ = engine_.rebuild([&relay](const EngineProgress& progress) {
        return relay(progress);
    });
    relay.hide();

    engineReady_ = status_.loaded;
    refreshDocumentTree();
    resultsTable_->setRowCount(0);
    refreshStatusBar();
    appendLog(status_.index_saved
        ? QStringLiteral("重建: chunk + build_index + save 完成。")
        : QStringLiteral("重建: 内存索引已更新，但快照保存失败。"));
    appendDenseReport(status_, QStringLiteral("重建"));
}

void MainWindow::startRetrieval() {
    const QString query = queryEdit_->text().trimmed();
    if (query.isEmpty()) {
        return;
    }
    launchWorker(query, false);
}

void MainWindow::startChatQuery() {
    if (!engineReady_) {
        appendLog(QStringLiteral("问答: 当前没有已加载语料。"));
        return;
    }
    const QString query = askEdit_->text().trimmed();
    if (query.isEmpty()) {
        return;
    }
    launchWorker(query, true);
}

// No query work on the UI thread. Both the retrieval-only search and
// the full RAG ask run the engine's two phases inside a QueryThread;
// this window only launches, renders and refuses corpus mutations while
// the worker is busy -- which is also what upholds the engine's "one
// thread at a time" contract.
void MainWindow::launchWorker(const QString& query, bool withGeneration) {
    if (chatBusy_) {
        return;
    }

    chatBusy_ = true;
    lastQuestion_ = query;
    lastResult_.reset();
    setChatBusy(true);
    resultsTable_->setRowCount(0);
    if (withGeneration) {
        chatStageLabel_->setText(QStringLiteral("检索中…（UI 不阻塞）"));
    }

    QueryRequest request;
    request.engine = &engine_;
    request.query = toUtf8(query);
    request.withGeneration = withGeneration;

    auto* thread = new QueryThread(
        std::move(request), this,
        [this]() { onWorkerGenerationStarted(); },
        [this](QueryResultPtr result) { onWorkerFinished(result); });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    busyThread_ = thread;
    thread->start();
}

void MainWindow::fillResultsTable(const QueryOutcome& outcome) {
    resultsTable_->setRowCount(0);

    for (const QueryHit& hit : outcome.hits) {
        // The chunker never emits chunks for a tombstone, so a deleted
        // document cannot appear here; this filter is double-insurance
        // for the "show deleted" toggle.
        if (hit.location.document_id >= 0 &&
            static_cast<size_t>(hit.location.document_id) < docs_.size() &&
            docs_[static_cast<size_t>(hit.location.document_id)].deleted &&
            !showDeletedCheck_->isChecked()) {
            continue;
        }

        QString routes;
        if (outcome.hybrid) {
            routes = hit.bm25.present && hit.dense.present
                ? QStringLiteral("bm25+vec")
                : hit.bm25.present ? QStringLiteral("bm25")
                                   : QStringLiteral("vec");
        } else {
            routes = QStringLiteral("bm25");
        }

        const int row = resultsTable_->rowCount();
        resultsTable_->insertRow(row);

        auto* rankItem = new QTableWidgetItem(QString::number(hit.rank));
        rankItem->setData(kRoleChunkId, hit.chunk_id);
        resultsTable_->setItem(row, 0, rankItem);
        resultsTable_->setItem(row, 1, new QTableWidgetItem(
            QString::number(hit.rrf, 'g', 8)));
        resultsTable_->setItem(row, 2, new QTableWidgetItem(routes));
        resultsTable_->setItem(row, 3, new QTableWidgetItem(fromUtf8(hit.doc_path)));
        resultsTable_->setItem(row, 4, new QTableWidgetItem(QString::number(hit.chunk_id)));
        resultsTable_->setItem(row, 5, new QTableWidgetItem(snippetForText(hit.text)));
    }
}

void MainWindow::setChatBusy(bool busy) {
    searchButton_->setEnabled(!busy);
    askButton_->setEnabled(!busy);
    browseButton_->setEnabled(!busy);
    reloadButton_->setEnabled(!busy);
    rebuildButton_->setEnabled(!busy);
    deleteButton_->setEnabled(!busy);
    showDeletedCheck_->setEnabled(!busy);
    // The tree reaches into the engine for a document's text, and the
    // engine is busy: keep the UI off it until the worker is done.
    documentTree_->setEnabled(!busy);
}

void MainWindow::onWorkerGenerationStarted() {
    chatStageLabel_->setText(QStringLiteral(
        "生成中…（本地模型 %1，UI 不阻塞）")
        .arg(fromUtf8(engine_.config().generation_model)));
}

void MainWindow::onWorkerFinished(QueryResultPtr result) {
    chatBusy_ = false;
    busyThread_ = nullptr;
    setChatBusy(false);
    lastResult_ = result;

    const QueryOutcome& outcome = result->outcome;

    if (!outcome.dense_error.empty()) {
        appendLog(QStringLiteral("查询: 语义路失败（%1），本次退回 BM25。")
            .arg(fromUtf8(outcome.dense_error)));
    }

    fillResultsTable(outcome);

    // The sources ledger: exactly the materials the model actually saw
    // (context.h), one row per [n].
    const int sourceCount = static_cast<int>(outcome.citations.size());
    sourcesTable_->setRowCount(sourceCount);
    for (int i = 0; i < sourceCount; ++i) {
        const Citation& citation = outcome.citations[static_cast<size_t>(i)];
        auto* numberItem = new QTableWidgetItem(
            QStringLiteral("[%1]").arg(citation.n));
        numberItem->setData(kRoleChunkId, citation.chunk_id);
        sourcesTable_->setItem(i, 0, numberItem);
        sourcesTable_->setItem(i, 1, new QTableWidgetItem(fromUtf8(citation.doc_path)));
        sourcesTable_->setItem(i, 2, new QTableWidgetItem(QString::number(citation.chunk_id)));
        sourcesTable_->setItem(i, 3, new QTableWidgetItem(
            snippetForChunkId(outcome, citation.chunk_id)));
    }

    appendLog(QStringLiteral("查询: \"%1\" -> %2 条结果（%3，检索 %4 秒）。")
        .arg(lastQuestion_)
        .arg(resultsTable_->rowCount())
        .arg(outcome.hybrid ? QStringLiteral("BM25+语义 RRF 混合")
                            : QStringLiteral("仅 BM25"))
        .arg(outcome.retrieval_seconds, 0, 'f', 2));

    if (!outcome.generation_attempted) {
        return;
    }

    if (!outcome.generation_ok) {
        chatStageLabel_->setText(QStringLiteral("生成失败，本轮降级为仅检索"));
        answerBrowser_->setHtml(QStringLiteral(
            "<p><b>%1</b> %2</p><hr>"
            "<p style=\"color:#b00020;\">%3</p>"
            "<p>%4</p>")
            .arg(QStringLiteral("问:"),
                 lastQuestion_.toHtmlEscaped(),
                 QStringLiteral("生成失败: %1")
                     .arg(fromUtf8(outcome.generation_error).toHtmlEscaped()),
                 QStringLiteral("已降级为仅检索：上方结果列表与来源账本仍然有效。")));
        appendLog(QStringLiteral("问答: 生成失败（%1），本轮降级为仅检索。")
            .arg(fromUtf8(outcome.generation_error)));
        return;
    }

    // Citation rendering + sanity check: [n] inside 1..ledger size
    // becomes a clickable link to the ledger row and the highlighted
    // source chunk; anything else is flagged red and counted -- the
    // model's citation honesty stays visible instead of hiding it.
    const QString escaped = fromUtf8(outcome.answer).toHtmlEscaped();
    static const QRegularExpression citationRx(
        QStringLiteral("\\[(\\d{1,5})\\]"));

    QString body;
    qsizetype cursor = 0;
    int validCitations = 0;
    int invalidCitations = 0;
    auto matches = citationRx.globalMatch(escaped);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        body += escaped.mid(cursor, match.capturedStart() - cursor);
        const int number = match.captured(1).toInt();
        if (number >= 1 && number <= sourceCount) {
            ++validCitations;
            body += QStringLiteral("<a href=\"cite:%1\">[%1]</a>").arg(number);
        } else {
            ++invalidCitations;
            body += QStringLiteral("<span style=\"color:#b00020;\">[%1]</span>")
                .arg(number);
        }
        cursor = match.capturedEnd();
    }
    body += escaped.mid(cursor);
    body.replace(QChar('\n'), QStringLiteral("<br>"));

    answerBrowser_->setHtml(QStringLiteral(
        "<p><b>%1</b> %2</p><hr>%3<hr>"
        "<p style=\"color:gray;\">%4 · 生成 %5 秒 · 引用 %6 有效 / %7 越界</p>")
        .arg(QStringLiteral("问:"),
             lastQuestion_.toHtmlEscaped(),
             body,
             fromUtf8(outcome.generation_model),
             QString::number(outcome.generation_seconds, 'f', 1))
        .arg(validCitations)
        .arg(invalidCitations));

    chatStageLabel_->setText(QStringLiteral("完成: 生成 %1 秒 · 引用 %2 有效 / %3 越界")
        .arg(outcome.generation_seconds, 0, 'f', 1)
        .arg(validCitations)
        .arg(invalidCitations));
    appendLog(QStringLiteral("问答: %1 秒出答，引用 %2 有效 / %3 越界。")
        .arg(outcome.generation_seconds, 0, 'f', 1)
        .arg(validCitations)
        .arg(invalidCitations));
}

void MainWindow::onAnswerAnchorClicked(const QUrl& url) {
    const QString reference = url.toString();
    if (!reference.startsWith(QStringLiteral("cite:"))) {
        return;
    }
    bool parsed = false;
    const int number = reference.mid(static_cast<qsizetype>(5)).toInt(&parsed);
    if (!parsed || !lastResult_) {
        return;
    }
    const auto& citations = lastResult_->outcome.citations;
    if (number < 1 || number > static_cast<int>(citations.size())) {
        return;
    }
    const Citation& citation = citations[static_cast<size_t>(number - 1)];
    sourcesTable_->selectRow(number - 1);
    highlightChunk(citation.location);
    appendLog(QStringLiteral("引用: [%1] -> chunk %2。")
        .arg(number)
        .arg(citation.chunk_id));
}

void MainWindow::onSourceRowClicked(int row, int column) {
    Q_UNUSED(column);
    const QTableWidgetItem* numberItem = sourcesTable_->item(row, 0);
    if (numberItem == nullptr || !lastResult_) {
        return;
    }
    const int chunkId = numberItem->data(kRoleChunkId).toInt();
    for (const Citation& citation : lastResult_->outcome.citations) {
        if (citation.chunk_id == chunkId) {
            highlightChunk(citation.location);
            return;
        }
    }
}

void MainWindow::deleteSelectedDocument() {
    if (chatBusy_) {
        appendLog(QStringLiteral("删除: 问答仍在进行，请等本轮结束后再操作。"));
        return;
    }
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
    const QString path = fromUtf8(docs_[static_cast<size_t>(documentId)].path);
    if (docs_[static_cast<size_t>(documentId)].deleted) {
        appendLog(QStringLiteral("删除: %1 已经是墓碑状态。").arg(path));
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("确认软删除"),
        QStringLiteral("将文档标记为已删除并立即重建索引：\n%1").arg(path),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    ProgressRelay relay(this, fromUtf8(engine_.config().dense_model));
    const MutationOutcome mutation = engine_.remove_document(documentId,
        [&relay](const EngineProgress& progress) { return relay(progress); });
    relay.hide();

    if (mutation.status != MutationStatus::OK) {
        appendLog(QStringLiteral("删除: 引擎拒绝了这次操作（状态 %1）。")
            .arg(static_cast<int>(mutation.status)));
        return;
    }

    status_ = engine_.status();
    engineReady_ = status_.loaded;
    refreshDocumentTree();
    resultsTable_->setRowCount(0);
    refreshStatusBar();

    appendLog(status_.index_saved
        ? QStringLiteral("删除: %1 -> 墓碑 + 重建 + 保存完成。").arg(path)
        : QStringLiteral("删除: %1 -> 内存已更新，但快照保存失败。").arg(path));
    appendDenseReport(status_, QStringLiteral("删除重建"));
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
    if (rankItem == nullptr || !lastResult_) {
        return;
    }
    const int chunkId = rankItem->data(kRoleChunkId).toInt();
    for (const QueryHit& hit : lastResult_->outcome.hits) {
        if (hit.chunk_id == chunkId) {
            highlightChunk(hit.location);
            return;
        }
    }
}

void MainWindow::updateDeletedVisibility(bool showDeleted) {
    for (int i = 0; i < documentTree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = documentTree_->topLevelItem(i);
        int visibleChildren = 0;
        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem* item = category->child(j);
            const int documentId = item->data(0, kRoleDocumentId).toInt();
            const bool validId = documentId >= 0 &&
                static_cast<size_t>(documentId) < docs_.size();
            const bool hide = validId &&
                docs_[static_cast<size_t>(documentId)].deleted && !showDeleted;
            item->setHidden(hide);
            if (!hide) {
                ++visibleChildren;
            }
        }
        category->setHidden(visibleChildren == 0);
    }

    if (!chatBusy_ && !queryEdit_->text().trimmed().isEmpty()) {
        startRetrieval();
    }
}

void MainWindow::refreshDocumentTree() {
    docs_ = engine_.documents();
    documentTree_->clear();

    QHash<QString, QTreeWidgetItem*> categories;
    for (const DocInfo& doc : docs_) {
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
        item->setText(1, QString::number(static_cast<qulonglong>(doc.bytes)));
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
    docsLabel_->setText(QStringLiteral("docs: %1")
        .arg(static_cast<qulonglong>(status_.documents)));
    chunksLabel_->setText(QStringLiteral("chunks: %1")
        .arg(static_cast<qulonglong>(status_.chunks)));
    denseLabel_->setText(outcomeDenseState(status_));
    snapshotLabel_->setText(QStringLiteral("snapshot: %1").arg(
        status_.index_restored
            ? QStringLiteral("ok (restored)")
            : QStringLiteral("%1%2").arg(snapshotStatusName(status_.index_state),
                status_.index_saved ? QString() : QStringLiteral(" / save failed"))));
    versionsLabel_->setText(QStringLiteral("format/pipeline: %1/%2")
        .arg(FORMAT_VERSION)
        .arg(PIPELINE_VERSION));
    avgdlLabel_->setText(QStringLiteral("avgdl: %1")
        .arg(status_.avg_chunk_length, 0, 'f', 2));
}

void MainWindow::showDocument(int documentId) {
    std::string content;
    if (!engine_.document_text(documentId, content)) {
        return;
    }

    previewEdit_->setPlainText(fromUtf8(content));
    previewDocumentId_ = documentId;
    previewEdit_->moveCursor(QTextCursor::Start);

    for (const DocInfo& doc : docs_) {
        if (doc.id == documentId) {
            statusBar()->showMessage(fromUtf8(doc.path), 4000);
            break;
        }
    }
}

void MainWindow::highlightChunk(const ChunkLocation& location) {
    std::string content;
    if (!engine_.document_text(location.document_id, content)) {
        return;
    }
    if (location.begin > location.end || location.end > content.size()) {
        appendLog(QStringLiteral("定位失败: chunk 字节区间无效。"));
        return;
    }

    if (previewDocumentId_ != location.document_id) {
        showDocument(location.document_id);
    }

    const QString prefix = QString::fromUtf8(
        content.data(), static_cast<qsizetype>(location.begin));
    const QString selected = QString::fromUtf8(
        content.data() + location.begin,
        static_cast<qsizetype>(location.end - location.begin));

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
