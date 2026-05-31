#include "tcApp.h"
#include <tcxNDI.h>

#ifdef TCX_NDI_AVAILABLE
using namespace tcx;
static NDIReceiver receiver;
static Texture tex;
static vector<NDISource> sources;
static bool connected = false;
#endif

void tcApp::setup() {
#ifdef TCX_NDI_AVAILABLE
    logNotice() << "Searching for NDI sources...";
    sources = receiver.findSources(3000);
    for (auto& s : sources) {
        logNotice() << "Found NDI source: " << s.name;
    }

    if (!sources.empty()) {
        connected = receiver.connect(sources[0]);
        if (connected) {
            logNotice() << "Connected to: " << sources[0].name;
        }
    }
#else
    logWarning() << "NDI SDK not installed - receiver disabled";
#endif
}

void tcApp::update() {
}

void tcApp::draw() {
#ifdef TCX_NDI_AVAILABLE
    clear(0.1f);

    if (connected && receiver.receive(tex)) {
        // New frame received
    }

    if (tex.isAllocated()) {
        tex.draw(0, 0, getWindowWidth(), getWindowHeight());
    }

    setColor(1);
    drawBitmapString("NDI Receiver", 20, 20);
    drawBitmapString("Sources: " + to_string(sources.size()), 20, 40);

    if (connected) {
        drawBitmapString("Connected: " + receiver.getSourceName(), 20, 60);
        drawBitmapString("Size: " + to_string(receiver.getWidth()) + "x" +
                         to_string(receiver.getHeight()), 20, 80);
        drawBitmapString("FPS: " + to_string((int)receiver.getFps()), 20, 100);
    } else {
        drawBitmapString("Not connected - press 'r' to refresh", 20, 60);
    }

    drawBitmapString("Keys: 'r' = refresh, 0-9 = connect to source", 20, getWindowHeight() - 20);
#else
    clear(0.1f);
    setColor(1);
    drawBitmapString("NDI SDK not installed", 20, 20);
    drawBitmapString("Download from: https://ndi.video/download-ndi-sdk/", 20, 40);
#endif
}

void tcApp::keyPressed(int key) {
#ifdef TCX_NDI_AVAILABLE
    if (key == 'r') {
        logNotice() << "Refreshing NDI sources...";
        sources = receiver.findSources(3000);
        for (auto& s : sources) {
            logNotice() << "Found: " << s.name;
        }
    }
    if (key >= '0' && key <= '9') {
        int idx = key - '0';
        if (idx < (int)sources.size()) {
            receiver.disconnect();
            connected = receiver.connect(sources[idx]);
            if (connected) {
                logNotice() << "Connected to: " << sources[idx].name;
            }
        }
    }
#endif
}

void tcApp::keyReleased(int key) {}

void tcApp::mousePressed(const MouseEventArgs& e) {}
void tcApp::mouseReleased(const MouseEventArgs& e) {}
void tcApp::mouseMoved(const MouseEventArgs& e) {}
void tcApp::mouseDragged(const MouseEventArgs& e) {}
void tcApp::mouseScrolled(const ScrollEventArgs& e) {}

void tcApp::windowResized(int width, int height) {}
void tcApp::filesDropped(const vector<string>& files) {}
void tcApp::exit() {}
