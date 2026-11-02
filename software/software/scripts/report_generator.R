#!/usr/bin/env Rscript

suppressWarnings({
  if (!requireNamespace("jsonlite", quietly = TRUE)) install.packages("jsonlite", repos="https://cloud.r-project.org", quiet=TRUE)
  if (!requireNamespace("ggplot2", quietly = TRUE)) install.packages("ggplot2", repos="https://cloud.r-project.org", quiet=TRUE)
  if (!requireNamespace("gridExtra", quietly = TRUE)) install.packages("gridExtra", repos="https://cloud.r-project.org", quiet=TRUE)
  library(jsonlite)
  library(ggplot2)
  library(gridExtra)
})

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 2) {
  cat("Usage: Rscript report_generator.R <scene.json> <output.pdf>\n")
  quit(status = 1)
}

scene_file <- args[1]
output_file <- args[2]

data <- fromJSON(scene_file)

pdf(output_file, width = 8, height = 10)
grid.text("3DAI Engine - Scene Report", x = 0.5, y = 0.9, gp = gpar(fontsize = 20, fontface = "bold"))

if (is.null(data$objects)) {
  grid.text("No objects in scene.", x = 0.5, y = 0.5)
} else {
  obj_count <- nrow(data$objects)
  grid.text(sprintf("Total Objects: %d", obj_count), x = 0.5, y = 0.8, gp = gpar(fontsize = 16))
  
  df <- data.frame(
    Name = data$objects$name,
    X = sapply(data$objects$pos, function(p) p$x),
    Y = sapply(data$objects$pos, function(p) p$y),
    Z = sapply(data$objects$pos, function(p) p$z),
    stringsAsFactors = FALSE
  )
  
  grid.table(df)
  
  p <- ggplot(df, aes(x = X, y = Z)) +
    geom_point(color = "blue", size = 2) +
    labs(title = "Object Layout (Top-Down)", x = "X Coordinate", y = "Z Coordinate") +
    theme_minimal()
  print(p)
  
  grid.text("Note: Concrete volume and material estimates require asset metadata extension.", x = 0.5, y = 0.05, gp = gpar(fontsize = 10, col = "gray40"))
}
dev.off()

cat("[REPORT] Saved to:", output_file, "\n")

