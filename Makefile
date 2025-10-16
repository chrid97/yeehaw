# ─────────────────────────────────────────────────────────────
# Works on macOS + Linux
# Usage:
#   make build   -> native build
#   make web     -> Emscripten web build
#   make run     -> run native build
#   make clean   -> remove build/
# ─────────────────────────────────────────────────────────────

.PHONY: build run web clean

SCRIPTS_DIR := scripts

watch:
	@bash $(SCRIPTS_DIR)/watch.sh

build:
	@bash $(SCRIPTS_DIR)/build.sh

run: 
	@bash $(SCRIPTS_DIR)/run.sh

web:
	@bash $(SCRIPTS_DIR)/web-build.sh

clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf build
	@echo "✅ Clean complete!"
