import os
import struct
import sys

import cv2


def Cover(frame, width, height):
    srcH, srcW = frame.shape[:2]
    scale = max(float(width) / float(srcW), float(height) / float(srcH))
    scaledW = max(width, int(round(srcW * scale)))
    scaledH = max(height, int(round(srcH * scale)))
    scaled = cv2.resize(frame, (scaledW, scaledH), interpolation=cv2.INTER_AREA)
    x = max(0, (scaledW - width) // 2)
    y = max(0, (scaledH - height) // 2)
    return scaled[y:y + height, x:x + width]


def Encode(capture, width, height, step, quality):
    capture.set(cv2.CAP_PROP_POS_FRAMES, 0)
    frames = []
    index = 0
    while True:
        ok, frame = capture.read()
        if not ok:
            break
        if index % step == 0:
            covered = Cover(frame, width, height)
            encoded, buffer = cv2.imencode(".jpg", covered, [int(cv2.IMWRITE_JPEG_QUALITY), quality])
            if not encoded:
                return None
            frames.append(buffer.tobytes())
        index += 1
    return frames


def Pack(source, destinations, maxBytes):
    capture = cv2.VideoCapture(source)
    if not capture.isOpened():
        print("Could not open " + source)
        return 1
    fps = capture.get(cv2.CAP_PROP_FPS)
    if fps < 1.0:
        fps = 24.0
    width = 1280
    height = 800
    quality = 75
    step = max(1, int(round(fps / 24.0)))
    frames = None
    while True:
        frames = Encode(capture, width, height, step, quality)
        if frames is None:
            print("Could not encode a background frame")
            capture.release()
            return 1
        if not frames:
            print("Background video has no frames")
            capture.release()
            return 1
        total = 20 + (len(frames) + 1) * 4
        for frame in frames:
            total += len(frame)
        if total <= maxBytes:
            break
        if quality > 55:
            quality -= 5
            continue
        if len(frames) > 8:
            step *= 2
            quality = 75
            continue
        break
    capture.release()
    frameMs = int(round(1000.0 * step / fps))
    if frameMs < 40:
        frameMs = 40
    cursor = 20 + (len(frames) + 1) * 4
    offsets = []
    payload = b""
    for frame in frames:
        offsets.append(cursor)
        payload += frame
        cursor += len(frame)
    offsets.append(cursor)
    blob = struct.pack("<4sIIII", b"RJPG", len(frames), frameMs, width, height)
    blob += struct.pack("<" + ("I" * len(offsets)), *offsets)
    blob += payload
    if len(blob) > maxBytes:
        print("Background reel is too large")
        return 1
    for destination in destinations:
        folder = os.path.dirname(destination)
        if folder:
            os.makedirs(folder, exist_ok=True)
        with open(destination, "wb") as output:
            output.write(blob)
        print("Packed " + destination + " (" + str(len(frames)) + " frames, " + str(len(blob)) + " bytes)")
    return 0


if __name__ == "__main__":
    root = os.path.dirname(os.path.abspath(__file__))
    source = os.path.join(root, "Assets", "Background.mp4")
    destinations = [
        os.path.join(root, "Build", "Esp", "BACKGROU.RJP"),
        os.path.join(root, "Build", "EspArm", "BACKGROU.RJP"),
        os.path.join(root, "Compiler", "Assets", "Background.rjp"),
    ]
    sys.exit(Pack(source, destinations, 20 * 1024 * 1024))
