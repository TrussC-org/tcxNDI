#include "tcApp.h"
#include <tcxNDI.h>

#ifdef TCX_NDI_AVAILABLE
using namespace tcx;
static NDISender sender;
static Fbo fbo;
static bool senderReady = false;
#endif

void tcApp::setup() {
#ifdef TCX_NDI_AVAILABLE
    fbo.allocate(960, 600);
    senderReady = sender.setup("TrussC NDI Sender", 960, 600);
    sender.setFrameRate(60);

    if (senderReady) {
        logNotice() << "NDI Sender started: " << sender.getName();
    } else {
        logError() << "NDI Sender failed to start";
    }
#else
    logWarning() << "NDI SDK not installed - sender disabled";
#endif
}

void tcApp::update() {
}

void tcApp::draw() {
#ifdef TCX_NDI_AVAILABLE
    // Draw content to FBO
    fbo.begin();
    clear(colors::navy);

    setColor(colors::orange);
    pushMatrix();
    translate(fbo.getWidth() / 2.0f, fbo.getHeight() / 2.0f);
    rotate(getElapsedTimef() * 0.3f);
    drawRect(-100, -100, 200, 200);
    popMatrix();

    setColor(1);
    drawBitmapString("NDI Sender - " + to_string((int)getFrameRate()) + " fps", 20, 20);
    fbo.end();

    // Send via NDI
    if (senderReady) {
        sender.send(fbo);
    }

    // Draw to screen
    clear(0.12f);
    fbo.draw(0, 0);

    setColor(1);
    string status = senderReady ? "NDI: Sending" : "NDI: Not available";
    if (senderReady) {
        status += " (" + to_string(sender.getNumConnections()) + " receivers)";
    }
    drawBitmapString(status, 20, getWindowHeight() - 20);
#else
    clear(0.1f);
    setColor(1);
    drawBitmapString("NDI SDK not installed", 20, 20);
    drawBitmapString("Download from: https://ndi.video/download-ndi-sdk/", 20, 40);
#endif
}

void tcApp::keyPressed(int key) {}
void tcApp::keyReleased(int key) {}

void tcApp::mousePressed(Vec2 pos, int button) {}
void tcApp::mouseReleased(Vec2 pos, int button) {}
void tcApp::mouseMoved(Vec2 pos) {}
void tcApp::mouseDragged(Vec2 pos, int button) {}
void tcApp::mouseScrolled(Vec2 delta) {}

void tcApp::windowResized(int width, int height) {}
void tcApp::filesDropped(const vector<string>& files) {}
void tcApp::exit() {}
