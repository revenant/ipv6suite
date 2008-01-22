#Requires ${SCRIPTDIR} as scripts need their $PWD for loadpath
ADD_TEST(multiconfig ${RUBY_EXECUTABLE} ${SCRIPTDIR}/multiconfig.rb --test)
ADD_TEST(ConfTest ${RUBY_EXECUTABLE} ${SCRIPTDIR}/ConfTest.rb --unittest)
#Requires R>2.2.x (works with 2.3)
ADD_TEST(RImportOmnet ${RUBY_EXECUTABLE} ${SCRIPTDIR}/RImportOmnet.rb --test)
ADD_TEST(importOmnet ${RUBY_EXECUTABLE} ${SCRIPTDIR}/importOmnet.rb --test)
ADD_TEST(scalars ${RUBY_EXECUTABLE} ${SCRIPTDIR}/../Scalars/scalars.rb --test)
#both tests fail but as not used currently leave as TODO
#needs rgl lib in ~/lib/ruby or as a gem
#ADD_TEST(NetworkGen ruby -rubygems ${SCRIPTDIR}/NedFile.rb --unittest)
