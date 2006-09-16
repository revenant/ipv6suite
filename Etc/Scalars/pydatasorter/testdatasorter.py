import datasorter
import os

import os.path, fnmatch
#from recipes book
def listFiles(root, patterns='*', recurse=1, return_folders=0):

    # Expand patterns from semicolon-separated string to list
    pattern_list = patterns.split(';')
    # Collect input and output arguments into one bunch
    class Bunch:
        def __init__(self, **kwds): self.__dict__.update(kwds)
    arg = Bunch(recurse=recurse, pattern_list=pattern_list,
        return_folders=return_folders, results=[])

    def visit(arg, dirname, files):
        # Append to arg.results all relevant files (and perhaps folders)
        for name in files:
            fullname = os.path.normpath(os.path.join(dirname, name))
            if arg.return_folders or os.path.isfile(fullname):
                for pattern in arg.pattern_list:
                    if fnmatch.fnmatch(name, pattern):
                        arg.results.append(fullname)
                        break
        # Block recursion if recursion was disallowed
        if not arg.recurse: files[:]=[]

    os.path.walk(root, visit, arg)

    return arg.results

    
#main    
sm=datasorter.ScalarManager
#Fails at this bad line
ds=datasorter.DataSorter(sm)

#Dir["/home/jmll/other/IPv6SuiteWithINET/Research/Networks/EHComp/EHComp_eh_*_20*.sca"].each{|files|
folder = os.path.expanduser("~/other/IPv6SuiteWithINET/Research/Networks/EHComp/")

for f in listFiles(folder, "EHComp_eh_*_20*.sca", 0):
    print f
    files=sm.loadFile(folder + f)
    print sm.values.length
#list
    print "runs are: "
    for r in sm.runs:
        print r.fileAndRunName
#both stringsets
    print sm.modnames
    print sm.scalars

ints = ds.getFilteredScalarList("*", "*.udpApp[*]", "* % *")
print ints.length
for d in ints:
    datum=sm.getValue(i)
    print datum.modname, datum.run, datum.scalar
    #see tkcmd.cc getListboxLine_cmd for rest of display datum



