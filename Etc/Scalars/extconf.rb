require 'mkmf'

#only for option value passed into command line
#dir_config('engine')

#if have_header('datasorter.h') and find_library('scalars',  'strdictcmp', "#{Dir.pwd}/engine/lib")
if find_library('scalars',  '_Z10strdictcmpPKcS0_', "#{Dir.pwd}/engine/lib")

  # If you use swig -c option, you may have to link libswigrb.
  # have_library('swigrb')
  $libs = append_library($libs, "stdc++")
  create_makefile('datasorter')
end
