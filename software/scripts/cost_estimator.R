#!/usr/bin/env Rscript
suppressPackageStartupMessages(library(jsonlite))

args <- commandArgs(trailingOnly = TRUE)

# Default Parameters
width_ft <- 20
length_ft <- 30
thickness_ft <- 0.5 # 6 inches concrete
gravel_thickness_ft <- 0.33 # 4 inches gravel
rebar_spacing_ft <- 1.5 # 18 inch grid spacing
formwork_height_factor <- 1.0 # Multiplier for board feet calculation

# Parse Arguments
if ("--width" %in% args) {
    width_ft <- as.numeric(args[which(args == "--width") + 1])
}
if ("--length" %in% args) {
    length_ft <- as.numeric(args[which(args == "--length") + 1])
}
if ("--thickness" %in% args) {
    thickness_ft <- as.numeric(args[which(args == "--thickness") + 1])
}

# Calculations
area_sqft <- width_ft * length_ft
perimeter_ft <- 2 * (width_ft + length_ft)

# 1. Concrete Volume (Cubic Yards)
concrete_vol_yd3 <- (area_sqft * thickness_ft) / 27.0

# 2. Gravel Volume (Cubic Yards)
gravel_vol_yd3 <- (area_sqft * gravel_thickness_ft) / 27.0

# 3. Rebar Length (Linear Feet)
# Approximation: Area / Spacing * 2 (for grid in both directions) * 1.1 waste factor
rebar_linear_ft <- (area_sqft / rebar_spacing_ft) * 2 * 1.1

# 4. Formwork Board Feet
# Linear perimeter * Height (thickness) * 1.5 factor for lumber width
formwork_bf <- perimeter_ft * thickness_ft * 1.5

# Material Prices (Hardcoded Defaults - Denver Metro Area Estimates)
prices <- list(
    concrete_per_yd3 = 155.00,
    gravel_per_yd3 = 45.00,
    rebar_per_ft = 0.65,
    formwork_per_bf = 2.20
)

# Cost Calculation
cost_concrete <- concrete_vol_yd3 * prices$concrete_per_yd3
cost_gravel <- gravel_vol_yd3 * prices$gravel_per_yd3
cost_rebar <- rebar_linear_ft * prices$rebar_per_ft
cost_formwork <- formwork_bf * prices$formwork_per_bf

total_estimated_cost <- cost_concrete + cost_gravel + cost_rebar + cost_formwork

# Output JSON
report <- list(
    project_specs = list(
        width_ft = width_ft,
        length_ft = length_ft,
        thickness_in = thickness_ft * 12
    ),
    materials = list(
        concrete = list(
            volume_yd3 = round(concrete_vol_yd3, 2),
            unit_cost = prices$concrete_per_yd3,
            total_cost = round(cost_concrete, 2)
        ),
        gravel_base = list(
            volume_yd3 = round(gravel_vol_yd3, 2),
            unit_cost = prices$gravel_per_yd3,
            total_cost = round(cost_gravel, 2)
        ),
        rebar_grid = list(
            linear_ft = round(rebar_linear_ft, 2),
            unit_cost = prices$rebar_per_ft,
            total_cost = round(cost_rebar, 2)
        ),
        formwork = list(
            board_feet = round(formwork_bf, 2),
            unit_cost = prices$formwork_per_bf,
            total_cost = round(cost_formwork, 2)
        )
    ),
    summary = list(
        total_material_cost = round(total_estimated_cost, 2),
        timestamp = format(Sys.time(), "%Y-%m-%d %H:%M:%S")
    )
)

cat(toJSON(report, pretty = TRUE, auto_unbox = TRUE), "\n")
