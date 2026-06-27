CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
BUILDDIR := build
TARGET   := $(BUILDDIR)/riscv-sim

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@
	@echo "==> Compilado: $(TARGET)"

$(BUILDDIR)/%.o: src/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) tests/riscvtest.bin

clean:
	rm -rf $(BUILDDIR)
	@echo "==> Limpio."
