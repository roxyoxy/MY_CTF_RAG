/* TraceRAG web UI -- vanilla JS, no build step.
 *
 * Endpoints:
 *   GET  /api/status          engine facts
 *   POST /api/search {query}  ranked hits
 *   POST /api/ask    {query}  hits + answer + citation ledger
 *   GET  /api/doc?id=N        full document text
 *
 * Two notes worth keeping:
 *
 * 1. `begin` / `end` on a hit are UTF-8 BYTE offsets into the document,
 *    not character indices. Highlighting therefore slices the encoded
 *    bytes and decodes each piece separately -- the same trick the Qt
 *    front end uses in highlightChunk.
 * 2. "生成答案" makes two calls: /api/search first so the hits land in
 *    ~0.3s, then /api/ask for the answer. That is the two-phase engine
 *    contract used as designed.
 */

const $ = (id) => document.getElementById(id);

// Crosshair glyph for the per-row locate button.
const TARGET_SVG =
  '<svg viewBox="0 0 16 16" width="14" height="14" aria-hidden="true">' +
  '<circle cx="8" cy="8" r="4.6" fill="none" stroke="currentColor" stroke-width="1.4"/>' +
  '<path d="M8 1.4v2.6M8 12v2.6M1.4 8h2.6M12 8h2.6" stroke="currentColor" ' +
  'stroke-width="1.4" stroke-linecap="round"/></svg>';

const MODE_SEARCH = "search";   // click a row -> expand in place
const MODE_ASK = "ask";         // click a row -> open the detail pane

let CURRENT_MODE = MODE_SEARCH;
let LAST_RESULTS = [];
let DETAIL = null;              // { docId, begin, end } for the jump button

async function api(path, body) {
  const opts = body
    ? { method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body) }
    : {};
  const response = await fetch(path, opts);
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;").replace(/</g, "&lt;")
    .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

// Scroll one element INSIDE another, without dragging the page:
// scrollIntoView() walks up to the nearest scrollable ancestor, which
// moved the whole results column instead of just the open document.
function scrollMarkIntoBox(box, mark) {
  if (!box || !mark) return;
  const boxRect = box.getBoundingClientRect();
  const markRect = mark.getBoundingClientRect();
  box.scrollTop += (markRect.top - boxRect.top) -
                   (box.clientHeight - markRect.height) / 2;
}

/* ---------- byte-offset helpers (see note 1) ---------- */
function highlightRange(text, begin, end) {
  const bytes = new TextEncoder().encode(text);
  if (begin > end || end > bytes.length) return escapeHtml(text);
  const before = new TextDecoder().decode(bytes.subarray(0, begin));
  const hit = new TextDecoder().decode(bytes.subarray(begin, end));
  const after = new TextDecoder().decode(bytes.subarray(end));
  return escapeHtml(before) + "<mark>" + escapeHtml(hit) + "</mark>" +
         escapeHtml(after);
}

/* ---------- settings ---------- */
const THEME_KEY = "traceRAG.theme";

function loadTheme() {
  try { return localStorage.getItem(THEME_KEY) || "system"; }
  catch (error) { return "system"; }
}

// "system" means no attribute at all, so the CSS media query decides;
// an explicit choice sets data-theme and wins over it.
function applyTheme(theme) {
  const root = document.documentElement;
  if (theme === "system") root.removeAttribute("data-theme");
  else root.setAttribute("data-theme", theme);
  document.querySelectorAll("#theme-seg button").forEach((button) => {
    button.classList.toggle("on", button.dataset.theme === theme);
  });
  try { localStorage.setItem(THEME_KEY, theme); } catch (error) { /* ignore */ }
}

function closeSettings() { $("settings-menu").classList.add("hidden"); }

