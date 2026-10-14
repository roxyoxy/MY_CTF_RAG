/* TraceRAG web UI -- vanilla JS, no build step.
 *
 * Talks to the C++ server over three endpoints:
 *   GET  /api/status | POST /api/search | POST /api/ask
 *
 * "生成答案" deliberately makes TWO calls: /api/search first (fast, the
 * hits land in ~0.3s) and then /api/ask (slow, ~10s of local model).
 * That is the two-phase engine contract used the way it was designed --
 * the user reads results while the model is still thinking, instead of
 * staring at a spinner for the whole answer. */

const $ = (id) => document.getElementById(id);

let LAST = null;             // last payload, for citation -> source jumps

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

/* ---------- status badge ---------- */
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

/* ---------- rendering ---------- */
function chip(name, value) {
  if (value === null || value === undefined) {
    return `<span class="chip miss">${name} ·</span>`;
  }
  const shown = typeof value === "number" ? value.toFixed(3) : value;
  return `<span class="chip">${name}=<b>${shown}</b></span>`;
}

function renderResults(data) {
  LAST = data;
  const box = $("results");
  box.innerHTML = "";
  $("empty-state").classList.add("hidden");
  $("search-time").textContent =
    `${data.mode} · ${data.ms.toFixed(1)} ms · ${data.results.length} 条`;

  for (const hit of data.results) {
    const card = document.createElement("div");
    card.className = "result glass";
    card.dataset.chunk = hit.chunk_id;
    card.innerHTML = `
      <div class="rank">${hit.rank}</div>
      <div class="pathline">
        <span class="path">${escapeHtml(hit.doc)}</span>
        ${chip("rrf", hit.rrf)} ${chip("bm25", hit.bm25)} ${chip("vec", hit.vec)}
      </div>
      <div class="snippet">${escapeHtml(hit.text)}</div>`;
    box.appendChild(card);
  }
}

function renderAnswer(data) {
  const panel = $("answer-panel");
  panel.classList.remove("hidden");

  if (data.answer === null || data.answer === undefined) {
    // Degradation is a design feature, not an error: the retrieval list
    // above IS the answer when the model is unavailable.
    const why = data.gen_error
      ? `生成失败（${escapeHtml(data.gen_error)}）`
      : "生成不可用（模型未就绪）";
    $("answer-body").innerHTML =
      `${why} —— 上方的检索列表即为答案（降级模式）。`;
    $("gen-meta").textContent = data.mode;
    $("sources").innerHTML = "";
    return;
  }

  const ledger = data.sources || [];
  const known = new Set(ledger.map((s) => s.n));

  // Citation sanity check: [n] inside the ledger becomes a jump link;
  // anything else is flagged red and counted, so the model's citation
  // honesty stays visible instead of silently broken.
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
      <div class="source-item" data-n="${s.n}" data-chunk="${s.chunk_id}">
        <span class="n">[${s.n}]</span>
        <span class="path">${escapeHtml(s.source)}</span>
        <span class="chunk">chunk ${s.chunk_id}</span>
      </div>`).join("");
}

/* ---------- interactions ---------- */
function jumpToChunk(chunkId) {
  const card = document.querySelector(`.result[data-chunk="${chunkId}"]`);
  if (!card) return;
  card.scrollIntoView({ behavior: "smooth", block: "center" });
  card.classList.add("flash");
  setTimeout(() => card.classList.remove("flash"), 1400);
}

function setBusy(busy) {
  $("btn-search").disabled = busy;
  $("btn-ask").disabled = busy;
}

async function doSearch() {
  const query = $("query").value.trim();
  if (!query) return;
  setBusy(true);
  try {
    renderResults(await api("/api/search", { query }));
    $("answer-panel").classList.add("hidden");
  } catch (error) {
    $("search-time").textContent = "请求失败: " + error.message;
  } finally {
    setBusy(false);
  }
}

async function doAsk() {
  const query = $("query").value.trim();
  if (!query) return;
  setBusy(true);

  const panel = $("answer-panel");
  panel.classList.remove("hidden");
  $("answer-body").innerHTML =
    `<span class="spin"></span>本地模型生成中（约 8–10 秒）…`;
  $("gen-meta").textContent = "";
  $("sources").innerHTML = "";

  try {
    // Phase 1: hits land almost immediately, so the page is useful while
    // the model works.
    renderResults(await api("/api/search", { query }));
    // Phase 2: the grounded answer.
    renderAnswer(await api("/api/ask", { query }));
  } catch (error) {
    $("answer-body").textContent = "请求失败: " + error.message;
  } finally {
    setBusy(false);
  }
}

/* ---------- wire up ---------- */
$("btn-search").addEventListener("click", doSearch);
$("btn-ask").addEventListener("click", doAsk);

$("query").addEventListener("keydown", (event) => {
  if (event.key !== "Enter") return;
  if (event.ctrlKey) doAsk(); else doSearch();
});

// [n] in the answer scrolls to that source row; a source row scrolls to
// the matching result card. Both directions, because the ledger and the
// ranking are two views of the same chunks.
$("answer-body").addEventListener("click", (event) => {
  const cite = event.target.closest(".cite");
  if (!cite) return;
  const row = document.querySelector(`.source-item[data-n="${cite.dataset.n}"]`);
  if (row) row.scrollIntoView({ behavior: "smooth", block: "center" });
});

$("sources").addEventListener("click", (event) => {
  const item = event.target.closest(".source-item");
  if (item) jumpToChunk(item.dataset.chunk);
});

refreshStatus();
setInterval(refreshStatus, 10000);   // keep the badge honest if Ollama drops

/* Extension notes (safe places to build on):
 *   - Dark/light switching: every colour is a :root variable in
 *     style.css; toggle a class on <html> and use localStorage.
 *   - Search history: push `query` into localStorage and render a
 *     dropdown on focus.
 *   - Keyword highlighting: wrap query terms in <mark> inside the
 *     snippet before escaping (escape first, then mark).
 *   - Three-route comparison view: ask the server for a per-route
 *     ranking (`/api/search` already returns bm25/vec per hit; a
 *     mode=bm25|dense parameter would show them side by side). */
