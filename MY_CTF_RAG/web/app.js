/* TraceRAG web UI -- vanilla JS, no build step.
 *
 * Endpoints:
 *   GET  /api/status          engine facts
 *   POST /api/search {query}  ranked hits
 *   POST /api/ask    {query}  hits + answer + citation ledger
 *   GET  /api/doc?id=N        full document text
 *
 * Three notes worth keeping:
 *
 * 1. `begin` / `end` on a hit are UTF-8 BYTE offsets into the document,
 *    not character indices. Highlighting therefore goes through the
 *    encoded bytes -- the same trick the Qt front end uses in
 *    highlightChunk.
 * 2. "生成答案" makes two calls: /api/search first so the hits land in
 *    ~0.3s, then /api/ask for the answer. That is the two-phase engine
 *    contract used as designed.
 * 3. A document and an answer are rendered as markdown before they reach
 *    the DOM, by the renderer in the markdown section below. The hit
 *    highlight survives the trip: the renderer carries every source
 *    character's offset through to its output, so <mark> still lands on
 *    the matched character rather than on the block containing it.
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
  if (!response.ok) {
    // A first run embeds the corpus before anything can be asked;
    // "starting" is a state to wait out, not a failure to report.
    const message = data.error === "starting"
      ? "引擎正在建立索引，请稍候…"
      : (data.error || `HTTP ${response.status}`);
    throw new Error(message);
  }
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

/* ---------- markdown (see note 3) ----------
 *
 * Hand-written, because this project is offline by definition: a CDN copy
 * of marked.js would 404 the moment the demo machine is unplugged, which
 * is the exact situation the project exists for.
 *
 * It covers what this corpus actually contains. A census over the 58
 * corpus files, fenced blocks excluded, found 569 headings, 483 table
 * rows in 75 tables, 319 "- " items, 206 block quotes, 652 bold spans,
 * 824 code spans, 565 links, 31 images -- and zero "* " items and zero
 * ordered lists. Those last two are supported anyway: they cost a few
 * lines, and a writeup that uses them must not render as prose.
 *
 * Two rules hold everywhere below.
 *
 *  1. Source text is escaped before any tag this file emits is added.
 *     The corpus is third-party writeups and the answers come from a
 *     local model; neither is trusted. No character of either can ever
 *     become markup, and hrefs go through a scheme whitelist.
 *
 *  2. Everything emitted carries its source character offset, so <mark>
 *     lands on the exact character the engine matched. That is why the
 *     emit helpers all take an offset -- leaving it out would be
 *     simpler, but the hit highlight would fall back from
 *     character-precise to block-precise.
 *
 * Marks are clipped per source line and never across one. A chunk is cut
 * on atom boundaries, so it can begin mid-line and end mid-line, and the
 * line it landed in may be a heading, a table cell or a code line.
 * Clipping at the line is what keeps every <mark> inside the element it
 * was opened in, whatever block structure it turned out to be.
 */

const MD_ENCODER = new TextEncoder();
const MD_DECODER = new TextDecoder();

// The engine speaks UTF-8 byte offsets; the DOM speaks UTF-16 indices.
// A cut inside a multi-byte character decodes to U+FFFD, one UTF-16 unit
// -- the same as the BMP character it replaced -- so text up to U+FFFF
// converts exactly. Chunk boundaries land on atom boundaries and never
// split a character in the first place.
function charIndexAtByte(text, byteOffset) {
  if (byteOffset <= 0) return 0;
  const bytes = MD_ENCODER.encode(text);
  if (byteOffset >= bytes.length) return text.length;
  return MD_DECODER.decode(bytes.subarray(0, byteOffset)).length;
}

// The hit's byte range, in the character offsets the renderer works in.
function charRange(text, begin, end) {
  return { begin: charIndexAtByte(text, begin), end: charIndexAtByte(text, end) };
}

// The part of a hit that falls inside one line, or null when it does not.
function clipMark(mark, start, end) {
  if (!mark) return null;
  const from = Math.max(mark.begin, start);
  const to = Math.min(mark.end, end);
  return from < to ? { from: from, to: to } : null;
}

// Escapes one piece of source text sitting at character offset `base`,
// wrapping the clipped part of it in <mark>. `filter` may rewrite the
// escaped result -- the answer panel uses it to turn [n] into a chip.
function emitText(text, base, clip, filter) {
  let html;
  if (!clip) {
    html = escapeHtml(text);
  } else {
    const from = Math.max(0, clip.from - base);
    const to = Math.min(text.length, clip.to - base);
    html = from < to
      ? escapeHtml(text.slice(0, from)) + "<mark>" +
        escapeHtml(text.slice(from, to)) + "</mark>" +
        escapeHtml(text.slice(to))
      : escapeHtml(text);
  }
  return filter ? filter(html) : html;
}