/* ---------- status ---------- */
async function refreshStatus() {
  const badge = $("mode-badge");
  try {
    const s = await api("/api/status");
    const hybrid = s.mode === "hybrid";
    badge.textContent = hybrid ? "hybrid 检索" : "BM25-only（语义路已降级）";
    badge.classList.toggle("bm25", !hybrid);
    $("corpus-info").textContent =
      `${s.docs} 文档 · ${s.chunks} chunks · ${s.dense_model}`;
  } catch (error) {
    badge.textContent = "引擎未连接";
    badge.classList.add("bm25");
    $("corpus-info").textContent = "";
  }
}

/* ---------- search history (localStorage) ---------- */
const HISTORY_KEY = "traceRAG.history";
const HISTORY_MAX = 30;

// Entries are { act, q } so a replayed lookup runs the same action it
// was made with. Plain strings from the first version are read as
// searches instead of being dropped.
function loadHistory() {
  let raw;
  try { raw = JSON.parse(localStorage.getItem(HISTORY_KEY)) || []; }
  catch (error) { return []; }
  return raw.map((entry) =>
    typeof entry === "string" ? { act: "search", q: entry } : entry);
}

// Entries carry the response payload, so opening one is instant and
// offline. Payloads are big (whole chunks, whole answers), so the write
// sheds the oldest entries until it fits; if not even one fits, the
// queries are kept without their payloads and a click re-runs instead.
function saveHistory(list) {
  let items = list.slice(0, HISTORY_MAX);
  while (items.length > 1) {
    try {
      localStorage.setItem(HISTORY_KEY, JSON.stringify(items));
      return;
    } catch (error) {
      items = items.slice(0, items.length - 1);
    }
  }
  try {
    localStorage.setItem(HISTORY_KEY, JSON.stringify(
      items.map((e) => ({ act: e.act, q: e.q }))));
  } catch (error) { /* private mode: history is a convenience */ }
}

function pushHistory(act, query, data) {
  const list = loadHistory().filter((e) => !(e.act === act && e.q === query));
  list.unshift({ act, q: query, t: Date.now(), data });
  saveHistory(list);
  renderHistory();
}

// Replays a stored response. Nothing is re-requested: the point of
// keeping history is to get the old answer back, not to run it again.
function showStored(entry) {
  $("pane-detail").classList.add("hidden");
  if (entry.act === "ask") {
    $("results").innerHTML = "";
    $("empty-state").classList.add("hidden");
    CURRENT_MODE = MODE_ASK;
    rememberResults(entry.data);
    renderAnswer(entry.data);
  } else {
    $("answer-panel").classList.add("hidden");
    renderResults(entry.data, MODE_SEARCH);
  }
}

function renderHistory() {
  const list = loadHistory();
  $("history").innerHTML = list.map((e, i) =>
    `<li data-i="${i}" title="${escapeHtml(e.q)}">` +
    `<span class="hist-act ${e.act}">${e.act === "ask" ? "生成" : "检索"}：</span>` +
    `${escapeHtml(e.q)}</li>`).join("");
  $("history-empty").classList.toggle("hidden", list.length > 0);
}

/* ---------- result rows ---------- */
function chip(name, value) {
  if (value === null || value === undefined) {
    return `<span class="chip miss">${name} ·</span>`;
  }
  const shown = typeof value === "number" ? value.toFixed(3) : value;
  return `<span class="chip">${name}=<b>${shown}</b></span>`;
}

// Keeps the last payload for the detail pane and the timing line,
// without drawing anything -- generate mode shows no result list.
function rememberResults(data) {
  LAST_RESULTS = data.results;
  $("search-time").textContent =
    `${data.mode} · ${data.ms.toFixed(1)} ms · ${data.results.length} 条`;
}

