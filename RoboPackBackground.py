import os
import struct
import sys

import cv2


def Pack(source, destinations, maxBytes):
    capture = cv2.VideoCapture(source)
    if not capture.isOpened():
        print("Could not open " + source)
        return 1
    fps = capture.get(cv2.CAP_PROP_FPS)
    if fps < 1.0:
        fps = 24.0
    width = 960
    height = 540
    targetFps = 8.0
    quality = 55
    step = max(1, int(round(fps / targetFps)))
    frames = []
    index = 0
    while True:
        ok, frame = capture.read()
        if not ok:
            break
        if index % step == 0:
            scaled = cv2.resize(frame, (width, height), interpolation=cv2.INTER_AREA)
            encoded, buffer = cv2.imencode(".jpg", scaled, [int(cv2.IMWRITE_JPEG_QUALITY), quality])
            if not encoded:
                print("Could not encode a background frame")
                return 1
            frames.append(buffer.tobytes())
        index += 1
    capture.release()
    if not frames:
        print("Background video has no frames")
        return 1
    while True:
        total = 20 + (len(frames) + 1) * 4
        for frame in frames:
            total += len(frame)
        if total <= maxBytes or len(frames) <= 8:
            break
        frames = frames[::2]
        step *= 2
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
    sys.exit(Pack(source, destinations, 6 * 1024 * 1024))
