# Makefile for Serpent Programming Language
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -pedantic -std=c11 -O3
LDFLAGS = -lwayland-client -lcurl -lxkbcommon -lm

SRC_DIR = src
OBJ_DIR = obj
WAYLAND_DIR = $(SRC_DIR)/wayland

# Wayland Protocols
XDG_SHELL_XML = /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
XDG_SHELL_HDR = $(WAYLAND_DIR)/xdg-shell-protocol.h
XDG_SHELL_SRC = $(WAYLAND_DIR)/xdg-shell-protocol.c

DECORATION_XML = /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
DECORATION_HDR = $(WAYLAND_DIR)/xdg-decoration-protocol.h
DECORATION_SRC = $(WAYLAND_DIR)/xdg-decoration-protocol.c

PROTO_HDRS = $(XDG_SHELL_HDR) $(DECORATION_HDR)
PROTO_SRCS = $(XDG_SHELL_SRC) $(DECORATION_SRC)

# Ana kaynak dosyaları (runner_template.c ve otomatik üretilen protokol dosyası hariç)
SRCS = $(filter-out $(SRC_DIR)/compiler/runner_template.c $(PROTO_SRCS), $(shell find $(SRC_DIR) -name '*.c'))
# Protokol dosyasını SRCS'e ekle
SRCS += $(PROTO_SRCS)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = serpent

all: $(PROTO_HDRS) $(PROTO_SRCS) $(TARGET)

$(WAYLAND_DIR)/%-protocol.h: /usr/share/wayland-protocols/stable/%/%.xml
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner client-header $< $@

$(WAYLAND_DIR)/%-protocol.c: /usr/share/wayland-protocols/stable/%/%.xml
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner private-code $< $@

# Unstable protocols (xdg-decoration)
$(DECORATION_HDR): $(DECORATION_XML)
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner client-header $< $@

$(DECORATION_SRC): $(DECORATION_XML)
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner private-code $< $@

# Fallback for xdg-shell if the pattern above doesn't match exactly
$(XDG_SHELL_HDR): $(XDG_SHELL_XML)
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner client-header $< $@

$(XDG_SHELL_SRC): $(XDG_SHELL_XML)
	@mkdir -p $(WAYLAND_DIR)
	wayland-scanner private-code $< $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(PROTO_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I $(SRC_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	rm -f $(PROTO_HDRS) $(PROTO_SRCS)

.PHONY: all clean