// Only http(s), mailto and page-relative targets survive. A writeup is
// third-party text, and `javascript:` in an href is the classic way a
// markdown renderer turns into an XSS hole.
function safeHref(url) {
  const trimmed = url.trim();
  if (!trimmed) return null;
  if (/^[a-z][a-z0-9+.-]*:/i.test(trimmed)) {
    return /^(https?|mailto):/i.test(trimmed) ? trimmed : null;
  }
  if (trimmed.startsWith("//")) return null;   // protocol-relative
  return trimmed;
}

// Inline spans. Each branch checks the closing delimiter before it
// commits, so an unmatched ** or ` is emitted as literal text instead of
// swallowing the rest of the line.
function mdInline(text, base, clip, filter) {
  let out = "";
  let plain = 0;      // start of the literal run not yet emitted
  let i = 0;

  const flush = (upto) => {
    if (upto > plain) {
      out += emitText(text.slice(plain, upto), base + plain, clip, filter);
    }
  };

  while (i < text.length) {
    const ch = text[i];

    // `code` -- literal inside, so a ** in here must not go bold, and a
    // [1] in here is an array index, not a citation.
    if (ch === "`") {
      const close = text.indexOf("`", i + 1);
      if (close > i + 1) {
        flush(i);
        out += "<code>" +
          emitText(text.slice(i + 1, close), base + i + 1, clip, null) +
          "</code>";
        i = close + 1;
        plain = i;
        continue;
      }
    }

    // ![alt](src) -- the corpus is offline, so an <img> would render as a
    // broken box. The alt text stands in, styled as a figure caption.
    if (ch === "!" && text[i + 1] === "[") {
      const image = /^!\[([^\]\n]*)\]\(([^)\s]*)\)/.exec(text.slice(i));
      if (image) {
        flush(i);
        out += '<span class="md-figure">' +
          emitText(image[1], base + i + 2, clip, null) + "</span>";
        i += image[0].length;
        plain = i;
        continue;
      }
    }

    // [text](href) -- an unsafe scheme keeps the text and drops the link.
    if (ch === "[") {
      const link = /^\[([^\]\n]+)\]\(([^)\s]*)\)/.exec(text.slice(i));
      if (link) {
        const href = safeHref(link[2]);
        flush(i);
        const inner = emitText(link[1], base + i + 1, clip, null);
        out += href
          ? '<a href="' + escapeHtml(href) +
            '" target="_blank" rel="noopener noreferrer">' + inner + "</a>"
          : inner;
        i += link[0].length;
        plain = i;
        continue;
      }
    }

    // **bold** and __bold__
    if ((ch === "*" && text[i + 1] === "*") ||
        (ch === "_" && text[i + 1] === "_")) {
      const close = text.indexOf(ch + ch, i + 2);
      if (close > i + 2) {
        flush(i);
        out += "<strong>" +
          mdInline(text.slice(i + 2, close), base + i + 2, clip, filter) +
          "</strong>";
        i = close + 2;
        plain = i;
        continue;
      }
    }

    // *italic* and _italic_
    if (ch === "*" || ch === "_") {
      const close = text.indexOf(ch, i + 1);
      if (close > i + 1) {
        flush(i);
        out += "<em>" +
          mdInline(text.slice(i + 1, close), base + i + 1, clip, filter) +
          "</em>";
        i = close + 1;
        plain = i;
        continue;
      }
    }

    i += 1;
  }

  flush(text.length);
  return out;
}

// CJK, CJK punctuation and fullwidth forms. Used only to decide whether a
// soft line break becomes a space or nothing at all.
// True for CJK ideographs, CJK punctuation and fullwidth forms. Only
// used to decide whether a soft line break becomes a space or nothing.
function mdIsCjk(ch) {
  if (!ch) return false;
  const code = ch.charCodeAt(0);
  return (code >= 0x3000 && code <= 0x303f) ||
         (code >= 0x3400 && code <= 0x4dbf) ||
         (code >= 0x4e00 && code <= 0x9fff) ||
         (code >= 0xff00 && code <= 0xffef);
}

