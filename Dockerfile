# =============================================================================
# Stage 1: Build the C++ executable on Linux using GCC
# =============================================================================
FROM gcc:bookworm AS builder

WORKDIR /app

# Copy C++ core algorithms and server files
COPY Graph.h Graph.cpp Metro.h Metro.cpp server.cpp ./

# Compile using Linux g++ with optimizations and static libstdc++/libgcc linkage
RUN g++ -std=c++14 -O2 -static-libstdc++ -static-libgcc server.cpp Graph.cpp Metro.cpp -o metro_server

# =============================================================================
# Stage 2: Runtime image using the same GCC base image (Guarantees libstdc++ compatibility)
# =============================================================================
FROM gcc:bookworm

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/metro_server ./metro_server

# Copy the static web frontend assets
COPY frontend/ ./frontend/

# Default port (Cloud hosting platforms like Render override $PORT dynamically)
ENV PORT=8080

# Expose port
EXPOSE 8080

# Execute the native C++ backend server
CMD ["./metro_server"]
