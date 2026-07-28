FROM python:3.12-alpine

WORKDIR /app
COPY overlay_proxy.py spectator-overlay.html skill-tree-overlay.html ./

# Icon pack is mounted at runtime (see docker-compose.yml) — the game's icon
# images are not checked into the repo.

ENV BIND=0.0.0.0 \
    PORT=8091

EXPOSE 8091
CMD ["python", "overlay_proxy.py"]
