const http = require("http");
const fs = require("fs");
const path = require("path");

const root = process.cwd();
const port = Number(process.argv[2] || 4173);
const home = process.argv[3] || "/Desktop/";
const types = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".wasm": "application/wasm",
  ".data": "application/octet-stream",
  ".ttf": "font/ttf"
};

function SafeJoin(urlPath) {
  const filePath = path.normalize(path.join(root, urlPath));
  const prefix = root.endsWith(path.sep) ? root : root + path.sep;
  if (filePath !== root && !filePath.startsWith(prefix)) {
    return null;
  }
  return filePath;
}

function IsFile(filePath) {
  return !!filePath && fs.existsSync(filePath) && fs.statSync(filePath).isFile();
}

function Resolve(urlPath) {
  if (urlPath === "/") {
    urlPath = home.endsWith("/") ? home : home + "/";
  }
  const candidates = [];
  if (urlPath.endsWith("/")) {
    candidates.push(urlPath + "Index.html", urlPath + "index.html");
  } else if (!path.posix.extname(urlPath)) {
    candidates.push(urlPath + "/Index.html", urlPath + "/index.html", urlPath);
  } else {
    candidates.push(urlPath);
  }
  let walk = urlPath.endsWith("/") ? urlPath.slice(0, -1) : path.posix.dirname(urlPath);
  while (walk && walk !== "/" && walk !== ".") {
    candidates.push(walk + "/Index.html", walk + "/index.html");
    const parent = path.posix.dirname(walk);
    if (parent === walk) {
      break;
    }
    walk = parent;
  }
  for (const candidate of candidates) {
    const filePath = SafeJoin(candidate);
    if (IsFile(filePath)) {
      return filePath;
    }
  }
  return null;
}

const server = http.createServer((req, res) => {
  try {
    const urlPath = decodeURIComponent((req.url || "/").split("?")[0]);
    const filePath = Resolve(urlPath);
    if (!filePath) {
      res.writeHead(404);
      res.end("Not found");
      return;
    }
    const ext = path.extname(filePath).toLowerCase();
    res.writeHead(200, {
      "Content-Type": types[ext] || "application/octet-stream",
      "Cache-Control": "no-store"
    });
    fs.createReadStream(filePath).pipe(res);
  } catch (error) {
    res.writeHead(500);
    res.end(String(error));
  }
});

server.listen(port, "127.0.0.1", () => {
  console.log("Preview at http://127.0.0.1:" + port + home);
});
