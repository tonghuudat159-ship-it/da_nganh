# ============================================================
# Smart Home CE Firmware - Makefile
# Platform: ESP32 YOLO UNO
# ============================================================

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Default target
.DEFAULT_GOAL := help

# ============================================================
# VARIABLES
# ============================================================

PROJECT_NAME := esp32-freertos-tinyml-monitor
BOARD := yolo_uno
BAUD_RATE := 115200
UPLOAD_PORT ?= /dev/ttyUSB0
PIO := pio

# ============================================================
# HELP TARGET
# ============================================================

.PHONY: help
help:
	@echo "$(BLUE)╔════════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Smart Home CE Firmware - Makefile                        ║$(NC)"
	@echo "$(BLUE)║  Platform: ESP32 YOLO UNO                                 ║$(NC)"
	@echo "$(BLUE)╚════════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(GREEN)Build & Compile:$(NC)"
	@echo "  $(YELLOW)make build$(NC)           - Build firmware (pio run)"
	@echo "  $(YELLOW)make rebuild$(NC)         - Clean and rebuild"
	@echo "  $(YELLOW)make clean$(NC)           - Clean build artifacts"
	@echo "  $(YELLOW)make re$(NC)              - Alias for rebuild"
	@echo ""
	@echo "$(GREEN)Flash & Upload:$(NC)"
	@echo "  $(YELLOW)make upload$(NC)          - Build and upload to device"
	@echo "  $(YELLOW)make upload-only$(NC)     - Upload without build"
	@echo "  $(YELLOW)make upload PORT=/dev/ttyUSB1$(NC) - Upload to specific port"
	@echo ""
	@echo "$(GREEN)Monitoring & Debug:$(NC)"
	@echo "  $(YELLOW)make monitor$(NC)         - Open serial monitor (115200 baud)"
	@echo "  $(YELLOW)make monitor PORT=/dev/ttyUSB1$(NC) - Monitor specific port"
	@echo "  $(YELLOW)make serial$(NC)          - Alias for monitor"
	@echo "  $(YELLOW)make logs$(NC)            - Monitor with tail (live logs)"
	@echo ""
	@echo "$(GREEN)Device Management:$(NC)"
	@echo "  $(YELLOW)make erase$(NC)           - Erase ESP32 flash memory"
	@echo "  $(YELLOW)make size$(NC)            - Show memory usage (RAM/Flash)"
	@echo "  $(YELLOW)make format-fs$(NC)       - Format LittleFS filesystem"
	@echo ""
	@echo "$(GREEN)Debugging:$(NC)"
	@echo "  $(YELLOW)make verbose-build$(NC)   - Build with verbose output"
	@echo "  $(YELLOW)make check$(NC)           - Check syntax without building"
	@echo "  $(YELLOW)make deps$(NC)            - Update dependencies"
	@echo ""
	@echo "$(GREEN)Project Info:$(NC)"
	@echo "  $(YELLOW)make info$(NC)            - Show project info"
	@echo "  $(YELLOW)make lib-list$(NC)        - List all libraries"
	@echo "  $(YELLOW)make version$(NC)         - Show PlatformIO version"
	@echo ""
	@echo "$(GREEN)All-in-One:$(NC)"
	@echo "  $(YELLOW)make all$(NC)             - Build, size, info"
	@echo "  $(YELLOW)make dev$(NC)             - Build, upload, and monitor"
	@echo ""

# ============================================================
# BUILD TARGETS
# ============================================================

.PHONY: build
build:
	@echo "$(BLUE)[BUILD]$(NC) Compiling firmware for $(BOARD)..."
	@$(PIO) run -e $(BOARD)
	@echo "$(GREEN)[OK]$(NC) Build complete!"

.PHONY: rebuild
rebuild: clean build
	@echo "$(GREEN)[OK]$(NC) Rebuild complete!"

.PHONY: re
re: rebuild

