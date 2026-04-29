// Single-translation-unit definition of the cgltf implementation.
//
// cgltf is a single-header glTF 2.0 parser. The standard pattern is to define
// CGLTF_IMPLEMENTATION in exactly ONE .cpp before including the header — every
// other site that #include <cgltf/cgltf.h> picks up declarations only.
//
// This file exists solely to host that one definition. Keep it tiny so the
// header's ~7000 lines compile in one place rather than poisoning every TU
// that needs glTF lookups.
//
// Adopted at Phase F Stage 1 closeout (Stage 1 still uses our hand-written
// minimal subset parser in MeshImporter.cpp). Stage 2 (skinned mesh +
// animation) will replace MeshImporter's parsing with cgltf calls; this stub
// proves cgltf compiles cleanly under our toolchain so Stage 2 doesn't run
// into vendor-side surprises.

// cgltf uses raw strcpy/strncpy for short, pre-bounded string copies (e.g.
// the version field). The project's SDL check promotes the resulting C4996
// "unsafe function" warning to an error project-wide. Silence it for THIS
// translation unit only; the rest of the codebase keeps the strict policy.
#define _CRT_SECURE_NO_WARNINGS

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
