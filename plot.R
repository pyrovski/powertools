#!/usr/bin/env Rscript
a = read.table('output', header=T)
l = length(a[,1])
b = a$timestamp[2:l] - a$timestamp[1:(l-1)]
t = a$timestamp[2:l] - a$timestamp[1]
l = length(a[,1])
p = a$pkg_J[2:l] / b
plot(t, p, pch='.')
