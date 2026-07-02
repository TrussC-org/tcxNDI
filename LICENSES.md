# tcxNDI Licenses

## tcxNDI

MIT License

Copyright (c) 2026 tettou771

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## NDI SDK (external, proprietary — NOT redistributed)

tcxNDI is a thin wrapper around the **NDI SDK** by NewTek / Vizrt. The SDK
is proprietary and is **not** bundled or redistributed with this addon. You
must download and install it yourself, and your use of it is governed by the
NDI SDK license agreement — not by the MIT license above.

- Download: https://ndi.video/download-ndi-sdk/
- License: NDI SDK License Agreement (NewTek / Vizrt). See the LICENSE file
  included with the SDK installation.

Default install locations searched by this addon's CMake:

| Platform | Path |
|----------|------|
| macOS   | `/Library/NDI SDK for Apple/` |
| Windows | `C:/Program Files/NDI/NDI 6 SDK/` |
| Linux   | `/usr/local/NDI SDK for Linux/` |