function renderResults(data, mode) {
  CURRENT_MODE = mode;
  rememberResults(data);
  const box = $("results");
  box.innerHTML = "";
  $("empty-state").classList.add("hidden");

  data.results.forEach((hit, index) => {
    const row = document.createElement("div");
    row.className = "hit";
    row.dataset.index = index;
    row.innerHTML = `
      <div class="hit-head">
        <span class="caret"></span>
        <span class="rank">${hit.rank}</span>
        <span class="path">${escapeHtml(hit.doc)}</span>
        <span class="id">#${hit.doc_id}</span>
        ${chip("rrf", hit.rrf)} ${chip("bm25", hit.bm25)} ${chip("vec", hit.vec)}
        <span class="spacer"></span>
        <button class="target-btn" title="定位到命中片段">${TARGET_SVG}</button>
      </div>
      <div class="hit-snippet">${escapeHtml(hit.text)}</div>
      <div class="hit-full hidden"></div>`;
    box.appendChild(row);
  });
}

// Expanded body of one row. Fetched lazily: most rows are never opened,
// and a full document is far bigger than the snippet.
async function fillRowFull(row, hit) {
  const body = row.querySelector(".hit-full");
  if (body.dataset.loaded === "1") return;
  body.innerHTML = `<pre><span class="spin"></span>正在读取全文…</pre>`;
  try {
    const doc = await api(`/api/doc?id=${hit.doc_id}`);
    body.innerHTML = `<pre>${highlightRange(doc.content, hit.begin, hit.end)}</pre>`;
    body.dataset.loaded = "1";
  } catch (error) {
    body.innerHTML = `<pre>读取失败: ${escapeHtml(error.message)}</pre>`;
  }
}

async function expandRow(row, hit) {
  row.classList.add("open");
  row.querySelector(".hit-full").classList.remove("hidden");
  await fillRowFull(row, hit);
}

// The crosshair jumps straight to the matched span: expand (if needed)
// and scroll the <mark> into view.
async function locateInRow(row) {
  const hit = LAST_RESULTS[Number(row.dataset.index)];
  if (!hit) return;
  if (CURRENT_MODE === MODE_ASK) {
    openDetail(hit);
    return;
  }
  await expandRow(row, hit);
  const full = row.querySelector(".hit-full");
  scrollMarkIntoBox(full.querySelector("pre"), full.querySelector("mark"));
}

async function onHitHeadClicked(row) {
  const hit = LAST_RESULTS[Number(row.dataset.index)];
  if (!hit) return;

  // Ask mode: a click opens the detail pane instead of expanding here,
  // so the answer, the ranking and the source text stay visible at once.
  if (CURRENT_MODE === MODE_ASK) {
    openDetail(hit);
    return;
  }

  // Rows open and close independently, so several documents can be
  // compared side by side.
  if (row.classList.contains("open")) {
    row.classList.remove("open");
    row.querySelector(".hit-full").classList.add("hidden");
    return;
  }
  await expandRow(row, hit);
}

/* ---------- detail pane ---------- */
async function openDetail(hit) {
  const pane = $("pane-detail");
  pane.classList.remove("hidden");

  const score = [];
  if (hit.rrf !== undefined) score.push("rrf=" + hit.rrf.toFixed(4));
  if (hit.bm25) score.push("bm25=" + hit.bm25.toFixed(2));
  if (hit.vec) score.push("vec=" + hit.vec.toFixed(3));

  $("detail-path").textContent = hit.doc;
  $("detail-id").textContent = "#" + hit.doc_id + " · chunk " + hit.chunk_id;
  $("detail-score").textContent = score.join("  ");

  DETAIL = { docId: hit.doc_id, begin: hit.begin, end: hit.end };

  const body = $("detail-body");
  body.textContent = "正在读取全文…";
  try {
    const doc = await api(`/api/doc?id=${hit.doc_id}`);
    body.innerHTML = highlightRange(doc.content, hit.begin, hit.end);
  } catch (error) {
    body.textContent = "读取失败: " + error.message;
  }
}

// The full text is long; the crosshair scrolls to the highlighted span
// inside the pane, leaving the rest of the layout where it was.
function jumpToDetailHit() {
  const body = $("detail-body");
  scrollMarkIntoBox(body, body.querySelector("mark"));
}

