#!/bin/bash
# ─────────────────────────────────────────────
# Shared environment for all project scripts
# Usage:  source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
# ─────────────────────────────────────────────

# --- Determine project root (absolute path) ---
if [[ -z "${PROJECT_ROOT:-}" ]]; then
  PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi

# --- Common paths ---
BUILD_DIR=${PROJECT_ROOT}/build
LIB_DIR=${PROJECT_ROOT}/lib
