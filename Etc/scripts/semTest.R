#Test out formula of groups of sd to work out sem

#sd func in R only has denom of N-1 for sample std dev
#but the equivalence fn for jl.sdalt is this with N denom
jl.sd <- function(set)
{
  sqrt(sum((set-mean(set))^2)/(length(set)))
}

jl.sdalt <- function(set)
{
  sqrt(sum(set^2)/length(set) - mean(set)^2)
}

jl.varCalc <- function(n, u, s)
{
  sum(n*(s*s + u*u))/ntot - utot^2
}

set1 = c(1,2,3,4,5,6,7,8,9,10,6)
set2 = c(40, 23.6, 39.6, 50.2, 23.8)
set3 = c(1, 3, 5, 7, 20, 18)
set4 = c(5, 3, 2000)
n = c(length(set1), length(set2),length(set3), length(set4))
u = c(mean(set1),mean(set2),mean(set3),mean(set4))
#will not be correct if wrong sd fn used
#s = c(jl.sd(set1),jl.sd(set2),jl.sd(set3),jl.sd(set4))
s = c(jl.sd(set1),jl.sd(set2),jl.sd(set3),jl.sd(set4))
ntot = sum(n) #length(totset)
utot = sum(n*u)/ntot
totset = c(set1,set2, set3,set4)
if (length(totset) != ntot)
  {
    print("very bad that length is wrong")
  }

#this is equiv to sum(set1^2)
#sum((jl.sd(set1)^2 + mean(set1)^2)*length(set1))

vartot = jl.varCalc(n,u,s)
sem = sqrt(vartot/ntot)
if (sem != jl.sd(totset)/sqrt(ntot))
  {
    print("sem calc does not equate to true sem")
  }
