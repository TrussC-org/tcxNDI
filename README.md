# tcxNDI アドオン設計計画

> **Status**: 保留（nariakiiwataniさんに相談予定）

## 概要

NDI (Network Device Interface) を TrussC で使えるようにするアドオン。
映像・音声をネットワーク経由で送受信できる。

---

## ofxNDI 調査結果

### 主要実装の比較

| 実装 | 特徴 | ライセンス |
|------|------|-----------|
| [leadedge/ofxNDI](https://github.com/leadedge/ofxNDI) | 最も機能豊富、PBO非同期読み出し、YUV変換シェーダー | GPL-3.0 |
| [nariakiiwatani/ofxNDI](https://github.com/nariakiiwatani/ofxNDI) | 全機能ラップ志向、PTZ/Router/Recorder対応 | 不明 |
| [thomasgeissl/ofxNDI](https://github.com/thomasgeissl/ofxNDI) | ofVideoGrabber継承でシンプル | 不明 |

### leadedge vs nariakiiwatani の違い

| 観点 | leadedge | nariakiiwatani |
|------|----------|----------------|
| **設計思想** | 映像送受信に特化、最適化重視 | NDI SDK全機能をラップ |
| **クラス構成** | Sender/Receiver のみ | Sender/Receiver + Finder + PTZ + Router + Recorder |
| **GPU最適化** | PBO非同期読み出し、YUVシェーダー | なし（シンプル） |
| **Audio** | API提供だがREADMEに記載なし | 明示的なAudio対応 |
| **メンテナンス** | 247 commits, アクティブ | 機能豊富だが一部未テスト |

**結論**: tcxNDIは leadedge のシンプルさ + nariakiiwatani のAudio対応を参考に設計

### leadedge/ofxNDI の設計パターン

**Sender API:**
```cpp
CreateSender(name, width, height)
SendImage(ofFbo/ofTexture/ofPixels/unsigned char*)
SetFrameRate(fps)
SetAsync(bool)  // 非同期送信
SetReadback(bool)  // GPU→CPU非同期読み出し
```

**Receiver API:**
```cpp
CreateFinder() / FindSenders() / GetSenderList()
SetSenderIndex(index) / SetSenderName(name)
CreateReceiver(index)
ReceiveImage(ofTexture/ofFbo/ofPixels)
```

**ポイント:**
- 低レベル層 (ofxNDIsend/ofxNDIreceive) と高レベル層 (ofxNDIsender/ofxNDIreceiver) の2層構造
- YUV変換シェーダーで帯域削減
- PBOによる非同期GPU読み出し

---

## TrussC アドオン規約

### namespace
- アドオンは `namespace tcx` を使用（TrussC本体の `tc` と区別）
- ユーザーコードでは `using namespace tcx;` で使用

### 命名規則
- ファイル名: `tcx<AddonName><Class>.h/cpp`（例: `tcxNDISender.h`）
- クラス名: `<Class>`（namespace内なので接頭辞不要、例: `tcx::NDISender`）
- CMakeターゲット: `tcx<AddonName>`、エイリアス `tc::tcx<AddonName>`

### include
- メインヘッダー: `<tcxNDI.h>`（すべてをinclude）
- ユーザーは `#include <TrussC.h>` と `#include <tcxNDI.h>` の2行

### 依存関係
- `target_link_libraries(${ADDON_NAME} PUBLIC TrussC ...)` でTrussCにリンク
- TrussCの型（`Texture`, `Fbo`, `Pixels`など）を直接使用

---

## tcxNDI 設計方針

### 1. シンプルなAPI（oF高レベル層相当）

TrussCユーザー向けに使いやすいAPIを提供。低レベル層は内部実装として隠蔽。

### 2. 手動SDK管理

NDI SDKはライセンス上再配布不可のため、ユーザーが手動でダウンロード。
CMakeで親切なエラーメッセージを出す。

### 3. Web非対応

NDIはネイティブ専用。WASMビルドは無効化。

---

## ディレクトリ構成

```
TrussC/addons/tcxNDI/
├── CMakeLists.txt              # SDK検索 + 親切なエラーメッセージ
├── README.md                   # SDKダウンロード手順
├── src/
│   ├── tcxNDI.h                # 共通ヘッダー（NDI SDK include）
│   ├── tcxNDISender.h          # 送信クラス
│   ├── tcxNDISender.cpp
│   ├── tcxNDIReceiver.h        # 受信クラス（Finder機能含む）
│   └── tcxNDIReceiver.cpp
├── example-sender/             # 送信サンプル
│   ├── CMakeLists.txt
│   └── src/tcApp.cpp
└── example-receiver/           # 受信サンプル
    ├── CMakeLists.txt
    └── src/tcApp.cpp
```

---

## CMakeLists.txt（SDK検索）

```cmake
# =============================================================================
# tcxNDI - NDI integration addon for TrussC
# =============================================================================

cmake_minimum_required(VERSION 3.16)
set(ADDON_NAME tcxNDI)

# -----------------------------------------------------------------------------
# Find NDI SDK (manual download required)
# -----------------------------------------------------------------------------

# Search paths by platform
# Note: These are the default installation paths from NDI SDK installers
if(APPLE)
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "/Library/NDI SDK for Apple"              # Default installer location
        "$ENV{HOME}/NDI SDK for Apple"
    )
    set(_NDI_LIB_NAME "libndi.dylib")
    set(_NDI_LIB_SUBDIR "lib/macOS")
elseif(WIN32)
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "C:/Program Files/NDI/NDI 6 SDK"          # NDI 6 default
        "C:/Program Files/NewTek/NDI 5 SDK"       # NDI 5 legacy
    )
    set(_NDI_LIB_NAME "Processing.NDI.Lib.x64.dll")
    set(_NDI_LIB_SUBDIR "Lib/x64")
else()  # Linux
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "/usr/local/NDI SDK for Linux"            # Manual install location
        "$ENV{HOME}/NDI SDK for Linux"
    )
    set(_NDI_LIB_NAME "libndi.so")
    set(_NDI_LIB_SUBDIR "lib/x86_64-linux-gnu")
endif()

# Find header
find_path(NDI_INCLUDE_DIR
    NAMES Processing.NDI.Lib.h
    PATHS ${_NDI_SDK_SEARCH_PATHS}
    PATH_SUFFIXES include Include
)

# Find library
find_library(NDI_LIBRARY
    NAMES ndi Processing.NDI.Lib.x64
    PATHS ${_NDI_SDK_SEARCH_PATHS}
    PATH_SUFFIXES lib Lib lib/x64 Lib/x64
)

# Helpful error message
if(NOT NDI_INCLUDE_DIR OR NOT NDI_LIBRARY)
    message(FATAL_ERROR
"
======================================================================
  NDI SDK not found!
======================================================================

  tcxNDI requires the NDI SDK which must be downloaded manually.

  DOWNLOAD:
    https://ndi.video/download-ndi-sdk/

  INSTALLATION:
    Run the installer - it will install to the default location below.

  DEFAULT PATHS (set by NDI installer):
    macOS:   /Library/NDI SDK for Apple/
    Windows: C:/Program Files/NDI/NDI 6 SDK/
    Linux:   (manual install to /usr/local/NDI SDK for Linux/)

  CUSTOM PATH:
    cmake -DNDI_SDK_DIR=/your/custom/path ..

  Searched: ${_NDI_SDK_SEARCH_PATHS}
======================================================================
"
    )
endif()

message(STATUS "tcxNDI: Found NDI SDK")
message(STATUS "  Include: ${NDI_INCLUDE_DIR}")
message(STATUS "  Library: ${NDI_LIBRARY}")

# -----------------------------------------------------------------------------
# Addon library
# -----------------------------------------------------------------------------

file(GLOB ADDON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB ADDON_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_library(${ADDON_NAME} STATIC ${ADDON_SOURCES} ${ADDON_HEADERS})
add_library(tc::${ADDON_NAME} ALIAS ${ADDON_NAME})

target_include_directories(${ADDON_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${NDI_INCLUDE_DIR}
)

target_link_libraries(${ADDON_NAME} PUBLIC
    TrussC
    ${NDI_LIBRARY}
)

# macOS: Set rpath for dylib
if(APPLE)
    set_target_properties(${ADDON_NAME} PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
    )
endif()
```

---

## クラス設計

### tcxNDISender

```cpp
namespace tcx {

class NDISender {
public:
    NDISender();
    ~NDISender();

    // Setup / Cleanup
    bool setup(const string& name, int width, int height);
    void close();
    bool isSetup() const;

    // Send (main API)
    bool send(const Texture& texture);
    bool send(const Fbo& fbo);
    bool send(const unsigned char* pixels, int width, int height,
              PixelFormat format = PixelFormat::RGBA);

    // Settings
    void setFrameRate(float fps);
    void setFrameRate(int numerator, int denominator);  // e.g., 30000/1001 for 29.97
    float getFrameRate() const;

    void setAsync(bool async);  // Async sending (default: true)
    bool isAsync() const;

    // Audio
    void setAudioEnabled(bool enabled);
    bool isAudioEnabled() const;
    void setAudioSampleRate(int sampleRate);  // Default: 48000
    void setAudioChannels(int channels);       // Default: 2
    bool sendAudio(const float* samples, int numSamples, int numChannels);

    // Info
    string getName() const;
    int getWidth() const;
    int getHeight() const;

private:
    struct Impl;
    unique_ptr<Impl> impl_;
};

} // namespace tcx
```

### tcxNDIReceiver

```cpp
namespace tcx {

struct NDISource {
    string name;      // e.g., "MACBOOK (OBS)"
    string url;       // Internal NDI URL
};

class NDIReceiver {
public:
    NDIReceiver();
    ~NDIReceiver();

    // Source Discovery
    vector<NDISource> findSources(uint32_t timeoutMs = 1000);
    int getSourceCount() const;

    // Connection
    bool connect(const NDISource& source);
    bool connect(const string& sourceName);
    bool connect(int sourceIndex);  // From last findSources() result
    void disconnect();
    bool isConnected() const;

    // Receive (main API)
    bool receive(Texture& texture);      // Resizes texture automatically
    bool receive(Pixels& pixels);        // Resizes pixels automatically
    bool isFrameNew() const;             // True if new frame since last receive

    // Source Info
    string getSourceName() const;
    int getWidth() const;
    int getHeight() const;
    float getFps() const;

    // Settings
    void setLowBandwidth(bool low);      // Receive lower quality for preview

    // Audio
    void setAudioEnabled(bool enabled);
    bool isAudioEnabled() const;
    bool hasAudio() const;               // True if source provides audio
    int getAudioSampleRate() const;
    int getAudioChannels() const;
    bool receiveAudio(float* samples, int& numSamples, int& numChannels);

private:
    struct Impl;
    unique_ptr<Impl> impl_;
};

} // namespace tcx
```

---

## 使用例

### Sender Example

```cpp
#include <TrussC.h>
#include <tcxNDI.h>
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
    // Draw to FBO
    fbo.begin();
    clear(0);
    setColor(1, 0.5, 0);
    drawCircle(getMouseX(), getMouseY(), 50);
    fbo.end();

    // Send over NDI
    sender.send(fbo);

    // Also draw to screen
    fbo.draw(0, 0);
}
```

### Receiver Example

```cpp
#include <TrussC.h>
#include <tcxNDI.h>
using namespace tc;
using namespace tcx;

NDIReceiver receiver;
Texture tex;
vector<NDISource> sources;
bool connected = false;

void setup() {
    // Find available NDI sources
    sources = receiver.findSources(2000);
    for (auto& s : sources) {
        logNotice("Found: " + s.name);
    }

    // Connect to first source
    if (!sources.empty()) {
        receiver.connect(sources[0]);
        connected = true;
    }
}

void draw() {
    clear(0.1);

    if (connected && receiver.receive(tex)) {
        tex.draw(0, 0, getWindowWidth(), getWindowHeight());
    }

    setColor(1);
    drawBitmapString("Sources: " + to_string(sources.size()), 20, 20);
    if (connected) {
        drawBitmapString("Connected: " + receiver.getSourceName(), 20, 40);
        drawBitmapString("Size: " + to_string(receiver.getWidth()) + "x" +
                         to_string(receiver.getHeight()), 20, 60);
    }
}

void keyPressed(int key) {
    if (key == 'r') {
        sources = receiver.findSources();
    }
}
```

---

## 実装ステップ

### Phase 1: 基本構造
1. ディレクトリ作成、CMakeLists.txt（SDK検索 + 親切なエラーメッセージ）
2. tcxNDI.h（NDI SDKのinclude、初期化/終了処理）
3. README.md（ダウンロード手順、プラットフォーム別パス）

### Phase 2: Sender実装
1. tcxNDISender.h/cpp
2. Video送信（Texture/Fbo→ピクセル読み出し→NDI送信）
3. Audio送信（float buffer → NDIlib_audio_frame_v2_t）
4. example-sender（映像のみでOK、Audioは別サンプルでも可）

### Phase 3: Receiver実装
1. tcxNDIReceiver.h/cpp
2. Source discovery (NDIlib_find_*)
3. Video受信（NDI → Texture）
4. Audio受信（NDIlib_audio_frame → float buffer）
5. example-receiver

### Phase 4: テスト・検証
1. OBS NDI出力との相互運用確認
2. Sender ↔ Receiver 双方向テスト

---

## プラットフォーム対応

| Platform | NDI SDK | Status |
|----------|---------|--------|
| macOS | NDI SDK for Apple | Primary target |
| Windows | NDI SDK for Windows | Secondary |
| Linux | NDI SDK for Linux | Untested |
| Web/WASM | N/A | Not supported |

---

## 検証方法

1. **macOS**: NDI SDK for Apple をインストール、example-sender/receiver をビルド
2. **OBS連携**: OBSのNDI出力をtcxNDIReceiverで受信
3. **双方向**: sender/receiver間で映像送受信

---

## NDI Audio API リファレンス

### 送信 (NDIlib_audio_frame_v2_t)
```cpp
NDIlib_audio_frame_v2_t audio_frame;
audio_frame.sample_rate = 48000;
audio_frame.no_channels = 2;
audio_frame.no_samples = 1920;  // samples per channel
audio_frame.p_data = float_buffer;
audio_frame.channel_stride_in_bytes = no_samples * sizeof(float);

NDIlib_send_send_audio_v2(pNDI_send, &audio_frame);
```

### 受信
```cpp
NDIlib_audio_frame_v2_t audio_frame;
NDIlib_frame_type_e frame_type = NDIlib_recv_capture_v2(pNDI_recv, nullptr, &audio_frame, nullptr, timeout);

if (frame_type == NDIlib_frame_type_audio) {
    // audio_frame.sample_rate, no_channels, no_samples, p_data
    // Process audio...
    NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);
}
```

---

## 参考リンク

- [NDI SDK Download](https://ndi.video/download-ndi-sdk/)
- [NDI SDK Documentation](https://docs.ndi.video/)
- [NDI Example Code](https://docs.ndi.video/all/developing-with-ndi/sdk/example-code)
- [leadedge/ofxNDI](https://github.com/leadedge/ofxNDI)
- [nariakiiwatani/ofxNDI](https://github.com/nariakiiwatani/ofxNDI)
