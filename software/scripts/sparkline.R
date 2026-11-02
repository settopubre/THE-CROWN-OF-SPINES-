#!/usr/bin/env Rscript

suppressWarnings({
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    install.packages("ggplot2", repos = "https://cloud.r-project.org", quiet = TRUE)
  }
  library(ggplot2)
})

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 2) {
  cat("Usage: Rscript sparkline.R <data.csv> <output.png> [--title \"Title\"]\n")
  quit(status = 1)
}

data_file <- args[1]
output_file <- args[2]
title <- if ("--title" %in% args) args[which(args == "--title") + 1] else ""

cat("[SPARKLINE] Loading data:", data_file, "\n")

data <- read.csv(data_file, header = TRUE)
if (nrow(data) == 0) {
  cat("[SPARKLINE] Error: empty data file\n")
  quit(status = 1)
}

p <- ggplot(data, aes(x = seq_len(nrow(data)), y = value)) +
  geom_line(color = "#4A90E2", size = 1.2) +
  theme_void() +
  theme(
    plot.background = element_rect(fill = "transparent", color = NA),
    panel.background = element_rect(fill = "transparent", color = NA)
  )

if (nzchar(title)) {
  p <- p + labs(title = title) +
    theme(plot.title = element_text(hjust = 0.5, color = "white", size = 10))
}

ggsave(output_file, plot = p, width = 4, height = 1, bg = "transparent", dpi = 150)
cat("[SPARKLINE] Saved:", output_file, "\n")
