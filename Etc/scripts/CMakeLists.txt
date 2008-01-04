#Requires ${SCRIPTDIR} as scripts need their $PWD for loadpath
ADD_TEST(multiconfig ruby ${SCRIPTDIR}/multiconfig.rb --test)
ADD_TEST(ConfTest ruby ${SCRIPTDIR}/ConfTest.rb --unittest)
#Requires R>2.2.x (works with 2.3)
ADD_TEST(RImportOmnet ruby ${SCRIPTDIR}/RImportOmnet.rb --test)
ADD_TEST(importOmnet ruby ${SCRIPTDIR}/importOmnet.rb --test)
ADD_TEST(scalars ruby ${SCRIPTDIR}/../Scalars/scalars.rb --test)
#both tests fail but as not used currently leave as TODO
#needs rgl lib in ~/lib/ruby or as a gem
#ADD_TEST(NetworkGen ruby -rubygems ${SCRIPTDIR}/NedFile.rb --unittest)
