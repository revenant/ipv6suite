### Copyright (C) 2004 by Johnny Lai
###
### This program is free software; you can redistribute it and/or
### modify it under the terms of the GNU General Public License
### as published by the Free Software Foundation; either version 2
### of the License, or (at your option) any later version.
###
### This program is distributed in the hope that it will be useful,
### but WITHOUT ANY WARRANTY; without even the implied warranty of
### MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
### GNU General Public License for more details.
###
### You should have received a copy of the GNU General Public License
### along with this program; if not, write to the Free Software
### Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


###Levels must be how you want the actual ordering of the levels to be Note
###according to the docs for relevel works for unordered factors only
###e.g. jl.relevel(scheme, c("mip", "pcoaf","hmip", "hmip-pcoaf", "ar",
###"pcoaf-ar","hmip-ar","hmip-pcoaf-ar"))
jl.relevel <- function(ufactor, levels)
  {
    rev.levels <- rev(levels)
    factor.new <- ufactor
    for (sch in rev.levels )
      {
        factor.new <- relevel(factor.new, sch)
      }
    factor.new
  }

jl.boxplotmeanscomp <-function(out, var, data, fn="graph", tit="Handover Improvement",
                               xlab="Mobility management scheme", ylab="Handover latency (s)",
                               sub ="Comparing boxplots and non-robust mean +/- SD",
                               log = FALSE, subset=0, print=TRUE, plotMeans=TRUE, notch=FALSE)
  {
    if (!require(gregmisc) && !require(gplots))
      return

    ##all measurements in inches for R
    width<-20/2.5
    height<-18/2.5
    fontsize<-8
    paper<-"special" #eps specify size for eps box
    if (log)
      logaxis="y"
    else
      logaxis=""
    
    ##if (fn==NULL)
    ##if (length(fn)==0) #pretty silly don't know why they didn't allow fn==NULL test
    onefile=FALSE #manual recommends this for eps

    if (print)
      postscript(paste(fn, "-box.eps",sep=""), horizontal=FALSE,
                 onefile=onefile,
                 paper=paper,width=width, height=height, pointsize=fontsize+2)
###    else
###      X11()
    ##Assuming we never want to plot just one row
    if(length(subset) == 1)      
      box<-boxplot.n(out ~ var, data=data, log=logaxis,main=tit, xlab=xlab, ylab
                     =ylab,top=TRUE,notch)
    else
      {
        box<-boxplot.n(out[subset] ~ var[subset], data=data, log=logaxis,main=tit,
                       xlab=xlab, ylab=ylab, top=TRUE,notch)
        ##box<-boxplot.n(out ~ var, data=data, log=logaxis,main=tit, xlab=xlab, ylab=ylab, subset=subset, top=TRUE)
        var <-var[subset]
        out <-out[subset]
      }
    ##box<-boxplot(out ~ var, data=data, log=logaxis,main=tit, xlab=xlab, ylab=ylab)
    ##boxplot(latency~scheme, subset= ho!="4th" & scheme!="l2trig")

    mn.t<-tapply(out, var, mean)
    sd.t <-tapply(out, var,sd)
    xi <- 0.3 +seq(box$n)
    points(xi, mn.t, col = "orange", pch = 18)
    arrows(xi, mn.t - sd.t, xi, mn.t + sd.t, code = 3, col = "red", angle = 75,length = .1)
    title(sub=sub)
    rm(mn.t,sd.t,xi)

    if (plotMeans)
      {

        if (print)
          {
            dev.off()#flush output buffer/graph
            postscript(paste(fn, "-meancin.eps",sep=""), horizontal=FALSE, onefile=onefile,
                       paper=paper,width=width, height=height, pointsize=fontsize)
          }
        else
          X11()

        plotmeans(out ~ var, data, n.label=FALSE, ci.label=TRUE, mean.label=TRUE,connect=FALSE,use.t=FALSE,
                  ylab=ylab,xlab=xlab,main=tit)
        title(sub="Sample mean and confidence intervals")

        if (print)
          dev.off()
      }

    box
  }