.PHONY: clean
clean:
	@echo "$(BLUE)[CLEAN]$(NC) Removing build artifacts..."
	@$(PIO) run -e $(BOARD) --target clean
	@echo "$(GREEN)[OK]$(NC) Clean complete!"

.PHONY: size
size:
	@echo "$(BLUE)[SIZE]$(NC) Memory usage:"
	@$(PIO) run -e $(BOARD) --target size

# ============================================================
# UPLOAD TARGETS
# ============================================================

.PHONY: upload
upload: build
	@echo "$(BLUE)[UPLOAD]$(NC) Uploading to $(UPLOAD_PORT) ($(BOARD))..."
	@$(PIO) run -e $(BOARD) --target upload --upload-port $(UPLOAD_PORT)
	@echo "$(GREEN)[OK]$(NC) Upload complete!"

.PHONY: upload-only
upload-only:
	@echo "$(BLUE)[UPLOAD]$(NC) Uploading to $(UPLOAD_PORT) without build..."
	@$(PIO) run -e $(BOARD) --target upload --upload-port $(UPLOAD_PORT)
	@echo "$(GREEN)[OK]$(NC) Upload complete!"

# ============================================================
# SERIAL MONITOR
# ============================================================

.PHONY: monitor
monitor:
	@echo "$(BLUE)[MONITOR]$(NC) Opening serial monitor on $(UPLOAD_PORT)@$(BAUD_RATE)..."
	@echo "$(YELLOW)Press Ctrl+C to exit$(NC)"
	@$(PIO) device monitor -p $(UPLOAD_PORT) -b $(BAUD_RATE)

.PHONY: serial
serial: monitor

.PHONY: logs
logs:
	@echo "$(BLUE)[LOGS]$(NC) Showing live serial logs..."
	@echo "$(YELLOW)Press Ctrl+C to exit$(NC)"
	@tail -f /dev/$(UPLOAD_PORT) 2>/dev/null || echo "$(RED)Cannot open $(UPLOAD_PORT)$(NC)"

# ============================================================
# DEVICE MANAGEMENT
# ============================================================

.PHONY: erase
erase:
	@echo "$(YELLOW)[WARNING]$(NC) Erasing ESP32 flash memory..."
	@read -p "Are you sure? (y/N): " confirm && [ "$${confirm}" = "y" ] && \
		$(PIO) run -e $(BOARD) --target erase || echo "Cancelled."

.PHONY: format-fs
format-fs:
	@echo "$(YELLOW)[WARNING]$(NC) Formatting LittleFS filesystem..."
	@read -p "Are you sure? (y/N): " confirm && [ "$${confirm}" = "y" ] && \
		$(PIO) run -e $(BOARD) --target buildfs || echo "Cancelled."

# ============================================================
# DEBUGGING & ANALYSIS
# ============================================================

.PHONY: verbose-build
verbose-build:
	@echo "$(BLUE)[BUILD VERBOSE]$(NC) Building with verbose output..."
	@$(PIO) run -e $(BOARD) -v

.PHONY: check
check:
	@echo "$(BLUE)[CHECK]$(NC) Running syntax check..."
	@$(PIO) check -e $(BOARD)

.PHONY: deps
deps:
	@echo "$(BLUE)[DEPS]$(NC) Updating dependencies..."
	@$(PIO) lib update

.PHONY: lib-list
lib-list:
	@echo "$(BLUE)[LIBRARIES]$(NC) Installed libraries:"
	@$(PIO) lib list

# ============================================================
# PROJECT INFORMATION
# ============================================================

.PHONY: info
info:
	@echo "$(BLUE)╔════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Project Information                                  ║$(NC)"
	@echo "$(BLUE)╚════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(GREEN)Project:$(NC) $(PROJECT_NAME)"
	@echo "$(GREEN)Board:$(NC) $(BOARD)"
	@echo "$(GREEN)Serial Port:$(NC) $(UPLOAD_PORT)"
	@echo "$(GREEN)Baud Rate:$(NC) $(BAUD_RATE)"
	@echo ""
	@$(PIO) boards $(BOARD) 2>/dev/null | head -20 || echo "Board info unavailable"
	@echo ""

