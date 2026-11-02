#!/usr/bin/env Rscript

options(warn = -1)

suppressPackageStartupMessages({
  suppressWarnings({
    if (!requireNamespace("igraph", quietly = TRUE)) {
      install.packages("igraph", repos = "https://cloud.r-project.org", quiet = TRUE)
    }
    if (!requireNamespace("rgl", quietly = TRUE)) {
      install.packages("rgl", repos = "https://cloud.r-project.org", quiet = TRUE)
    }
    library(igraph)
    library(rgl)
  })
})

cat("[R] Loading data...\n")
nodes <- read.csv("output/node_positions.csv", na.strings = "", check.names = FALSE)
nodes <- na.omit(nodes)
adj <- as.matrix(read.csv("output/adjacency_matrix.csv", header = FALSE, na.strings = "", check.names = FALSE, colClasses = "numeric"))
adj[is.na(adj)] <- 0

n_nodes <- nrow(nodes)
cat(sprintf("[R] Building graph: %d nodes", n_nodes))

adj[adj < 0.01] <- 0
g <- graph_from_adjacency_matrix(adj, mode = "undirected", weighted = TRUE, diag = FALSE)

n_edges <- ecount(g)
cat(sprintf(", %d edges\n", n_edges))

cat("[R] Applying RGB colors...\n")

tryCatch({
  open3d()
  par3d(windowRect = c(100, 100, 1100, 800))
  bg3d(color = "#1a1a2e")
}, error = function(e) {
  rgl.useNULL()
  open3d()
  par3d(windowRect = c(100, 100, 1100, 800))
  bg3d(color = "#1a1a2e")
})

coords <- as.matrix(nodes[, c("x", "y", "z")])
deg <- degree(g)
max_deg <- max(deg)
if (max_deg > .Machine$double.eps) {
  deg_norm <- deg / max_deg
} else {
  deg_norm <- rep(0, length(deg))
}
node_size <- 0.05 + deg_norm * 0.20

z_vals <- coords[,3]
z_min <- min(z_vals)
z_max <- max(z_vals)
if ((z_max - z_min) > .Machine$double.eps) {
  z_norm <- (z_vals - z_min) / (z_max - z_min)
} else {
  z_norm <- rep(0.5, length(z_vals))
}

r_vals <- pmin(1, pmax(0, z_norm))
g_vals <- rep(0.8, length(z_norm))
b_vals <- pmin(1, pmax(0, 1 - z_norm * 0.5))
node_colors <- rgb(r_vals, g_vals, b_vals)

spheres3d(coords[,1], coords[,2], coords[,3], 
          radius = node_size, 
          color = node_colors)

if (n_edges > 0) {
  edge_weights <- E(g)$weight
  max_weight <- max(edge_weights)
  if (max_weight > .Machine$double.eps) {
    w_norm <- edge_weights / max_weight
  } else {
    w_norm <- rep(0, length(edge_weights))
  }
  edge_alpha <- 0.15 + w_norm * 0.65
  
  el <- as_edgelist(g)
  seg_x <- numeric(n_edges * 2)
  seg_y <- numeric(n_edges * 2)
  seg_z <- numeric(n_edges * 2)
  edge_color_vec <- character(n_edges * 2)
  
  for (i in 1:n_edges) {
    v1 <- as.numeric(el[i,1])
    v2 <- as.numeric(el[i,2])
    idx <- (i-1)*2 + 1
    seg_x[idx] <- coords[v1,1]
    seg_x[idx+1] <- coords[v2,1]
    seg_y[idx] <- coords[v1,2]
    seg_y[idx+1] <- coords[v2,2]
    seg_z[idx] <- coords[v1,3]
    seg_z[idx+1] <- coords[v2,3]
    edge_color <- rgb(1, 0.667, 0, alpha = edge_alpha[i])
    edge_color_vec[idx] <- edge_color
    edge_color_vec[idx+1] <- edge_color
  }
  
  segments3d(x = seg_x, y = seg_y, z = seg_z, color = edge_color_vec, lwd = 2)
}

aspect3d(1, 1, 1)

grid3d("x", col = "#ffffff20")
grid3d("y", col = "#ffffff20")
grid3d("z", col = "#ffffff20")

title_text <- sprintf("3DAI Engine - %d Nodes | %d Edges", n_nodes, n_edges)
title3d(main = title_text, xlab = "X", ylab = "Y", zlab = "Z", color = "#ffffff", cex = 1.2)

view3d(theta = 30, phi = 20, fov = 60)

cat("[R] Rendering 3D scene...\n")
rgl.snapshot("output/network_3d.png", fmt = "png")
cat("[R] Snapshot saved: output/network_3d.png\n")

cat("[R] Interactive 3D window open. Rotate/zoom with mouse. Press Ctrl+C to exit.\n")
tryCatch({
  Sys.sleep(Inf)
}, interrupt = function(e) {
  cat("[R] Exiting renderer.\n")
})
