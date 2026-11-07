#pragma once
// Commit id of the published source this binary was built from, so anyone can match a binary to its source bundle:
//   strings dinput8.dll | grep GA-SOURCE-REVISION
// src/Shared/SourceRevisionId.inc holds the id as a quoted string. The release tool writes it at build time and the
// source bundle carries it, because a commit cannot contain its own id. Without that file the id reads "unknown".
static const char kGaSourceRevision[] = "GA-SOURCE-REVISION="
#if __has_include("src/Shared/SourceRevisionId.inc")
#include "src/Shared/SourceRevisionId.inc"
#else
"unknown"
#endif
;
