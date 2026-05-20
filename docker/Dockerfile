# Game-instance sandbox image.
#
# The image deliberately does NOT contain:
#   - the game install     (bind-mounted from host: cfg.game_binary parent dir)
#   - the wineprefix       (bind-mounted from host: per slot)
#   - the wine binary      (bind-mounted from host: cfg.wine_install_dir)
#   - the DLL              (lives inside the game install dir on host)
#   - the SQLite DB        (bind-mounted from host: control-server CWD)
#
# All of those are scp-deployed to the host and bind-mounted in at run time
# by the control server's docker spawn branch. The image is just "linux +
# xvfb + the runtime libs wine links against." Rebuild only when one of
# those system deps changes — usually never.
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 && \
    apt-get update && apt-get install -y --no-install-recommends \
        xvfb \
        xauth \
        ca-certificates \
        libc6:i386 \
        libstdc++6:i386 \
        libgl1 libgl1:i386 \
        libasound2 libasound2:i386 \
        libpulse0 libpulse0:i386 \
        libfreetype6 libfreetype6:i386 \
        libfontconfig1 libfontconfig1:i386 \
        libxml2 libxml2:i386 \
        libxcomposite1 libxcomposite1:i386 \
        libxcursor1 libxcursor1:i386 \
        libxi6 libxi6:i386 \
        libxrandr2 libxrandr2:i386 \
        libxinerama1 libxinerama1:i386 \
        libxext6 libxext6:i386 \
        libxrender1 libxrender1:i386 \
        libxtst6 libxtst6:i386 \
        libsm6 libsm6:i386 \
        libice6 libice6:i386 \
        libxt6 libxt6:i386 \
        libgnutls30 libgnutls30:i386 \
    && rm -rf /var/lib/apt/lists/*

# Fake a machine-id so wine/dbus probes don't error out. Real value doesn't
# matter for our use case (we never federate identity across containers).
RUN echo "0123456789abcdef0123456789abcdef" > /etc/machine-id

# Pre-create a world-writable fontconfig cache dir at a predictable path.
# The spawn line sets HOME to this so fontconfig stops complaining.
RUN mkdir -p /var/cache/ga-server/home && chmod 777 /var/cache/ga-server/home

# Add a passwd/group entry for the host user (UID/GID 1000) so getpwuid()
# from inside the container resolves instead of returning NULL. Wine and
# PhysX both call this — without it some code paths null-deref. Wine also
# uses the username to resolve WINEPREFIX/drive_c/users/<name>/ so the
# name MUST match whatever subdir already exists in the prefix on host
# (set up by the original wineprefix install). If your host UID is
# something other than 1000 or your prefix uses a different user dir,
# adjust these two lines.
RUN echo 'zax:x:1000:1000::/var/cache/ga-server/home:/bin/sh' >> /etc/passwd && \
    echo 'zax:x:1000:' >> /etc/group

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
