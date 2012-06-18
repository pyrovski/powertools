#!/usr/bin/env Rscript
args <- commandArgs(trailingOnly = TRUE)
a = read.table(args[1], header=T);
t(a)
#[which(a != 0)]