###Assumes that 0 is the omnetpp vector number for pingDelay
scanRoundTripTime <- function(filename, scheme)
{
  rttscan <- scan(pipe(paste("grep ^0", filename)),list(scheme=0,time=0,rtt=0))
  attach(rttscan)
  rtt <- data.frame(scheme=scheme, time=time, rtt=rtt)
  detach(rttscan)
  rtt
}

simulateTimes <-  function()
  {
    time <- 0
    ping.begin <- 7
    sim.end <- 250
    ping.dur <- sim.end - ping.begin
    ping.period <- 0.05
    ##R starts at 1 as 0 is class or type of vector
    index <- 1
    fastbeacon.min <- 0.05
    fastbeacon.max <- 0.07
    ping.times <- 1:(ping.dur*(1/ping.period))
    ##Give it a longer length just to make it happy
    value <- vector(mode="numeric",length(ping.times))
    time <- ping.begin
    while (time < ping.dur)
      {
        value[index] <- runif(1, fastbeacon.min, fastbeacon.max)
        time <-time + value[index]
        value[index] <-  time
        index <- index + 1
      }
    
    #Eliminate 0 values
    value <- value[!is.element(value, 0)]
    ping.times <- ping.times[1:length(value)]
    #cor(ping.times, value)
    ##ret <- data.frame(time=value)
    ##values get deleted once return from function
    ##    rm(index, ping.period, sim.end, ping.times, time, ping.dur,value,fastbeacon.min, fastbeacon.max)
#Don't know how to return a clist
    ##ret <- vector(length=2)
    ##ret[1]=value
    ##ret[2]=ping.times
    #clist(value, ping.times)
    list(value, ping.times)
    
  }

###In emacs use C-M-h to quickly highlight a function and type manuall M-x
###ess-eval-region.  After that we can use C-r and type eval to look up history
###of complex commands typed in and run straight away.

jl.ci <- function(x,y, p=0.95,use.t=TRUE, rowLabels=levels(y),
                  columnLabels=c("n", "Mean", "Lower CI limit",
                    "Upper CI limit"), unit=c("",rep("(s)",length(columnLabels)-1)))
  {

   #In Mthesis I used +-interval  2*interval=width
   #hence no 4 term in numerator of Eq. 5.1 as it would be cancelled out in
   #denominator since I did /interval^2 instead of width (4*estvar*qnorm(1-alpha/2)^2)/width^2

   means <- tapply(x,y, mean)
   ns <- tapply(x,y,length)
   vars <- tapply(x,y,var)
   if (use.t)
     ciw <- qt((1+p)/2,ns-1) * sqrt(vars)/sqrt(ns)
   else ciw <- qnorm((1+p)/2)*sqrt(vars/(ns))
   ci.lower <- means - ciw
   ci.upper <- means + ciw

   ##c is concatentate components of each arg so length of result is length of
   ##indiivdual cmps
   ##list is contatene components and length is number arguments concatenated
   hugelist <- c(ns,means,ci.lower,ci.upper)
   table <- matrix(hugelist,length(ciw),length(hugelist)/length(ciw))
   columnLabels <- paste(columnLabels, unit)
   dimnames(table) <- list(rowLabels, columnLabels)
   table
 }

jl.sem <- function(x,y=NULL)
  {
    if (length(y) == 0)
      {
        means <- mean(x)
        ns <- length(x)
        vars <- var(x)
      } else
      {
        means <- tapply(x,y, mean)
        ns <- tapply(x,y,length)
        vars <- tapply(x,y,var)
      }
   #standard error of the median (1.25 approximates sqrt(pi/2))
   #semedian <- 1.25*sem
   sqrt(vars/ns)
  }

jl.sed <- function(sem1,sem2)
  {
   #standard error of the difference in two means from same population diff samples
    sqrt(sem1^2 + sem2^2)    
  }

jl.cis <- function(x,p=0.95,use.t=TRUE, rowLabel="x", columnLabels=c("n", "Mean", "Lower CI limit",
                                                        "Upper CI limit"), unit="", citest = FALSE)
  {  
   means <- mean(x)
   ns <- length(x)
   vars <- var(x)
   sem <- sqrt(vars/ns) 
   if (use.t)
     ciw <- qt((1+p)/2,ns-1) * sem
   else ciw <- qnorm((1+p)/2)* sem
   if (citest) {
     return(ciw)
   }   
   ci.lower <- means - ciw
   ci.upper <- means + ciw
   hugelist <- c(ns,means,ci.lower,ci.upper)
   return(hugelist)
   if (FALSE) {
     table <- matrix(hugelist,length(ciw),length(hugelist)/length(ciw))
     dimnames(table) <- list(rowLabel, columnLabels)
     table
   }
  }

