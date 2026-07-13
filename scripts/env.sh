#!/usr/bin/env bash
# Source this file from a WSL shell to use the project PlatformIO environment.

export PATH="$HOME/.platformio/penv/bin:$PATH"

echo "Project environment loaded."
echo "pio: $(command -v pio)"
pio --version
