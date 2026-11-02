#!/usr/bin/env Rscript
# Generate shadow map using rayshader

# Check if rayshader is installed
if (!requireNamespace("rayshader", quietly = TRUE)) {
  stop("Package 'rayshader' is required. Install it with: install.packages('rayshader')")
}
library(rayshader)

# Read terrain heightmap (512x512 float32)
if (!file.exists("assets/terrain.raw")) {
  stop("assets/terrain.raw not found!")
}

con <- file("assets/terrain.raw", "rb")
h <- readBin(con, numeric(), n = 512 * 512, size = 4)
close(con)

# Reshape to matrix
h <- matrix(h, nrow = 512, ncol = 512, byrow = TRUE)

# Generate ray-traced shadow map
# sunangle: 0=North, 90=East, 180=South, 270=West
# sunaltitude: 0=horizon, 90=zenith
cat("Generating shadow map with rayshader...\n")
shadow <- ray_shade(h, 
                    sunaltitude = 45, 
                    sunangle = 315,
                    zscale = 5.0,
                    maxsearch = 100)

# Ensure output directory exists
if (!dir.exists("output")) dir.create("output")

# Write as raw float32
con <- file("output/shadowmap.raw", "wb")
writeBin(as.vector(shadow), con, size = 4)
close(con)

cat("Shadow map generated: output/shadowmap.raw\n")