#group sample sizes in n, means in u and stddevs in s
#Or if n is a dataframe then these columns have to exist
#mean stddev and samples
jl.groupci <- function(n, u = NULL, s =NULL, p=0.95)
  {
    if (length(u) != 0)
      x <- data.frame(mean=u,samples=n, stddev=s)    
    else x <- n
      
    v <- x$stddev^2
    nfactor <- (x$samples-1)/x$samples
    sstar <- sqrt(nfactor*v)

    ntot <- sum(x$samples)
    utot = sum((x$samples * x$mean)/ntot)
    v <- sstar^2
    u2 = x$mean^2
    vtot = sum((x$samples*(v+u2)))/ntot-utot^2
    semn = sqrt(vtot/ntot)
    qnorm((1+p)/2) * sqrt(ntot) * semn/sqrt(ntot-1)
  }

#run 'fun' on subsets determined from 'by' on dataframe 'x'.
#Good for cases when tapply does not work as it expects a simple 'x' vector
#Rest is for prettying up output and checking whether rest
#tested with jl.groupci where it requires more than a single subsetted vector
jl.tapply <- function(x, by, fun = jl.groupci, ...)
  {
    if (!is.data.frame(x))
      stop("'x' must be a frame so all columns are equal length")
    nI <- length(by)
    extent = integer(nI)
    namelist <- vector("list", nI)
    names(namelist) <- names(by)
    for (i in seq(by)) {
        index <- as.factor(by[[i]])
        namelist[[i]] <- levels(index)
        extent[i] <- nlevels(index)
        if (length(index) != length(x[,1]))
            stop("arguments must have same length")
      }

    ans = lapply(split(x, by), fun, ...)
    if (all(unlist(lapply(ans, length)) == 1)) {
        ansmat <- array(dim = extent, dimnames = namelist)
        ans <- unlist(ans, recursive = FALSE)
        return(matrix(ans, nrow = nrow(ansmat), ncol=ncol(ansmat),
                      dimnames=dimnames(ansmat)))
        
    }
    return(ans)
  }
              
##Gives back the value of true for odd numbers 
jl.odd <- function(x)
  {
    if (length(x) == 1)
      {
        if (x %% 2 > 0)
          return(TRUE)
        else
          return(FALSE)
      }
    ##For each element
    for (i in x) {
      if (!jl.odd(i))
        return(FALSE)
    }
    TRUE
  }

##Change the dataframe$factor use new level labels
##dataframe is a single dataframe or list of them.
##factor is a factor column name in dataframe
##levels is the new level names. No checking is done on arguments
##Returns altered dataframe
jl.changeLevelName <- function(dataframe, factor, levels)
  {
    levels(dataframe[,factor]) <- levels
    return(dataframe)
  }

smoothedJitter<-function(v)
{
  index = 1
  o = c()
for (i in v)
  {
    if (index == 1)
      {
        o[index] = (1/16)*v[index]
      }
    else
      {
        o[index] = o[index-1] + (1/16)*(v[index] - o[index-1])
      }
    index = index+1
  }
  return (o)
}

jl.test <- function()
  {
    testl=seq(0,18,2)
    testv=c(0.05,0.1,seq(0.15,0.5,0.05))
    jl.Rfactor(testv, Ppl=testl)
  }

 # Ta is sender to listener delay
 # Defaults values of Ie and Bpl are for G.728 encoding and come from
 # PacketCable data sheet

 #Ppl packet loss rate (0-20)%, burstR = 1 for random otherwise > 1 BurstR =
 #length of observed burst loss/ mean length of losses under random conditions