// Renders a run of source lines as inline HTML, joining them the way
// markdown soft-wraps them. Markdown joins with a space, which is right
// for English and wrong for Chinese -- joining two Chinese lines with a
// space drops a gap into the middle of a sentence. The separator is
// decided from the source text, never from the HTML emitted so far,
// whose last character would be a tag half the time.
function mdFlow(parts, mark, filter) {
  let html = "";
  let lastChar = "";
  for (let k = 0; k < parts.length; k += 1) {
    const part = parts[k];
    if (k > 0) {
      const nextChar = part.text.replace(/^\s+/, "").charAt(0);
      html += (mdIsCjk(lastChar) && mdIsCjk(nextChar)) ? "" : " ";
    }
    html += mdInline(part.text, part.start,
      clipMark(mark, part.start, part.end), filter);
    lastChar = part.text.replace(/\s+$/, "").slice(-1);
  }
  return html;
}

// Lines, each remembering where it starts in the document. Line endings
// are CRLF -- the loader reads raw bytes and normalises nothing -- so the
// \r is dropped for parsing but still counted in the offsets. Skipping
// that count would drift every highlight past the first line by one byte
// per line, which is exactly the kind of bug that stays invisible until
// somebody opens a long document.
function mdLines(text) {
  const lines = [];
  let start = 0;
  for (;;) {
    let stop = text.indexOf("\n", start);
    if (stop === -1) stop = text.length;
    let body = text.slice(start, stop);
    if (body.endsWith("\r")) body = body.slice(0, -1);
    lines.push({ text: body, start: start, end: start + body.length });
    if (stop >= text.length) break;
    start = stop + 1;
  }
  return lines;
}

const MD_FENCE = /^\s*(`{3,}|~{3,})\s*([^\s`]*)\s*$/;
const MD_HEADING = /^(#{1,6})\s+(.*?)\s*#*\s*$/;
const MD_RULE = /^\s*([-*_])\s*(\1\s*){2,}$/;
const MD_ITEM = /^(\s*)([-*+]|\d+[.)])\s+/;

// A separator row is pipes, dashes, colons and spaces, with at least one
// dash. `|---|---|` and `| :--- | ---: |` both qualify.
function mdIsSeparator(text) {
  const trimmed = text.trim();
  return /^[|\-: ]+$/.test(trimmed) &&
         trimmed.includes("-") && trimmed.includes("|");
}

// Splits one table row into cells that remember their offsets. \| is a
// literal pipe inside a cell, so the split has to be escape-aware or a
// writeup documenting a shell pipeline breaks its own table.
//
// Offsets are absolute document offsets, not indices into the row: they
// are compared against the hit's range, which is absolute. Cell offsets
// are the one place that distinction is easy to lose, and losing it
// fails silently -- the table still renders, it just never highlights.
function mdCells(line) {
  let from = 0;
  let to = line.text.length;
  while (from < to && /\s/.test(line.text[from])) from += 1;
  while (to > from && /\s/.test(line.text[to - 1])) to -= 1;
  if (line.text[from] === "|") from += 1;
  if (to > from && line.text[to - 1] === "|") to -= 1;

  const cells = [];
  let text = "";
  let cellStart = line.start + from;
  let i = from;
  while (i < to) {
    if (line.text[i] === "\\" && line.text[i + 1] === "|") {
      text += "|";
      i += 2;
      continue;
    }
    if (line.text[i] === "|") {
      cells.push({ text: text, start: cellStart });
      text = "";
      cellStart = line.start + i + 1;
      i += 1;
      continue;
    }
    text += line.text[i];
    i += 1;
  }
  cells.push({ text: text, start: cellStart });
  return cells;
}

function mdTable(head, rows, mark, filter) {
  const cell = (c) => {
    const trimmed = c.text.trim();
    const at = c.start + c.text.indexOf(trimmed);
    return mdInline(trimmed, at,
      clipMark(mark, c.start, c.start + c.text.length), filter);
  };
  const headHtml = head.map((c) => "<th>" + cell(c) + "</th>").join("");
  const bodyHtml = rows.map((row) =>
    "<tr>" + row.map((c) => "<td>" + cell(c) + "</td>").join("") + "</tr>"
  ).join("");
  // The wrapper scrolls the table sideways on its own, so a wide table
  // never makes the whole page scroll.
  return '<div class="md-table-wrap"><table><thead><tr>' + headHtml +
    "</tr></thead><tbody>" + bodyHtml + "</tbody></table></div>";
}

