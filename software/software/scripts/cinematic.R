#!/usr/bin/env Rscript

suppressWarnings({
  if (!requireNamespace("rayshader", quietly = TRUE)) {
    install.packages("rayshader", repos = "https://cloud.r-project.org", quiet = TRUE)
  }
  if (!requireNamespace("rgl", quietly = TRUE)) {
    install.packages("rgl", repos = "https://cloud.r-project.org", quiet = TRUE)
  }
  library(rayshader)
  library(rgl)
})

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 2) {
  cat("Usage: Rscript cinematic.R <scene.obj> <output.png> [--dof] [--samples N]\n")
  quit(status = 1)
}

scene_file <- args[1]
output_file <- args[2]
use_dof <- "--dof" %in% args
samples <- 64
if ("--samples" %in% args) {
  idx <- which(args == "--samples")
  if (idx < length(args)) samples <- as.integer(args[idx + 1])
}

cat("[CINEMATIC] Loading scene:", scene_file, "\n")

tryCatch({
  open3d()
  par3d(windowRect = c(100, 100, 1200, 900))
  
  rgl::readOBJ(scene_file)
  
  cat("[CINEMATIC] Setting up render...\n")
  
  if (use_dof) {
    render_highquality(theta = 45, phi = 30, zoom = 0.7,
                       samples = samples, aperture = 0.02, focal_distance = 5,
                       background = "white")
  } else {
    render_highquality(theta = 45, phi = 30, zoom = 0.7,
                       samples = samples, background = "white")
  }
  
  cat("[CINEMATIC] Saving to", output_file, "\n")
  render_snapshot(filename = output_file, clear = TRUE)
  cat("[CINEMATIC] Done.\n")
  
}, error = function(e) {
  cat("[CINEMATIC] Error:", conditionMessage(e), "\n")
  quit(status = 1)
})
