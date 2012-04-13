#!/usr/bin/env Rscript
args = commandArgs(trailingOnly = TRUE)
if(length(args) < 2){
  cat("must provide an input file and output pdf\n")
  quit()
}

if(length(args) >= 3){
  name = args[3]
} else {
  name = ""
}

a = read.table(args[1], header=T)

# remove NAs
sel = which(is.na(a$pkg_J))
if(length(sel) > 0){
  a = a[-sel,]
}
sel = which(is.na(a$pp0_J))
if(length(sel) > 0){
  a = a[-sel,]
}

# remove outliers due to overflow
sel = union(which(a$pkg_J > 2.5*median(a$pkg_J)), which(a$pp0_J > 2.5*median(a$pp0_J)))
if(length(sel) > 0){
  a = a[-sel,]
}

l = length(a[,1])
b = a$timestamp[2:l] - a$timestamp[1:(l-1)]
t = a$timestamp[2:l] - a$timestamp[1]
j = a$pkg_J[2:l]
p = j / b

meanSamplePeriod = mean(b)
#print(meanSamplePeriod)

table = cbind(t, j, b, p)
o = order(p, decreasing=T)
#print.table(table[o[1:50],])

# @todo this is wrong?
pkgAvgPower = sum(a$pkg_J)/(a$timestamp[l] - a$timestamp[1])

pdf(args[2])
plot(t, p, pch='.', xlab='time (s)', ylab='power (w)', main=paste(name, 'power vs time'), sub=paste('mean pkg power:', format(pkgAvgPower, digits=3), 'w, mtbs:', format(meanSamplePeriod, digits=2, scientific=T)))
dev.off()

#pdf(paste('hist', args[2], sep=''))
#h = hist(p, breaks=1000, xlab='watts', freq=F, main=paste(name, 'histogram of power samples'))
#smoothed = filter(h$counts, rep(1, 10))
#barplot(smoothed[which(!is.na(smoothed))], ylab='density', xlab='watts')
#dev.off()
