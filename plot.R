#!/usr/bin/env Rscript
args = commandArgs(trailingOnly = TRUE)
if(length(args) < 2){
  cat("must provide an input file and output pdf\n")
  quit()
}
a = read.table(args[1], header=T)
l = length(a[,1])
b = a$timestamp[2:l] - a$timestamp[1:(l-1)]
t = a$timestamp[2:l] - a$timestamp[1]
l = length(a[,1])
p = a$pkg_J[2:l] / b
pdf(args[2])
plot(t, p, pch='.')
dev.off()
