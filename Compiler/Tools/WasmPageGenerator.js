const fs = require("fs");
const path = require("path");

const PART_LIMIT = 50 * 1024;

function ReadArg(name) {
  const index = process.argv.indexOf(name);
  if (index < 0 || index + 1 >= process.argv.length) {
    throw new Error("Missing " + name);
  }
  return process.argv[index + 1];
}

function DecodeXml(value) {
  return value
    .replace(/&#(\d+);/g, (_, digits) => String.fromCharCode(Number(digits)))
    .replace(/&#x([0-9a-f]+);/gi, (_, digits) => String.fromCharCode(parseInt(digits, 16)))
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&quot;/g, "\"")
    .replace(/&apos;/g, "'")
    .replace(/&amp;/g, "&");
}

function EscapeHtml(value) {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function Attribute(raw, name) {
  const match = raw.match(new RegExp("\\b" + name + "\\s*=\\s*\"([^\"]*)\"", "i"));
  return match ? DecodeXml(match[1]) : "";
}

function PlainText(inner) {
  const stripped = inner.replace(/<[^>]+>/g, " ");
  return DecodeXml(stripped).replace(/\s+/g, " ").trim();
}

function ReadSeo(xmlPath) {
  const source = fs.readFileSync(xmlPath, "utf8").replace(/<!--[\s\S]*?-->/g, "");
  let pageTitle = "";
  const titleBar = source.match(/<TitleBar\b([^>]*)\/?>/i);
  if (titleBar) {
    pageTitle = Attribute(titleBar[1], "title");
  }

  const blocks = [];
  const pattern = /<(H[1-6]|Heading|Paragraph|Text)\b([^>]*)>([\s\S]*?)<\/\1>/gi;
  let match = pattern.exec(source);
  while (match) {
    const tag = match[1];
    const text = PlainText(match[3]);
    if (text) {
      let level = 0;
      if (/^H[1-6]$/i.test(tag)) {
        level = Number(tag.slice(1));
      } else if (/^Heading$/i.test(tag)) {
        level = Number(Attribute(match[2], "level") || "1");
        if (!Number.isFinite(level)) {
          level = 1;
        }
        level = Math.min(6, Math.max(1, level));
      }
      blocks.push({ level, text });
    }
    match = pattern.exec(source);
  }

  if (!pageTitle) {
    const firstHeading = blocks.find((block) => block.level >= 1);
    pageTitle = firstHeading ? firstHeading.text : "Robo";
  }
  const firstParagraph = blocks.find((block) => block.level === 0);
  const description = (firstParagraph ? firstParagraph.text : pageTitle).slice(0, 160);
  return { pageTitle, description, blocks };
}

function SeoHtml(blocks) {
  return blocks.map((block) => {
    const text = EscapeHtml(block.text);
    if (block.level >= 1) {
      return "    <h" + block.level + ">" + text + "</h" + block.level + ">";
    }
    return "    <p>" + text + "</p>";
  }).join("\n");
}

function WriteIndex(outDir, folder, seo) {
  const title = EscapeHtml(seo.pageTitle);
  const description = EscapeHtml(seo.description);
  const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>${title}</title>
  <meta name="description" content="${description}">
  <link rel="canonical" href="/${folder}/">
  <meta property="og:title" content="${title}">
  <meta property="og:description" content="${description}">
  <meta property="og:type" content="website">
  <style>
    html, body {
      margin: 0;
      width: 100%;
      min-height: 100%;
      overflow: hidden;
      background: #FFFFFF;
      color: #1D1D1F;
      font-family: Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    #robo-canvas {
      position: fixed;
      left: 0;
      top: 0;
      width: 100%;
      height: 100%;
      z-index: 1;
      display: block;
      outline: none;
      visibility: hidden;
      touch-action: manipulation;
      image-rendering: auto;
    }
    #robo-canvas.ready {
      visibility: visible;
    }
    #robo-loader {
      position: fixed;
      inset: 0;
      z-index: 2;
      display: flex;
      align-items: center;
      justify-content: center;
      box-sizing: border-box;
      background: #FFFFFF;
      user-select: none;
      pointer-events: none;
    }
    #robo-loader[hidden] {
      display: none;
    }
    .robo-loader-stack {
      display: flex;
      flex-direction: column;
      align-items: center;
      animation: robo-loading-in 0.45s cubic-bezier(0.28, 0.11, 0.32, 1);
    }
    .robo-logo {
      display: block;
    }
    .robo-progress {
      width: 160px;
      margin-top: 32px;
    }
    .robo-track {
      height: 6px;
      overflow: hidden;
      border-radius: 980px;
      background: #EDEDEF;
    }
    .robo-fill {
      height: 100%;
      width: 0%;
      border-radius: 980px;
      background: #0E7AFF;
      transition: width 0.3s cubic-bezier(0.28, 0.11, 0.32, 1);
    }
    .robo-percent {
      margin: 8px 0 0;
      text-align: center;
      font-size: 12px;
      line-height: 16px;
      font-weight: 500;
      letter-spacing: -0.01em;
      font-variant-numeric: tabular-nums;
      color: #1D1D1F;
    }
    .robo-status {
      margin: 24px 0 0;
      max-width: 280px;
      text-align: center;
      font-size: 15px;
      line-height: 20px;
      font-weight: 400;
      color: #8E8E93;
      transition: opacity 0.2s cubic-bezier(0.28, 0.11, 0.32, 1);
    }
    #robo-document {
      max-width: 720px;
      margin: 0 auto;
      padding: 48px 24px 96px;
    }
    #robo-document h1,
    #robo-document h2,
    #robo-document h3,
    #robo-document h4,
    #robo-document h5,
    #robo-document h6 {
      font-weight: 600;
      letter-spacing: -0.02em;
      line-height: 1.2;
    }
    #robo-document h1 { font-size: 32px; margin: 28px 0 12px; }
    #robo-document h2 { font-size: 24px; margin: 24px 0 10px; }
    #robo-document h3 { font-size: 20px; margin: 20px 0 8px; }
    #robo-document p { font-size: 17px; line-height: 1.47; margin: 0 0 12px; }
    @keyframes robo-loading-in {
      from { opacity: 0; transform: translateY(6px); }
      to { opacity: 1; transform: translateY(0); }
    }
    @media (prefers-reduced-motion: reduce) {
      .robo-loader-stack { animation: none; }
      .robo-fill, .robo-status { transition: none; }
    }
  </style>
