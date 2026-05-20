#!/bin/sh
# The control server's docker spawn branch builds the full
# `xvfb-run -a <wine> <game.exe> server <map> ...` invocation and passes it
# as docker run args — the same argv shape as the bare-metal execvp path.
# We just exec it.
exec "$@"
