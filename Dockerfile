# Stage 1: Build stage
FROM gcc:13 AS builder

WORKDIR /app
COPY . .
RUN make clean && make release

# Stage 2: Runtime stage (Image size chhoti rakhne ke liye)
FROM debian:bookworm-slim

WORKDIR /app

# Zaroori libraries copy karo
COPY --from=builder /app/build/kvdb /usr/local/bin/kvdb

# Non-root user banao (Security best practice)
RUN useradd -m kvdbuser && chown -R kvdbuser:kvdbuser /app
USER kvdbuser

# Port expose karo
EXPOSE 6380

# Default command
CMD ["kvdb", "--port", "6380"]