jl.Rfactor <- function(Ta, Ie = 7, Bpl = 17, Ppl = 0, burstR = 1)
  {
    Idd <- function(Ta)
      {
        Ta <- Ta * 1000 #in milliseconds
        X <- function(Ta)
          {        
            log10(Ta/100)/log10(2)
          }
        if (length(Ta) == 1)
          {
            if (Ta <= 100)
              return (0)
            else
              {
                return(25*((1 + X(Ta)^6)^(1/6)-3*(1+(X(Ta)/3)^6)^(1/6) + 2))
              }
          }
        else
          {
            Ta[Ta<=100] = 0            
            Ta[Ta>100] = 25*((1 + X(Ta[Ta>100])^6)^(1/6)-3*(1+(X(Ta[Ta>100])/3)^6)^(1/6) + 2)
            return(Ta)
          }
      }

    Ieff <- function(Ie, Ppl, Bpl)
      {
        Ie + (95 - Ie)*(Ppl/(Ppl/burstR + Bpl))
      }
    #assuming echo cancelation is perfect so Id reduces to Idd only
    93.2 - Idd(Ta) - Ieff(Ie, Ppl, Bpl)
  }

#MOS conversational situation
jl.Mos <- function(R)
  {
    if (R < 0)
      return(1)
    if (R > 100)
      return(4.5)
    return(1+0.035*R + R*(R-60)*(100-R)*7*10^-6)
  }

jl.renameColumn <- function(frame, renameIndex = length(frame[1,]), newname = "loss")
  {
    dimnames(frame)[[2]][renameIndex] = newname
    dumbWay = F
    if (dumbWay) {
    #The column to be renamed is the response column
      response = renameIndex
    #responseIndex = dim(frame)[2]
    renamecolumn = frame[, response]
    frame = transform(frame, newname = renamecolumn)
    frame = frame[,-response]
    #can't rename it this way
    #dimnames(frame[1,])[[2]][length(frame[1,])] = "loss"
    }
    frame
  }

  codecs.read <- function()
  {
    codecs = read.csv("~/src/IPv6SuiteWithINET/Etc/scripts/codecs.csv")
    pps = c()
    for(index in 1:(length(codecs)+1))
    {
      pps[index] <- 1/(codecs[index,]$Tp*10^-3)
    }
    codecs=cbind(codecs,pps)
    #  print(codec)
    codecs
  }

jl.IeffHodelay <- function(hodelay = seq(0.1,3,0.05), lamda = seq(1/(120*2), 1/60,1/(120*2)), cindex=1,graph="Ieff")
{
  codecs=codecs.read()

   BurstR <- function(hodelay, lamda, cindex=1)
    {
      cod <- codecs[cindex,]
      Ie <- cod$Ie
      Bpl <- cod$Bpl
      R <- cod$pps
#      print(R)
      return(R*hodelay*(1-hodelay*lamda))
    }


  Ppl <- function(hodelay, lamda)
    {
      100 * hodelay * lamda
    }

  Ieff <- function(hodelay, lamda = 1/60, cindex=1)
    {

      cod <- codecs[cindex,]
      Ie <- cod$Ie
      Bpl <- cod$Bpl
      R <- cod$pps

                                        #0 <= lamda < 1 (handovers/sec)
                                        #hodelay < 1 sec best although possible for more
      burstR = BurstR(hodelay, lamda, cindex)
#      if (min(burstR) < 0)
#        {
                                        #c(index,value) at which this occurs
#        return(c(which(burstR<0),burstR[which(burstR < 0 )]))
#      }
      names(hodelay) <- hodelay
      names(lamda) <- lamda
      ppl = Ppl(hodelay, lamda)
                                        #0 <= Ppl  <= 100
      if (min(ppl) < 0)
        return(which(ppl < 0))
                                        #  if (max(Ppl) > 2)
                                        #    return(which(Ppl > 2))
      return(Ie + (95 - Ie)*(ppl/(ppl/burstR + Bpl)))
       #make sure this is equivalent as may need when we actually determine these rates online
      # Ie+(95 - Ie)*(p/((p+q)*(p+Bpl)))
    }

  
  names(hodelay) <- hodelay
  names(lamda) <- lamda
  if (graph=="BurstR")
    {
      BurstR <- outer(hodelay, lamda, BurstR, cindex=cindex)
      persp(hodelay, lamda, BurstR, theta=30, phi=20,
            ticktype = "detailed", nticks=8)
      return(BurstR)
    } 
  if (graph=="ppl")
    {
      Ppl <- outer(hodelay, lamda, Ppl)
      persp(hodelay, lamda, Ppl, theta=30, phi=20,
            ticktype = "detailed", nticks=8)
      Ppl
    }
  else
    {
      call_length = 3*60 #3 minutes
      handovers_per_call = lamda*call_length
#If I put that in directly to calculate Ieff will give wrong answer as
#increasing lamda a by factor of a lot
#        Ieff <- outer(hodelay, handovers_per_call, Ieff)
      ieff <- outer(hodelay, lamda, Ieff, cindex=cindex)
#This is a valid transformation of the axis
      if (graph=="Ieff")
        {
        persp(hodelay, handovers_per_call, ieff,theta=30, phi=20,
              zlim=range(0:30),
              r=50,#expand=0.5,
              col = "lightgreen", ltheta=90, lphi=180, shade=0.75,
              ticktype = "detailed", nticks=5)
      }
      else
        {
          #cbind(Ieff,hodelay, handovers_per_call)
          jl.matrix.as.data.frame(ieff,"Ieff", "hodelay","lamda")
      }
    }
}