/* ---------- answer ---------- */
function renderAnswer(data) {
  const panel = $("answer-panel");
  panel.classList.remove("hidden");

  if (data.answer === null || data.answer === undefined) {
    const why = data.gen_error
      ? `生成失败（${escapeHtml(data.gen_error)}）`
      : "生成不可用（模型未就绪）";
    $("answer-body").innerHTML =
      `${why} —— 下方的检索列表即为答案（降级模式）。`;
    $("gen-meta").textContent = data.mode;
    $("sources").innerHTML = "";
    return;
  }

  const ledger = data.sources || [];
  const known = new Set(ledger.map((s) => s.n));
  let bad = 0;
  const body = escapeHtml(data.answer).replace(/\[(\d+)\]/g, (_, digits) => {
    const n = Number(digits);
    if (known.has(n)) return `<span class="cite" data-n="${n}">[${n}]</span>`;
    bad += 1;
    return `<span class="cite bad" title="账本里没有这一条">[${n}]</span>`;
  });
  $("answer-body").innerHTML = body;

  $("gen-meta").textContent =
    `${data.gen_model} · ${data.gen_seconds.toFixed(1)}s · ${data.mode}` +
    (bad > 0 ? ` · ${bad} 处引用越界` : "");

  $("sources").innerHTML = `<h3>sources（出处账本）</h3>` +
    ledger.map((s) => `
      <div class="source-item" data-n="${s.n}" data-doc="${s.doc_id}"
           data-begin="${s.begin}" data-end="${s.end}" data-chunk="${s.chunk_id}">
        <span class="n">[${s.n}]</span>
        <span class="path">${escapeHtml(s.source)}</span>
        <span class="chunk">chunk ${s.chunk_id}</span>
      </div>`).join("");
}

// Find the hit that produced a citation, so the detail pane can show the
// same scores the ranking did.
function hitForChunk(chunkId) {
  return LAST_RESULTS.find((h) => h.chunk_id === chunkId) || null;
}

function openDetailFromSource(item) {
  const chunkId = Number(item.dataset.chunk);
  const hit = hitForChunk(chunkId);
  if (hit) { openDetail(hit); return; }
  openDetail({
    doc: item.querySelector(".path").textContent,
    doc_id: Number(item.dataset.doc),
    chunk_id: chunkId,
    begin: Number(item.dataset.begin),
    end: Number(item.dataset.end),
    rrf: undefined, bm25: null, vec: null,
  });
}

/* ---------- actions ---------- */
// The split button's current action. The label shows it, so the
// button never says a generic "send".
let ACTION = "search";

function setAction(action) {
  ACTION = action;
  $("btn-act").textContent = action === "ask" ? "生成" : "检索";
}

function runAction() {
  if (ACTION === "ask") doAsk(); else doSearch();
}

function setBusy(busy) {
  $("btn-act").disabled = busy;
  $("btn-menu").disabled = busy;
}

async function doSearch() {
  const query = $("query").value.trim();
  if (!query) return;
  $("query").value = "";   // the query lives on in the history
  setBusy(true);
  try {
    $("answer-panel").classList.add("hidden");
    $("pane-detail").classList.add("hidden");
    const data = await api("/api/search", { query });
    renderResults(data, MODE_SEARCH);
    pushHistory("search", query, data);
  } catch (error) {
    $("search-time").textContent = "请求失败: " + error.message;
  } finally {
    setBusy(false);
  }
}

async function doAsk() {
  const query = $("query").value.trim();
  if (!query) return;
  $("query").value = "";   // the query lives on in the history
  setBusy(true);

  // Generate mode shows the answer and its citation ledger only --
  // the ledger already lists every source, so a second ranking list
  // below it would just repeat itself.
  $("results").innerHTML = "";
  $("empty-state").classList.add("hidden");
  $("pane-detail").classList.add("hidden");
  $("answer-panel").classList.remove("hidden");
  $("answer-body").innerHTML =
    `<span class="spin"></span>本地模型生成中（约 8–10 秒）…`;
  $("gen-meta").textContent = "";
  $("sources").innerHTML = "";

  try {
    // Phase 1: keep the hits (the detail pane shows their scores) but
    // draw nothing; phase 2 answers.
    CURRENT_MODE = MODE_ASK;
    rememberResults(await api("/api/search", { query }));
    // Phase 2: the grounded answer (its payload carries the hits too,
    // so it is what the history entry stores).
    const data = await api("/api/ask", { query });
    renderAnswer(data);
    pushHistory("ask", query, data);
  } catch (error) {
    $("answer-body").textContent = "请求失败: " + error.message;
  } finally {
    setBusy(false);
  }
}