</head>
<body>
  <div id="robo-loader" role="status" aria-live="polite" aria-busy="true" aria-label="Setting things up">
    <div class="robo-loader-stack">
      <svg class="robo-logo" width="56" height="56" viewBox="0 0 24 24" fill="none" aria-hidden="true">
        <rect x="3" y="3" width="18" height="18" rx="3" stroke="#8B5CF6" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></rect>
        <path d="M10 8L6 12L10 16" stroke="#10B981" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path>
        <path d="M14 8L18 12L14 16" stroke="#10B981" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path>
      </svg>
      <div class="robo-progress">
        <div class="robo-track" role="progressbar" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" aria-valuetext="0 percent">
          <div class="robo-fill" id="robo-fill"></div>
        </div>
        <p class="robo-percent" id="robo-percent">0%</p>
      </div>
      <p class="robo-status" id="robo-status">Setting things up…</p>
    </div>
  </div>
  <canvas id="robo-canvas" tabindex="0"></canvas>
  <main id="robo-document">
${SeoHtml(seo.blocks)}
  </main>
  <script>window.RoboManualBoot = true;</script>
  <script src="./App.js"></script>
</body>
</html>
`;
  fs.writeFileSync(path.join(outDir, "Index.html"), html);
}

function WriteApp(outDir, parts, wasmBytes, engineJs, packageUrl, packageBytes) {
  const app = `window.RoboManualBoot = true;

const PARTS = ${JSON.stringify(parts)};
const ENGINE_JS = ${JSON.stringify(engineJs)};
const PACKAGE_URL = ${JSON.stringify(packageUrl)};
const PACKAGE_BYTES = ${packageBytes};
const WASM_BYTES = ${wasmBytes};
const PART_CONCURRENCY = 8;

const loader = document.getElementById("robo-loader");
const fill = document.getElementById("robo-fill");
const percentNode = document.getElementById("robo-percent");
const statusNode = document.getElementById("robo-status");
const track = loader.querySelector("[role='progressbar']");
const pageCanvas = document.getElementById("robo-canvas");

let loadedBytes = 0;
let totalBytes = WASM_BYTES + PACKAGE_BYTES;
let shownPercent = 0;
let packageBuffer = null;

function SetProgress(percent) {
  const next = Math.max(0, Math.min(100, Math.round(percent)));
  if (next < shownPercent) {
    return;
  }
  shownPercent = next;
  fill.style.width = next + "%";
  percentNode.textContent = next + "%";
  track.setAttribute("aria-valuenow", String(next));
  track.setAttribute("aria-valuetext", next + " percent");
  loader.setAttribute("aria-label", next + " percent");
}

function NoteBytes(count) {
  loadedBytes += count;
  if (totalBytes > 0) {
    SetProgress(Math.min(99, (loadedBytes / totalBytes) * 100));
  }
}

function LoadScript(src) {
  return new Promise((resolve, reject) => {
    const script = document.createElement("script");
    script.src = src;
    script.onload = () => resolve();
    script.onerror = () => reject(new Error("Could not load " + src));
    document.body.appendChild(script);
  });
}

async function ReadBody(response) {
  const reader = response.body.getReader();
  const chunks = [];
  let size = 0;
  while (true) {
    const step = await reader.read();
    if (step.done) {
      break;
    }
    chunks.push(step.value);
    size += step.value.length;
    NoteBytes(step.value.length);
  }
  const bytes = new Uint8Array(size);
  let offset = 0;
  for (const chunk of chunks) {
    bytes.set(chunk, offset);
    offset += chunk.length;
  }
  return bytes;
}

