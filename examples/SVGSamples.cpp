#include "SVGSamples.h" 

namespace svgmedia {

const char* MediaPlay = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M0 0v6l6-3-6-3z" transform="translate(1 1)" />
</svg>
)-";

const char* MediaStop = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M0 0v6h6v-6h-6z" transform="translate(1 1)" />
</svg>
)-";

const char* MediaPause = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M0 0v6h2v-6h-2zm4 0v6h2v-6h-2z" transform="translate(1 1)" />
</svg>
)-";

const char* MediaRecord = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M3 0c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z" transform="translate(1 1)" />
</svg>
)-";

const char* MediaStepBackward = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M0 0v6h2v-6h-2zm2 3l5 3v-6l-5 3z" transform="translate(0 1)" />
</svg>
)-";

const char* MediaStepForward = R"-(
<svg width="8" height="8" viewBox="0 0 8 8">
  <path d="M0 0v6l5-3-5-3zm5 3v3h2v-6h-2v3z" transform="translate(0 1)" />
</svg>
)-";

}