// Renders a whole document, or a model answer, to HTML.
//   options.mark       { begin, end } in character offsets, or null
//   options.textFilter rewrites escaped text runs (citation chips)
function renderMarkdown(text, options) {
  const opts = options || {};
  const mark = opts.mark || null;
  const filter = opts.textFilter || null;
  const lines = mdLines(text);
  const out = [];
  let i = 0;

  while (i < lines.length) {
    const line = lines[i];

    if (!line.text.trim()) { i += 1; continue; }

    // ```lang ... ``` or ~~~lang ... ~~~
    const fence = MD_FENCE.exec(line.text);
    if (fence) {
      const marker = fence[1][0];
      const close = new RegExp(
        "^\\s*\\" + marker + "{" + fence[1].length + ",}\\s*$");
      let j = i + 1;
      const body = [];
      while (j < lines.length && !close.test(lines[j].text)) {
        body.push(lines[j]);
        j += 1;
      }
      const lang = fence[2]
        ? ' data-lang="' + escapeHtml(fence[2]) + '"' : "";
      const code = body.map((l) =>
        emitText(l.text, l.start, clipMark(mark, l.start, l.end), null)
      ).join("\n");
      out.push('<pre class="md-code"><code' + lang + ">" + code + "</code></pre>");
      i = j < lines.length ? j + 1 : j;
      continue;
    }

    const heading = MD_HEADING.exec(line.text);
    if (heading) {
      const level = heading[1].length;
      const content = heading[2];
      const at = line.start + heading[0].indexOf(content);
      out.push("<h" + level + ">" +
        mdInline(content, at, clipMark(mark, line.start, line.end), filter) +
        "</h" + level + ">");
      i += 1;
      continue;
    }

    if (MD_RULE.test(line.text)) {
      out.push("<hr>");
      i += 1;
      continue;
    }

    // > quoted line
    if (/^\s*>/.test(line.text)) {
      const parts = [];
      while (i < lines.length && /^\s*>/.test(lines[i].text)) {
        const lead = /^\s*>\s?/.exec(lines[i].text)[0].length;
        parts.push({
          text: lines[i].text.slice(lead),
          start: lines[i].start + lead,
          end: lines[i].end,
        });
        i += 1;
      }
      out.push("<blockquote>" + mdFlow(parts, mark, filter) + "</blockquote>");
      continue;
    }

    // a row of pipes over a separator row
    if (/^\s*\|/.test(line.text) && i + 1 < lines.length &&
        mdIsSeparator(lines[i + 1].text)) {
      const head = mdCells(line);
      i += 2;
      const rows = [];
      while (i < lines.length && lines[i].text.trim() &&
             /^\s*\|/.test(lines[i].text)) {
        rows.push(mdCells(lines[i]));
        i += 1;
      }
      out.push(mdTable(head, rows, mark, filter));
      continue;
    }

    // - item / * item / + item / 1. item
    const item = MD_ITEM.exec(line.text);
    if (item) {
      const ordered = /\d/.test(item[2]);
      const items = [];
      while (i < lines.length) {
        const head = MD_ITEM.exec(lines[i].text);
        if (!head || /\d/.test(head[2]) !== ordered) break;
        const lead = head[0].length;
        const parts = [{
          text: lines[i].text.slice(lead),
          start: lines[i].start + lead,
          end: lines[i].end,
        }];
        i += 1;
        // An indented line that is not a new item continues this one.
        while (i < lines.length && lines[i].text.trim() &&
               !MD_ITEM.test(lines[i].text) && /^\s{2,}/.test(lines[i].text)) {
          const trimmed = lines[i].text.trim();
          parts.push({
            text: trimmed,
            start: lines[i].start + lines[i].text.indexOf(trimmed),
            end: lines[i].end,
          });
          i += 1;
        }
        items.push("<li>" + mdFlow(parts, mark, filter) + "</li>");
      }
      out.push(ordered ? "<ol>" + items.join("") + "</ol>"
                       : "<ul>" + items.join("") + "</ul>");
      continue;
    }

    // everything else is a paragraph, up to the next blank line or the
    // start of another block
    const para = [];
    while (i < lines.length && lines[i].text.trim() &&
           !MD_FENCE.test(lines[i].text) && !MD_HEADING.test(lines[i].text) &&
           !MD_RULE.test(lines[i].text) && !MD_ITEM.test(lines[i].text) &&
           !/^\s*>/.test(lines[i].text) && !/^\s*\|/.test(lines[i].text)) {
      para.push(lines[i]);
      i += 1;
    }
    if (!para.length) { i += 1; continue; }
    out.push("<p>" + mdFlow(para, mark, filter) + "</p>");
  }

  return out.join("\n");
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
const STAGE_TEXT = { plan: "准备中", index: "重建词法索引", vectors: "嵌入向量" };

// Self-scheduling rather than setInterval: while the engine is still
// coming up the badge should move every couple of seconds, and once it
// is ready there is nothing to watch that often.
async function refreshStatus() {
  const badge = $("mode-badge");
  let nextDelay = 10000;
  try {
    const s = await api("/api/status");
    if (s.starting) {
      const name = STAGE_TEXT[s.stage] || "建立索引";
      badge.textContent = s.total > 0
        ? `${name} ${s.done}/${s.total}`
        : `${name}…`;
      badge.classList.add("bm25");
      badge.title = "";
      $("corpus-info").textContent = "";
      nextDelay = 2000;
    } else {
      const hybrid = s.mode === "hybrid";
      badge.textContent = hybrid ? "hybrid 检索" : "BM25-only";
      badge.classList.toggle("bm25", !hybrid);
      // A degraded session still answers every query -- it answers from
      // the lexical route alone. "BM25-only" is the badge's word for
      // that, which means nothing to someone who has not read the
      // engine, so the tooltip and the trailing note say it in plain
      // language: what is off, and what it costs.
      badge.title = hybrid
        ? "词法（BM25）与语义（向量）双路并行"
        : "语义（向量）检索未启用：本次会话只用语料词法匹配，" +
          "换个说法问同一件事的召回会变差";
      // Defensive: a payload missing the corpus fields must never
      // render "undefined documents". The starting payload has no
      // docs/chunks at all, and a degraded payload has no model name.
      const docs = s.docs ?? 0;
      const chunks = s.chunks ?? 0;
      const model = s.dense_model || "";
      const corpus = `${docs} 文档 · ${chunks} chunks` +
        (model ? ` · ${model}` : "");
      $("corpus-info").innerHTML = hybrid
        ? escapeHtml(corpus)
        : escapeHtml(corpus) + ' <span class="degraded">语义检索未启用</span>';
    }
  } catch (error) {
    badge.textContent = error.message === "引擎未连接"
      ? error.message : "引擎未连接";
    badge.classList.add("bm25");
    badge.title = "";
    $("corpus-info").textContent = "";
    nextDelay = 5000;
  }
  setTimeout(refreshStatus, nextDelay);
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
      <div class="hit-full md-scope hidden"></div>`;
    box.appendChild(row);
  });
}

// Expanded body of one row. Fetched lazily: most rows are never opened,
// and a full document is far bigger than the snippet.
async function fillRowFull(row, hit) {
  const body = row.querySelector(".hit-full");
  if (body.dataset.loaded === "1") return;
  body.innerHTML =
    `<div class="md-note"><span class="spin"></span>正在读取全文…</div>`;
  try {
    const doc = await api(`/api/doc?id=${hit.doc_id}`);
    body.innerHTML = renderMarkdown(doc.content, {
      mark: charRange(doc.content, hit.begin, hit.end),
    });
    body.dataset.loaded = "1";
  } catch (error) {
    body.innerHTML =
      `<div class="md-note">读取失败: ${escapeHtml(error.message)}</div>`;
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
  scrollMarkIntoBox(full, full.querySelector("mark"));
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
    body.innerHTML = renderMarkdown(doc.content, {
      mark: charRange(doc.content, hit.begin, hit.end),
    });
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
  // The chip goes in after the renderer has produced HTML, never before:
  // escaping first and patching afterwards would double-escape every tag
  // the renderer just emitted. textFilter only ever sees an escaped text
  // run, never a tag or a code span, so [n] can only match where a
  // citation actually belongs.
  const cite = (html) => html.replace(/\[(\d+)\]/g, (whole, digits) => {
    const n = Number(digits);
    if (known.has(n)) return `<span class="cite" data-n="${n}">[${n}]</span>`;
    bad += 1;
    return `<span class="cite bad" title="账本里没有这一条">[${n}]</span>`;
  });
  $("answer-body").innerHTML =
    renderMarkdown(data.answer, { textFilter: cite });

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
    `<span class="spin"></span>本地模型生成中…`;
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

// Theme picks are handled at the document level so they work even if the
// menu's own listener never attached; the menu deliberately stays open so
// both options can be tried without reopening it.
document.addEventListener("click", (event) => {
  const themeButton = event.target.closest("button[data-theme]");
  if (themeButton) {
    applyTheme(themeButton.dataset.theme);
    return;
  }
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
refreshStatus();   // schedules its own next run

/* Still open (see the task list):
 *   - keyword highlighting in the snippet (escape first, then wrap)
 *   - a visible degradation banner when mode flips to bm25-only
 *   - a manual dark/light toggle (the CSS already follows the system) */
