#Requires ${SCRIPTDIR} as scripts need their $PWD for loadpath
#Requires R>2.2.x (works with 2.3 and 2.6)
ADD_TEST(ConfTest ${RUBY_EXECUTABLE} ${SCRIPTDIR}/ConfTest.rb --unittest)
ADD_TEST(RImportOmnet ${RUBY_EXECUTABLE} ${SCRIPTDIR}/RImportOmnet.rb --test)
ADD_TEST(importOmnet ${RUBY_EXECUTABLE} ${SCRIPTDIR}/importOmnet.rb --test)
#Tests below can affect result of stuff in Networks/test
#ADD_TEST(multiconfig ${RUBY_EXECUTABLE} ${SCRIPTDIR}/multiconfig.rb --test)
#ADD_TEST(scalars ${RUBY_EXECUTABLE} ${SCRIPTDIR}/../Scalars/scalars.rb --test)
#both tests fail but as not used currently leave as TODO
#needs rgl lib in ~/lib/ruby or as a gem
#ADD_TEST(NetworkGen ruby -rubygems ${SCRIPTDIR}/NedFile.rb --unittest)
#cd ${TESTDIR}
#multiconfig.rb -C test_fig_script.yaml wcmc -r 200
#SmartSubmit.rb -p 0.01 jobs.txt -r 200