.PHONY: version
version:
	@echo "$(BLUE)PlatformIO Version:$(NC)"
	@$(PIO) --version

# ============================================================
# COMBINED TARGETS
# ============================================================

.PHONY: all
all: clean build size info
	@echo "$(GREEN)[OK]$(NC) All targets complete!"

.PHONY: dev
dev: build upload monitor
	@echo "$(GREEN)[OK]$(NC) Development cycle complete!"

.PHONY: quick
quick: build upload
	@echo "$(GREEN)[OK]$(NC) Build and upload complete!"

# ============================================================
# UTILITY TARGETS
# ============================================================

.PHONY: status
status:
	@echo "$(BLUE)[STATUS]$(NC) Checking project configuration..."
	@test -f platformio.ini && echo "$(GREEN)✓$(NC) platformio.ini found" || echo "$(RED)✗$(NC) platformio.ini missing"
	@test -d src && echo "$(GREEN)✓$(NC) src/ directory found" || echo "$(RED)✗$(NC) src/ directory missing"
	@test -d include && echo "$(GREEN)✓$(NC) include/ directory found" || echo "$(RED)✗$(NC) include/ directory missing"
	@test -d lib && echo "$(GREEN)✓$(NC) lib/ directory found" || echo "$(RED)✗$(NC) lib/ directory missing"
	@echo ""
	@echo "$(GREEN)File Statistics:$(NC)"
	@find src -name "*.cpp" -o -name "*.h" | wc -l | xargs echo "Source files:"
	@find include -name "*.h" | wc -l | xargs echo "Header files:"

.PHONY: tree
tree:
	@echo "$(BLUE)[TREE]$(NC) Project structure:"
	@tree -L 2 -I '.pio|.git' 2>/dev/null || find . -maxdepth 2 -type d ! -path '*/\.*' | sort

.PHONY: cppcheck
cppcheck:
	@echo "$(BLUE)[CPPCHECK]$(NC) Running static analysis..."
	@cppcheck --enable=all src/ include/ 2>/dev/null || echo "cppcheck not installed"

.PHONY: tags
tags:
	@echo "$(BLUE)[TAGS]$(NC) Generating ctags..."
	@ctags -R src/ include/ lib/ 2>/dev/null && echo "$(GREEN)✓$(NC) Tags generated" || echo "ctags not installed"

# ============================================================
# GIT TARGETS
# ============================================================

.PHONY: status-git
status-git:
	@echo "$(BLUE)[GIT STATUS]$(NC)"
	@git status --short || echo "Not a git repository"

.PHONY: log-git
log-git:
	@echo "$(BLUE)[GIT LOG]$(NC) Last 5 commits:"
	@git log --oneline -5 2>/dev/null || echo "Not a git repository"

.PHONY: diff
diff:
	@git diff --stat 2>/dev/null || echo "Not a git repository"

# ============================================================
# DOCUMENTATION
# ============================================================

.PHONY: docs
docs:
	@echo "$(BLUE)[DOCS]$(NC) Generating documentation..."
	@doxygen Doxyfile 2>/dev/null && echo "$(GREEN)✓$(NC) Documentation generated" || echo "Doxyfile not found"

# ============================================================
# CLEANING
# ============================================================

.PHONY: distclean
distclean: clean
	@echo "$(YELLOW)[DISTCLEAN]$(NC) Removing all generated files..."
	@rm -rf .pio build/ *.o *.a debug/ release/
	@echo "$(GREEN)[OK]$(NC) Distclean complete!"

.PHONY: clobber
clobber: distclean
	@rm -rf .vscode/ tags TAGS .tags
	@echo "$(GREEN)[OK]$(NC) Clobber complete!"

# ============================================================
# PHONY TARGETS
# ============================================================

.PHONY: default
