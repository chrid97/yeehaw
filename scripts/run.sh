#!/bin/bash

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
cd "$PROJECT_ROOT"

$PROJECT_ROOT/scripts/build.sh && $PROJECT_ROOT/build/horse-riding
