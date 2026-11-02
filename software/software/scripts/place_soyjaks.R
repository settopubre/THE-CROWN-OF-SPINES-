#!/usr/bin/env Rscript
# Generate soyjak placement Lua script based on terrain heightmap

# Read terrain heightmap (512x512 float32)
if (!file.exists("assets/terrain.raw")) {
  stop("assets/terrain.raw not found!")
}

con <- file("assets/terrain.raw", "rb")
h <- readBin(con, numeric(), n = 512 * 512, size = 4)
close(con)

h <- matrix(h, nrow = 512, ncol = 512, byrow = TRUE)

# Bilinear interpolation function to get exact height
get_height <- function(x, z) {
  x <- max(0, min(511, x))
  z <- max(0, min(511, z))
  
  x0 <- floor(x)
  x1 <- min(511, x0 + 1)
  z0 <- floor(z)
  z1 <- min(511, z0 + 1)
  
  fx <- x - x0
  fz <- z - z0
  
  # R matrices are 1-indexed
  h00 <- h[z0 + 1, x0 + 1]
  h10 <- h[z0 + 1, x1 + 1]
  h01 <- h[z1 + 1, x0 + 1]
  h11 <- h[z1 + 1, x1 + 1]
  
  h_x0 <- h00 * (1 - fx) + h10 * fx
  h_x1 <- h01 * (1 - fx) + h11 * fx
  
  return(h_x0 * (1 - fz) + h_x1 * fz)
}

# Generate 13 random positions
set.seed(42)
positions <- data.frame(
  x = runif(13, 10, 500),
  z = runif(13, 10, 500)
)

# Interpolate heights
positions$y <- mapply(get_height, positions$x, positions$z)

# Write Lua script
lua_file <- file("scripts/soyjak_positions.lua", "w")
for (i in 1:13) {
  line <- sprintf('engine_place_asset("soyjak_%02d", %.2f, %.2f, %.2f)\n', 
                  i, positions$x[i], positions$y[i], positions$z[i])
  cat(line, file = lua_file)
}
close(lua_file)

cat("Generated: scripts/soyjak_positions.lua\n")
