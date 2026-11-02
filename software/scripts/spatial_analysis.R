#!/usr/bin/env Rscript

if (!requireNamespace("igraph", quietly = TRUE)) {
  install.packages("igraph", repos = "https://cloud.r-project.org", quiet = TRUE)
}
if (!requireNamespace("plotly", quietly = TRUE)) {
  install.packages("plotly", repos = "https://cloud.r-project.org", quiet = TRUE)
}
if (!requireNamespace("dbscan", quietly = TRUE)) {
  install.packages("dbscan", repos = "https://cloud.r-project.org", quiet = TRUE)
}

suppressPackageStartupMessages({
  library(igraph)
  library(plotly)
  library(dbscan)
})

cat("[R-SPATIAL] Loading data...\n")
metadata <- read.csv("output/metadata.csv")
edges <- jsonlite::fromJSON("output/edges.json")

n_nodes <- nrow(metadata)
cat(sprintf("[R-SPATIAL] Loaded %d nodes, %d edges\n", n_nodes, nrow(edges)))

coords <- as.matrix(metadata[, c("x", "y", "z")])

cat("[R-SPATIAL] Computing nearest-neighbor distances...\n")
nn_dists <- numeric(n_nodes)
for (i in 1:n_nodes) {
  dists <- sqrt(rowSums((coords - rep(coords[i,], each=n_nodes))^2))
  dists[i] <- Inf
  nn_dists[i] <- min(dists)
}
mean_nn <- mean(nn_dists)

cat("[R-SPATIAL] Computing DBSCAN clusters...\n")
db_result <- dbscan(coords, eps = 1.0, minPts = 3)
cluster_ids <- db_result$cluster
n_clusters <- length(unique(cluster_ids[cluster_ids > 0]))

cat("[R-SPATIAL] Computing shortest paths...\n")
g <- graph.empty(n = n_nodes, directed = FALSE)
if (nrow(edges) > 0) {
  edge_list <- cbind(edges$src + 1, edges$dst + 1)
  g <- add_edges(g, t(edge_list))
  E(g)$weight <- edges$dist
}

path_lengths <- numeric(0)
if (ecount(g) > 0 && vcount(g) > 1) {
  for (i in 1:min(10, n_nodes)) {
    for (j in (i+1):min(10, n_nodes)) {
      if (i != j) {
        sp <- shortest_paths(g, from = i, to = j, weights = E(g)$weight)
        if (length(sp$vpath[[1]]) > 0) {
          path_lengths <- c(path_lengths, length(sp$vpath[[1]]) - 1)
        }
      }
    }
  }
}
mean_path <- if(length(path_lengths) > 0) mean(path_lengths) else 0

stats <- data.frame(
  metric = c("mean_nn_dist", "n_clusters", "mean_path_length"),
  value = c(mean_nn, n_clusters, mean_path)
)
write.csv(stats, "output/spatial_stats.csv", row.names = FALSE)
cat("[R-SPATIAL] Saved: output/spatial_stats.csv\n")

cat("[R-SPATIAL] Creating cluster plot...\n")
plot_data <- data.frame(
  x = coords[,1],
  y = coords[,2],
  z = coords[,3],
  cluster = as.factor(cluster_ids)
)

p <- plot_ly(plot_data, x = ~x, y = ~y, z = ~z, color = ~cluster,
             colors = "Set1", type = "scatter3d", mode = "markers",
             marker = list(size = 5),
             text = ~paste("ID:", 0:(n_nodes-1), "<br>Degree:", metadata$degree))

htmlwidgets::saveWidget(p, "output/cluster_plot.html", selfcontained = TRUE)
cat("[R-SPATIAL] Saved: output/cluster_plot.html\n")

cat("[R-SPATIAL] Analysis complete.\n")
