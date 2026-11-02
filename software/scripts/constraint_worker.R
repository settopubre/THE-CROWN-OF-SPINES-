#!/usr/bin/env Rscript

suppressWarnings({
  if (!requireNamespace("jsonlite", quietly = TRUE)) {
    install.packages("jsonlite", repos = "https://cloud.r-project.org", quiet = TRUE)
  }
  if (!requireNamespace("rayshader", quietly = TRUE)) {
    install.packages("rayshader", repos = "https://cloud.r-project.org", quiet = TRUE)
  }
  library(jsonlite)
  library(rayshader)
})

validate_weights <- function(weights_json) {
  weights <- fromJSON(weights_json)
  violations <- list()
  for (i in seq_along(weights)) {
    w <- weights[[i]]
    if (any(w < 0) || any(w > 1)) {
      violations[[length(violations) + 1]] <- list(index = i, issue = "weight_out_of_range")
    }
    total <- sum(w)
    if (abs(total - 1.0) > 0.01) {
      violations[[length(violations) + 1]] <- list(index = i, issue = "weights_not_normalized", sum = total)
    }
  }
  toJSON(list(valid = length(violations) == 0, violations = violations), auto_unbox = TRUE)
}

render_weightmap <- function(weights_json, output_path = "output/weightmap.png") {
  weights <- fromJSON(weights_json)
  if (length(weights) == 0) {
    cat("ok\n")
    return()
  }
  n <- length(weights[[1]])
  z_vals <- numeric(n)
  for (i in seq_len(n)) {
    z_vals[i] <- mean(sapply(weights, function(w) if (i <= length(w)) w[i] else 0))
  }
  z_mat <- matrix(z_vals, nrow = ceiling(sqrt(n)), ncol = ceiling(sqrt(n)), byrow = TRUE)
  z_mat[is.na(z_mat)] <- 0
  tryCatch({
    plot_3d(sphere_shade(z_mat, texture = "imhof1"), zscale = 10, theta = 45, phi = 30)
    render_snapshot(filename = output_path, clear = TRUE)
    cat("ok\n")
  }, error = function(e) {
    cat("error:", conditionMessage(e), "\n")
  })
}

if ("--persistent" %in% commandArgs(trailingOnly = TRUE)) {
  con <- file("stdin", open = "r")
  while (TRUE) {
    cmd <- readLines(con, n = 1, warn = FALSE)
    if (length(cmd) == 0 || cmd == "quit") break
    data <- readLines(con, n = -1, warn = FALSE)
    data <- paste(data, collapse = "\n")
    data <- sub("END\n?$", "", data)
    
    if (cmd == "validate_weights") {
      cat(validate_weights(data), "\n", sep = "")
      flush.console()
    } else if (cmd == "render_weightmap") {
      render_weightmap(data)
      flush.console()
    }
  }
  close(con)
} else {
  args <- commandArgs(trailingOnly = TRUE)
  if (length(args) >= 2) {
    cmd <- args[1]
    data <- paste(args[-1], collapse = " ")
    if (cmd == "validate_weights") {
      cat(validate_weights(data), "\n")
    } else if (cmd == "render_weightmap") {
      render_weightmap(data)
    }
  }
}
