#!/usr/bin/env Rscript
# 3D Weight Calculator for 3DAI Engine
# Calculates volumetric weights and spatial distribution for assets

# Load required libraries
if (!requireNamespace("jsonlite", quietly = TRUE)) {
  stop("Package 'jsonlite' is required.")
}
library(jsonlite)

# Configuration
input_file <- "assets/asset_weights.json"
output_file <- "output/weights_3d.json"

# Check if input exists
if (!file.exists(input_file)) {
  # Create dummy data if missing
  cat("Input file not found. Generating default weights...\n")
  assets <- data.frame(
    name = c("concrete_barrier", "steel_beam", "wooden_pallet"),
    density_kg_m3 = c(2400, 7850, 600),
    volume_m3 = c(0.5, 1.2, 0.8)
  )
} else {
  assets <- fromJSON(input_file)
}

# Calculate 3D weights (Mass = Density * Volume)
assets$mass_kg <- assets$density_kg_m3 * assets$volume_m3

# Calculate center of mass offset (assuming uniform density, COM is at geometric center)
# In a real scenario, this would integrate over the 3D mesh volume
assets$com_offset_x <- 0.0
assets$com_offset_y <- assets$volume_m3^(1/3) / 2.0
assets$com_offset_z <- 0.0

# Apply Z-axis scaling factor for engine units (meters to engine units)
engine_scale <- 1.0
assets$engine_mass <- assets$mass_kg * engine_scale

# Save results
if (!dir.exists("output")) dir.create("output")
write_json(assets, output_file, pretty = TRUE, auto_unbox = TRUE)

cat(sprintf("Processed %d assets. Output written to %s\n", nrow(assets), output_file))
