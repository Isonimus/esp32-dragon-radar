#pragma once

// The release job passes the git tag in as a build flag (ADR-0008). Any other build
// is a development build, and says so on the title screen instead of claiming a
// version it was not released as.
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev"
#endif
