#ifndef PDN_Header_pdn_version
#define PDN_Header_pdn_version

// Single source of truth for the pdn version.
//
// Keep PDN_VERSION_STRING in sync with the three numeric macros below.
// CMake parses this header to obtain project(pdn VERSION ...), so do not
// duplicate the version number in CMakeLists.txt.

#define PDN_VERSION_MAJOR 0
#define PDN_VERSION_MINOR 0
#define PDN_VERSION_PATCH 1

#define PDN_VERSION_STRING "0.0.1"

// Encode a version as one comparable integer.
// Each component must be in [0, 999].
#define PDN_VERSION_ENCODE(major, minor, patch) (((major) * 1000000) + ((minor) * 1000) + (patch))

#define PDN_VERSION_NUMBER PDN_VERSION_ENCODE(PDN_VERSION_MAJOR, PDN_VERSION_MINOR, PDN_VERSION_PATCH)

#define PDN_VERSION_AT_LEAST(major, minor, patch) (PDN_VERSION_NUMBER >= PDN_VERSION_ENCODE(major, minor, patch))

#endif
