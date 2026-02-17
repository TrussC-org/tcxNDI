#pragma once

// =============================================================================
// tcxNDI - NDI integration addon for TrussC
// =============================================================================
//
// Usage:
//   #include <TrussC.h>
//   #include <tcxNDI.h>
//   using namespace tc;
//   using namespace tcx;
//
// Requirements:
//   - NDI SDK must be installed (download from https://ndi.video/download-ndi-sdk/)
//   - macOS: /Library/NDI SDK for Apple/
//   - Windows: C:/Program Files/NDI/NDI 6 SDK/
//
// =============================================================================

#include <TrussC.h>

#ifdef TCX_NDI_AVAILABLE
#include <Processing.NDI.Lib.h>
#include "tcxNDISender.h"
#include "tcxNDIReceiver.h"
#else
#warning "tcxNDI: NDI SDK not found - NDI functionality disabled. Download from https://ndi.video/download-ndi-sdk/"
#endif
