.PHONY: all build test bench clean run

BUILD_DIR = build

all: build

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

build: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

test: build
	./$(BUILD_DIR)/test_fs

bench: build
	python3 scripts/run_benchmarks.py

run: build
	./$(BUILD_DIR)/fs_app

clean:
	rm -rf $(BUILD_DIR) virtual_disk.bin
