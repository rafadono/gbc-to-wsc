#!/usr/bin/env python3
"""
Applies Super Mario Land Widescreen Enhancement Patch to decompiled C source files.
Modifies camera scroll thresholds, initial tilemap column population, and entity bounds.
"""

import sys
import os
import re

BASE_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RECOMP_DIR = os.path.join(BASE_DIR, "build", "recompiled", "super_marioland")
INTERNAL_H = os.path.join(RECOMP_DIR, "Super_Mario_Land__World___Rev_A__internal.h")
FUNCS_1_C = os.path.join(RECOMP_DIR, "Super_Mario_Land__World___Rev_A__funcs_1.c")
HOOKS_H = os.path.join(BASE_DIR, "ports", "super_marioland", "hooks", "sml_hooks.h")

def check_prerequisites():
    if not os.path.isdir(RECOMP_DIR):
        print(f"Error: Recompiled directory not found: {RECOMP_DIR}")
        print("Please run pipeline first: python tools/pipeline.py \"roms/Super Mario Land (World) (Rev A).gb\"")
        return False
    if not os.path.isfile(INTERNAL_H) or not os.path.isfile(FUNCS_1_C):
        print("Error: Required generated C files not found in recompiled directory.")
        return False
    return True

def apply_patch():
    if not check_prerequisites():
        return False

    print("--- Applying Super Mario Land Widescreen Patch ---")

    # 1. Update Super_Mario_Land__World___Rev_A__internal.h
    with open(INTERNAL_H, "r", encoding="utf-8") as f:
        internal_content = f.read()

    include_decl = '#include "sml_hooks.h"\n'
    if include_decl not in internal_content:
        # Insert right after #include "Super_Mario_Land__World___Rev_A_.h"
        target = '#include "Super_Mario_Land__World___Rev_A_.h"\n'
        if target in internal_content:
            internal_content = internal_content.replace(target, target + include_decl)
            with open(INTERNAL_H, "w", encoding="utf-8") as f:
                f.write(internal_content)
            print("Patched: Injected sml_hooks.h into Super_Mario_Land__World___Rev_A__internal.h")
        else:
            print("Warning: Could not locate include anchor in internal.h")
    else:
        print("Already included: sml_hooks.h in internal.h")

    # 2. Patch Super_Mario_Land__World___Rev_A__funcs_1.c
    with open(FUNCS_1_C, "r", encoding="utf-8") as f:
        funcs_content = f.read()

    # Hook A: Initial column population at PC 0x1326
    # Original: /* 1326 */     ctx->a = 0x54;
    # Patched:  /* 1326 */     ctx->a = sml_hook_initial_column_end(0x54);
    orig_a = "/* 1326 */     ctx->a = 0x54;"
    patch_a = "/* 1326 */     ctx->a = sml_hook_initial_column_end(0x54);"
    if orig_a in funcs_content:
        funcs_content = funcs_content.replace(orig_a, patch_a)
        print("Patched: Initial column end count hooked (0x1326)")
    elif patch_a in funcs_content:
        print("Already patched: Initial column end count (0x1326)")
    else:
        print("Warning: Anchor for 0x1326 initial column count not found")

    # Hook B: Camera scroll trigger threshold at PC 0x11ec
    # Original: /* 11ec */     gb_cp8(ctx, 0x50);
    # Patched:  /* 11ec */     gb_cp8(ctx, sml_hook_scroll_threshold(0x50));
    orig_b = "/* 11ec */     gb_cp8(ctx, 0x50);"
    patch_b = "/* 11ec */     gb_cp8(ctx, sml_hook_scroll_threshold(0x50));"
    if orig_b in funcs_content:
        funcs_content = funcs_content.replace(orig_b, patch_b)
        print("Patched: Camera scroll threshold hooked (0x11ec)")
    elif patch_b in funcs_content:
        print("Already patched: Camera scroll threshold (0x11ec)")
    else:
        print("Warning: Anchor for 0x11ec scroll threshold not found")

    with open(FUNCS_1_C, "w", encoding="utf-8") as f:
        f.write(funcs_content)

    print("Success: Super Mario Land widescreen patch applied successfully.")
    return True

def revert_patch():
    if not check_prerequisites():
        return False

    print("--- Reverting Super Mario Land Widescreen Patch ---")
    with open(INTERNAL_H, "r", encoding="utf-8") as f:
        content = f.read()
    include_decl = '#include "sml_hooks.h"\n'
    if include_decl in content:
        content = content.replace(include_decl, "")
        with open(INTERNAL_H, "w", encoding="utf-8") as f:
            f.write(content)
        print("Reverted: Removed sml_hooks.h from internal.h")

    with open(FUNCS_1_C, "r", encoding="utf-8") as f:
        funcs = f.read()
    patch_a = "/* 1326 */     ctx->a = sml_hook_initial_column_end(0x54);"
    orig_a = "/* 1326 */     ctx->a = 0x54;"
    patch_b = "/* 11ec */     gb_cp8(ctx, sml_hook_scroll_threshold(0x50));"
    orig_b = "/* 11ec */     gb_cp8(ctx, 0x50);"

    funcs = funcs.replace(patch_a, orig_a)
    funcs = funcs.replace(patch_b, orig_b)
    with open(FUNCS_1_C, "w", encoding="utf-8") as f:
        f.write(funcs)
    print("Reverted: Restored vanilla camera values in funcs_1.c")
    return True

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--revert":
        revert_patch()
    else:
        apply_patch()
