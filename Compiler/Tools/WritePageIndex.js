const fs = require("fs");
const path = require("path");

function ReadArg(name) {
  const index = process.argv.indexOf(name);
  if (index < 0 || index + 1 >= process.argv.length) {
    throw new Error("Missing " + name);
  }
  return process.argv[index + 1];
}

function WritePageIndex(outDir, title, assetBase) {
  const base = assetBase.endsWith("/") ? assetBase : assetBase + "/";
  const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>${title}</title>
  <style>
    html, body { margin: 0; width: 100%; height: 100%; overflow: hidden; background: #FFFFFF; }
    #robo-canvas { position: fixed; inset: 0; width: 100%; height: 100%; outline: none; touch-action: none; }
  </style>
</head>
<body>
  <canvas id="robo-canvas" tabindex="0"></canvas>
  <script src="${base}Robo.js"></script>
  <script>
    const assetBase = ${JSON.stringify(base)};
    const canvas = document.getElementById("robo-canvas");
    CreateRoboModule({
      locateFile: (name) => assetBase + name
    }).then((app) => {
      const pixelScale = () => window.devicePixelRatio || 1;
      const point = (event) => {
        const rect = canvas.getBoundingClientRect();
        const scale = pixelScale();
        return [(event.clientX - rect.left) * scale, (event.clientY - rect.top) * scale];
      };
      const paint = () => {
        const scale = pixelScale();
        app.Render(Math.max(1, Math.round(window.innerWidth * scale)), Math.max(1, Math.round(window.innerHeight * scale)), scale);
      };
      const follow = (changed) => {
        if (changed) {
          paint();
        }
      };
      paint();
      window.addEventListener("resize", paint);
      let last = performance.now();
      const frame = (now) => {
        const delta = now - last;
        last = now;
        if (app.Tick(delta)) {
          paint();
        }
        requestAnimationFrame(frame);
      };
      requestAnimationFrame(frame);
      canvas.addEventListener("pointermove", (event) => {
        const [x, y] = point(event);
        follow(app.PointerMove(x, y));
      });
      canvas.addEventListener("pointerdown", (event) => {
        canvas.focus();
        const [x, y] = point(event);
        follow(app.PointerDown(x, y, event.pointerType === "touch", 1));
      });
      window.addEventListener("pointerup", (event) => {
        const [x, y] = point(event);
        follow(app.PointerUp(x, y));
      });
      canvas.addEventListener("pointerleave", () => follow(app.PointerLeave()));
      canvas.addEventListener("wheel", (event) => {
        event.preventDefault();
        const [x, y] = point(event);
        follow(app.Wheel(x, y, event.deltaX, event.deltaY));
      }, { passive: false });
      canvas.addEventListener("keydown", (event) => {
        if (event.key === "Backspace") app.KeyBackspace();
        else if (event.key === "Delete") app.KeyDelete();
        else if (event.key === "Escape") app.KeyEscape();
        else if (event.key === "ArrowLeft") app.KeyLeft();
        else if (event.key === "ArrowRight") app.KeyRight();
        else if (event.key.length === 1 && !event.ctrlKey && !event.metaKey) app.KeyInsert(event.key);
      });
    });
  </script>
</body>
</html>
`;
  fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(path.join(outDir, "Index.html"), html);
  console.log("Wrote " + path.join(outDir, "Index.html"));
}

WritePageIndex(ReadArg("--out"), ReadArg("--title"), ReadArg("--asset"));
