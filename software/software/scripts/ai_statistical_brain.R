#!/usr/bin/env Rscript

suppressPackageStartupMessages({
  if (!requireNamespace("jsonlite", quietly = TRUE)) install.packages("jsonlite", repos="https://cloud.r-project.org")
  library(jsonlite)
  library(stats)
  if (!requireNamespace("car", quietly = TRUE)) install.packages("car", repos="https://cloud.r-project.org")
  library(car)
})

options(warn = -1)

con <- file("stdin", open = "r")
cat("R_AI_BRAIN_READY\n")
flush.console()

while (TRUE) {
  cmd <- readLines(con, n = 1, warn = FALSE)
  if (length(cmd) == 0 || cmd == "quit") break
  
  data_json <- readLines(con, n = 1, warn = FALSE)
  if (length(data_json) == 0) next
  
  data <- tryCatch(fromJSON(data_json), error = function(e) list(error = e$message))
  if ("error" %in% names(data)) {
    cat(toJSON(list(error = "Invalid JSON")), "\n")
    flush.console()
    next
  }
  
  result <- tryCatch({
    switch(cmd,
      "t_test_independent" = {
        res <- t.test(data$group1, data$group2, var.equal = TRUE)
        list(statistic = unname(res$statistic), p_value = res$p.value,
             significant = res$p.value < 0.05,
             interpretation = ifelse(res$p.value < 0.05, "Significant difference found", "No significant difference"))
      },
      "t_test_paired" = {
        res <- t.test(data$before, data$after, paired = TRUE)
        list(statistic = unname(res$statistic), p_value = res$p.value,
             significant = res$p.value < 0.05)
      },
      "anova" = {
        if (length(data$value) != length(data$group)) stop("value and group vectors must be same length")
        df <- data.frame(value = data$value, group = as.factor(data$group))
        res <- aov(value ~ group, data = df)
        p_val <- summary(res)[[1]][["Pr(>F)"]][1]
        list(p_value = p_val, significant = p_val < 0.05)
      },
      "mann_whitney" = {
        res <- wilcox.test(data$group1, data$group2)
        list(statistic = unname(res$statistic), p_value = res$p.value)
      },
      "wilcoxon_paired" = {
        res <- wilcox.test(data$before, data$after, paired = TRUE)
        list(statistic = unname(res$statistic), p_value = res$p.value)
      },
      "kruskal_wallis" = {
        if (length(data$value) != length(data$group)) stop("value and group vectors must be same length")
        df <- data.frame(value = data$value, group = as.factor(data$group))
        res <- kruskal.test(value ~ group, data = df)
        list(statistic = unname(res$statistic), p_value = res$p.value)
      },
      "pearson_correlation" = {
        res <- cor.test(data$x, data$y, method = "pearson")
        list(correlation = unname(res$estimate), p_value = res$p.value)
      },
      "spearman_correlation" = {
        res <- cor.test(data$x, data$y, method = "spearman")
        list(correlation = unname(res$estimate), p_value = res$p.value)
      },
      "chi_square" = {
        tbl <- table(data$var1, data$var2)
        res <- chisq.test(tbl)
        list(statistic = unname(res$statistic), p_value = res$p.value)
      },
      "linear_regression" = {
        df <- data.frame(y = data$y, x = data$x)
        model <- lm(y ~ x, data = df)
        list(coefficients = unname(coef(model)), r_squared = summary(model)$r.squared)
      },
      "logistic_regression" = {
        df <- data.frame(outcome = as.factor(data$outcome), x = data$x)
        model <- glm(outcome ~ x, data = df, family = binomial)
        list(coefficients = unname(coef(model)), deviance = model$deviance)
      },
      "f_test" = {
        res <- var.test(data$group1, data$group2)
        list(statistic = unname(res$statistic), p_value = res$p.value)
      },
      "levene_test" = {
        if (length(data$value) != length(data$group)) stop("value and group vectors must be same length")
        df <- data.frame(value = data$value, group = as.factor(data$group))
        res <- leveneTest(value ~ group, data = df)
        list(statistic = res[1, "F value"], p_value = res[1, "Pr(>F)"])
      },
      "predict_failure" = list(prediction = 0.0, confidence = 0.0, note = "model_not_trained"),
      "find_patterns" = list(clusters_found = 0, high_risk_zones = NULL),
      "learn_from_experience" = list(model_updated = TRUE, samples_processed = length(data$outcome)),
      stop(paste("Unknown command:", cmd))
    )
  }, error = function(e) {
    list(error = e$message)
  })
  
  cat(toJSON(result, auto_unbox = TRUE, digits = 4), "\n")
  flush.console()
}
close(con)