async function FetchParts() {
  const results = new Array(PARTS.length);
  let cursor = 0;
  async function Worker() {
    while (cursor < PARTS.length) {
      const index = cursor;
      cursor += 1;
      const response = await fetch(PARTS[index], { cache: "no-store" });
      if (!response.ok) {
        throw new Error("Could not load " + PARTS[index]);
      }
      results[index] = await ReadBody(response);
    }
  }
  const workers = [];
  const count = Math.min(PART_CONCURRENCY, PARTS.length);
  for (let index = 0; index < count; index += 1) {
    workers.push(Worker());
  }
  await Promise.all(workers);
  let size = 0;
  for (const part of results) {
    size += part.length;
  }
  const wasm = new Uint8Array(size);
  let offset = 0;
  for (const part of results) {
    wasm.set(part, offset);
    offset += part.length;
  }
  return wasm;
}

async function FetchPackage() {
  if (!PACKAGE_URL || PACKAGE_BYTES <= 0) {
    return;
  }
  const response = await fetch(PACKAGE_URL, { cache: "no-store" });
  if (!response.ok) {
    throw new Error("Could not load " + PACKAGE_URL);
  }
  const bytes = await ReadBody(response);
  packageBuffer = bytes.buffer;
}

function ShowPage() {
  SetProgress(100);
  pageCanvas.classList.add("ready");
  loader.hidden = true;
  loader.setAttribute("aria-busy", "false");
}

function ShowArticle(message) {
  loader.hidden = true;
  statusNode.textContent = message;
  pageCanvas.style.display = "none";
}

async function Start() {
  window.setTimeout(() => {
    if (!loader.hidden && shownPercent < 100) {
      statusNode.textContent = "This is taking longer than usual…";
    }
  }, 5000);
  try {
    const wasmPromise = FetchParts();
    const packagePromise = FetchPackage();
    const wasm = await wasmPromise;
    await packagePromise;
    await LoadScript(ENGINE_JS);
    await LoadScript("../Demo/Demo.js");
    await window.RoboBoot({
      instantiateWasm: (imports, receiveInstance) => {
        WebAssembly.instantiate(wasm, imports).then((result) => {
          receiveInstance(result.instance);
        }).catch((error) => {
          ShowArticle(String(error));
        });
        return {};
      },
      getPreloadedPackage: () => packageBuffer,
    });
    ShowPage();
  } catch (error) {
    ShowArticle(error && error.message ? error.message : "This page could not load.");
  }
}

Start();
`;
  fs.writeFileSync(path.join(outDir, "App.js"), app);
}

function SplitWasm(wasmPath, outDir) {
  const wasm = fs.readFileSync(wasmPath);
  const existing = fs.readdirSync(outDir);
  for (const name of existing) {
    if (/^part-\d+\.wasm$/i.test(name)) {
      fs.unlinkSync(path.join(outDir, name));
    }
  }
  const parts = [];
  const width = String(Math.ceil(wasm.length / PART_LIMIT)).length;
  const digits = Math.max(5, width);
  for (let offset = 0; offset < wasm.length; offset += PART_LIMIT) {
    const end = Math.min(wasm.length, offset + PART_LIMIT);
    const slice = wasm.subarray(offset, end);
    if (slice.length > PART_LIMIT) {
      throw new Error("Part exceeded 50 KB");
    }
    const name = "part-" + String(parts.length).padStart(digits, "0") + ".wasm";
    fs.writeFileSync(path.join(outDir, name), slice);
    const written = fs.statSync(path.join(outDir, name)).size;
    if (written > PART_LIMIT) {
      throw new Error(name + " is " + written + " bytes");
    }
    parts.push(name);
  }
  if (parts.length === 0) {
    throw new Error("WebAssembly file is empty");
  }
  return { parts, bytes: wasm.length };
}

function Main() {
  const folder = ReadArg("--folder");
  const xmlPath = ReadArg("--xml");
  const wasmPath = ReadArg("--wasm");
  const dataPath = ReadArg("--data");
  const engineJs = ReadArg("--engine");
  const packageUrl = ReadArg("--package");
  const outDir = ReadArg("--out");
  fs.mkdirSync(outDir, { recursive: true });
  const split = SplitWasm(wasmPath, outDir);
  const packageBytes = fs.existsSync(dataPath) ? fs.statSync(dataPath).size : 0;
  const seo = ReadSeo(xmlPath);
  WriteIndex(outDir, folder, seo);
  WriteApp(outDir, split.parts, split.bytes, engineJs, packageBytes > 0 ? packageUrl : "", packageBytes);
  const largest = split.parts.reduce((max, name) => {
    return Math.max(max, fs.statSync(path.join(outDir, name)).size);
  }, 0);
  console.log(folder + ": " + split.parts.length + " wasm parts, " + split.bytes + " bytes, largest " + largest);
}

Main();
