BUILD_DIR := build
CMAKE     := cmake
CMAKE_FLAGS :=

.PHONY: all configure build clean install test

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DHR_BUILD_EXAMPLES=ON

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

release:
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S . \
		-DCMAKE_BUILD_TYPE=Release \
		-DHR_BUILD_EXAMPLES=OFF \
		-DHR_BUILD_SHARED=ON
	$(CMAKE) --build $(BUILD_DIR) --parallel

static:
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S . \
		-DCMAKE_BUILD_TYPE=Release \
		-DHR_BUILD_SHARED=OFF
	$(CMAKE) --build $(BUILD_DIR) --parallel

install: release
	$(CMAKE) --install $(BUILD_DIR) --prefix $(PREFIX)

demo_c: build
	@cd examples/demo_c && ../../$(BUILD_DIR)/examples/demo_c/demo_c

clean:
	rm -rf $(BUILD_DIR) .hotreload_build

info:
	@echo "Platform : $$(uname -s)"
	@echo "Arch     : $$(uname -m)"
	@which gcc   && gcc --version   | head -1 || echo "gcc not found"
	@which g++   && g++ --version   | head -1 || echo "g++ not found"
	@which rustc && rustc --version          || echo "rustc not found"
	@which zig   && zig version              || echo "zig not found"
	@which go    && go version               || echo "go not found"
