# tcxNDI

NDI (Network Device Interface) send/receive addon for [TrussC](https://github.com/TrussC-org/TrussC).

Send and receive video/audio over the network using the NDI protocol.

## Features

- **NDISender**: Send video frames (Texture, Fbo, Pixels, raw data) over NDI
- **NDIReceiver**: Discover NDI sources, connect, and receive video to Texture/Pixels
- Async sending mode for non-blocking operation
- Frame rate control
- Audio send/receive support
- Graceful degradation when NDI SDK is not installed

## Requirements

**NDI SDK** must be downloaded and installed manually (not redistributable).

### Download

https://ndi.video/download-ndi-sdk/

### Default Installation Paths

| Platform | Path |
|----------|------|
| macOS | `/Library/NDI SDK for Apple/` |
| Windows | `C:/Program Files/NDI/NDI 6 SDK/` |
| Linux | `/usr/local/NDI SDK for Linux/` |

### Custom Path

```bash
cmake -DNDI_SDK_DIR=/your/custom/path ..
```

> **Note:** If the NDI SDK is not found, the addon compiles as a stub — your app will build and run, but NDI functionality will be disabled with a warning message.

## Setup

1. Add `tcxNDI` to your project's `addons.make`:
   ```
   tcxNDI
   ```

2. Run projectGenerator to update your project.

3. Install the NDI SDK (see above).

## Usage

### Sender

```cpp
#include <TrussC.h>
#include <tcxNDI.h>
using namespace std;
using namespace tc;
using namespace tcx;

NDISender sender;
Fbo fbo;

void setup() {
    fbo.allocate(1920, 1080);
    sender.setup("TrussC Output", 1920, 1080);
    sender.setFrameRate(60);
}

void draw() {
    fbo.begin();
    clear(0);
    setColor(colors::orange);
    drawCircle(getMouseX(), getMouseY(), 50);
    fbo.end();

    sender.send(fbo);
    fbo.draw(0, 0);
}
```

### Receiver

```cpp
#include <TrussC.h>
#include <tcxNDI.h>
using namespace std;
using namespace tc;
using namespace tcx;

NDIReceiver receiver;
Texture tex;

void setup() {
    auto sources = receiver.findSources(3000);
    if (!sources.empty()) {
        receiver.connect(sources[0]);
    }
}

void draw() {
    clear(0.1);
    if (receiver.receive(tex)) {
        tex.draw(0, 0, getWindowWidth(), getWindowHeight());
    }
}
```

## API Reference

### NDISender

| Method | Description |
|--------|-------------|
| `setup(name, w, h)` | Create NDI sender |
| `close()` | Stop sending |
| `send(Texture/Fbo/Pixels)` | Send a video frame |
| `send(data, w, h, ch)` | Send raw pixel data |
| `setFrameRate(fps)` | Set output frame rate |
| `setAsync(bool)` | Enable async sending |
| `getName()` | Get sender name |
| `getNumConnections()` | Number of connected receivers |

### NDIReceiver

| Method | Description |
|--------|-------------|
| `findSources(timeoutMs)` | Discover available NDI sources |
| `connect(source/name)` | Connect to a source |
| `disconnect()` | Disconnect |
| `isConnected()` | Check connection status |
| `receive(Texture/Pixels)` | Receive a video frame |
| `isFrameNew()` | Check for new frame |
| `getSourceName()` | Connected source name |
| `getWidth() / getHeight()` | Frame dimensions |
| `getFps()` | Current receive frame rate |

### NDISource

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Source display name |
| `url` | string | Internal NDI URL |

## Platform Support

| Platform | Status |
|----------|--------|
| macOS | Supported |
| Windows | Supported |
| Linux | Supported |
| Web/WASM | Not supported (NDI is native only) |

## Examples

- `example-sender/` — Sends animated graphics over NDI
- `example-receiver/` — Discovers and displays NDI sources

## References

- [NDI SDK Download](https://ndi.video/download-ndi-sdk/)
- [NDI SDK Documentation](https://docs.ndi.video/)
- [leadedge/ofxNDI](https://github.com/leadedge/ofxNDI)

## License

MIT
