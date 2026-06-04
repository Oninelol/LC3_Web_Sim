CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Isrc
SRC       = src/cpu.cpp src/memory.cpp
EMCC     ?= emcc
EMFLAGS   = -std=c++17 -O2 -Isrc --bind \
            -s MODULARIZE=1 -s EXPORT_NAME=createLC3 -s EXPORT_ES6=1 \
            -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1
 
.PHONY: all wasm test native serve clean
 
all: native
 

native: lc3_simulator
lc3_simulator: $(SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/main.cpp -o $@
 

test: $(SRC) src/test.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/test.cpp -o src/test
	./src/test
 

wasm: www/lc3.js
www/lc3.js: $(SRC) src/bindings.cpp
	$(EMCC) $(EMFLAGS) $(SRC) src/bindings.cpp -o www/lc3.js
	@echo "Built www/lc3.js + www/lc3.wasm — commit both for GitHub Pages."
 

serve:
	@echo "Serving on http://localhost:8000/www/  (Ctrl-C to stop)"
	python3 -m http.server 8000
 
clean:
	rm -f lc3_simulator src/test www/lc3.js www/lc3.wasm