/* ---------- wire up ---------- */
function closeSendMenu() {
  $("send-menu").classList.add("hidden");
}

// Left half runs the current action; right half opens the menu that
// switches it. Enter is the fast path and runs whatever the label says.
$("btn-act").addEventListener("click", (event) => {
  event.stopPropagation();
  closeSendMenu();
  runAction();
});

$("btn-menu").addEventListener("click", (event) => {
  event.stopPropagation();   // otherwise the document handler re-closes it
  closeSettings();
  $("send-menu").classList.toggle("hidden");
});

$("send-menu").addEventListener("click", (event) => {
  const item = event.target.closest("button[data-act]");
  if (!item) return;
  closeSendMenu();
  if (item.dataset.act === "search") {
    setAction("search");
    doSearch();
  } else {
    setAction("ask");
    doAsk();
  }
});

// Settings: same click-outside pattern as the send menu, and each one
// closes the other so only one dropdown is ever open.
$("btn-settings").addEventListener("click", (event) => {
  event.stopPropagation();
  closeSendMenu();
  $("settings-menu").classList.toggle("hidden");
});

$("theme-seg").addEventListener("click", (event) => {
  const button = event.target.closest("button[data-theme]");
  if (button) applyTheme(button.dataset.theme);
});

document.addEventListener("click", () => {
  closeSendMenu();
  closeSettings();
});

$("query").addEventListener("keydown", (event) => {
  if (event.key !== "Enter") return;
  closeSendMenu();
  if (event.ctrlKey) doAsk(); else runAction();
});

$("btn-sidebar").addEventListener("click", () => {
  $("sidebar").classList.toggle("collapsed");
});

$("btn-clear-history").addEventListener("click", () => {
  try { localStorage.removeItem(HISTORY_KEY); } catch (error) { /* ignore */ }
  renderHistory();
});

$("history").addEventListener("click", (event) => {
  const item = event.target.closest("li");
  if (!item) return;
  const entry = loadHistory()[Number(item.dataset.i)];
  if (!entry) return;
  $("query").value = entry.q;
  setAction(entry.act);
  closeSendMenu();
  if (entry.data) showStored(entry);
  else runAction();   // an entry saved before payloads existed
});

$("results").addEventListener("click", (event) => {
  const row = event.target.closest(".hit");
  if (!row) return;
  if (event.target.closest(".target-btn")) {
    locateInRow(row);
    return;
  }
  if (event.target.closest(".hit-head")) onHitHeadClicked(row);
});

$("answer-body").addEventListener("click", (event) => {
  const cite = event.target.closest(".cite");
  if (!cite || cite.classList.contains("bad")) return;
  const row = document.querySelector(`.source-item[data-n="${cite.dataset.n}"]`);
  if (row) openDetailFromSource(row);
});

$("sources").addEventListener("click", (event) => {
  const item = event.target.closest(".source-item");
  if (item) openDetailFromSource(item);
});

$("btn-detail-close").addEventListener("click", () => {
  $("pane-detail").classList.add("hidden");
});
$("btn-jump").addEventListener("click", jumpToDetailHit);

renderHistory();
applyTheme(loadTheme());
refreshStatus();
setInterval(refreshStatus, 10000);

/* Still open (see the task list):
 *   - keyword highlighting in the snippet (escape first, then wrap)
 *   - a visible degradation banner when mode flips to bm25-only
 *   - a manual dark/light toggle (the CSS already follows the system) */
