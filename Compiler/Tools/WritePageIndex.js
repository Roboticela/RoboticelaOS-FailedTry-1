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
      const modifiers = (event) =>
        (event.shiftKey ? 1 : 0) | (event.ctrlKey ? 2 : 0) | (event.metaKey ? 4 : 0) | (event.altKey ? 8 : 0);
      let pressTimer = 0;
      let pointerDown = false;
      const clearPress = () => {
        if (pressTimer) {
          clearTimeout(pressTimer);
          pressTimer = 0;
        }
      };
      canvas.addEventListener("pointermove", (event) => {
        const [x, y] = point(event);
        follow(app.PointerMove(x, y));
        if (pointerDown && (event.buttons & 1) === 0) {
          pointerDown = false;
          clearPress();
          follow(app.PointerUp(x, y));
        }
      });
      canvas.addEventListener("pointerdown", (event) => {
        if (event.button !== 0) {
          return;
        }
        canvas.focus();
        app.SetModifiers(modifiers(event));
        if (canvas.setPointerCapture) {
          canvas.setPointerCapture(event.pointerId);
        }
        const [x, y] = point(event);
        clearPress();
        if (event.pointerType === "touch") {
          pressTimer = setTimeout(() => follow(app.LongPressSelect(x, y)), 500);
        }
        pointerDown = true;
        follow(app.PointerDown(x, y, event.pointerType === "touch", event.detail || 1));
      });
      window.addEventListener("pointerup", (event) => {
        pointerDown = false;
        clearPress();
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
        app.SetModifiers(modifiers(event));
        if (app.KeyDown(event.key, event.shiftKey)) {
          event.preventDefault();
          return;
        }
        if (event.key === "Backspace") app.KeyBackspace();
        else if (event.key === "Delete") app.KeyDelete();
        else if (event.key === "Escape") app.KeyEscape();
        else if (event.key === "ArrowLeft") app.KeyLeft();
        else if (event.key === "ArrowRight") app.KeyRight();
        else if (event.key.length === 1 && !event.ctrlKey && !event.metaKey) app.KeyInsert(event.key);
      });
      canvas.addEventListener("keyup", (event) => app.SetModifiers(modifiers(event)));
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