jl.matrix.as.data.frame <- function(m,valuename, rowname, columname)
{
  d=dim(m)
  nrow=d[1]
  ncol=d[2]
  names=dimnames(m)
  rows=rep(names[[1]],length(m)/nrow)
  rows = as.numeric(rows)
  cols=as.numeric(names[[2]])
  colsf = c()
  for(col in cols)
    colsf = c(colsf,rep(col,nrow))
  cols=as.vector(t( colsf))
  df=data.frame(valuename=as.vector(m)) #by row
  df=cbind(df,rowname=rows)
  df=cbind(df,columnname=cols)
  dimnames(df)[[2]]=c(valuename,rowname,columname)
  df
}

if (FALSE)
  {
oldpar=par(mar=c(1,1,1,1),mfrow=c(2,3))
for(index in 1:(length(codecs)+1))
  jl.IeffHodelay(lamda = seq(1/(120*2), 1/120,1/(120*2)), hodelay = seq(0.1,2.5,0.02),cindex=index)

codecs=codecs.read()
ieff = cbind(jl.IeffHodelay(lamda = seq(1/(240*2), 1/120,1/(240*2)), hodelay = seq(0.1,2.5,0.02),cindex=1,graph=""),codec=codecs[1,]$codec)
for(index in 2:(length(codecs)+1))
ieff=rbind(ieff,
      cbind(jl.IeffHodelay(lamda = seq(1/(240*2), 1/120,1/(240*2)), hodelay = seq(0.1,2.5,0.02),cindex=index,graph=""),codec=codecs[index,]$codec))
library(lattice)
 cloud(Ieff ~ hodelay * lamda|codec, data=ieff,
       screen = list(x = -90, y = 70))

 cloud(Ieff ~ hodelay * lamda, data=ieff,groups = codec)
#legend("topleft", legend=labels(ieff$codec))

# ,      fill=seq(along=split.df), bty="n") 
}

sl.boxplot.lines <- function()
{
  #By S. Lamb
#df <- read.csv("http://www.slamb.org/tmp/one-active.csv")
df <- read.csv("one-active.csv")
#png(filename="one-active.png", width=800, height=600)
split.df <- split(df, df$method)
plot(0, xlim=c(0, max(df$inactive)), ylim=range(df$elapsed),

     ylab="time (µs)", xlab="inactive file descriptors", log="y",
     main="1 active descriptor, 1 write", bty="n", type="n")

grid()
for (i in seq(along=split.df)) {

  this_method <- split.df[[i]]
  unique_inactive <- unique(this_method$inactive)
  boxplot(elapsed~inactive, data=this_method,

            at=unique_inactive, axes=FALSE,
            add=TRUE, border=i, boxfill=i, outline=FALSE, bty="l",
            whisklty="solid", staplelty="blank", medlty="blank",
            boxwex=max(unique_inactive)/length(unique_inactive)/2)

}
legend("topleft", legend=labels(split.df),

       fill=seq(along=split.df), bty="n") 
}
