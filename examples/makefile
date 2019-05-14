

# # assuming zeep-test is build when install was not done already
# zeep-test: obj/zeep-test.o libzeep.a
# 	$(CXX) $(BOOST_INC_DIR:%=-I%) -o $@ -I. $^ $(LDFLAGS) -lboost_date_time -lboost_regex

# install-libs: libzeep.so
# 	install -d $(LIBDIR)
# 	install $(LIB_NAME) $(LIBDIR)/$(LIB_NAME)
# 	strip --strip-unneeded $(LIBDIR)/$(LIB_NAME)
# 	ln -Tfs $(LIB_NAME) $(LIBDIR)/$(SO_NAME)
# 	ln -Tfs $(LIB_NAME) $(LIBDIR)/libzeep.so
# 	$(LD_CONFIG) -n $(LIBDIR)

# install-dev: libzeep.a
# 	install -d $(LIBDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http $(INCDIR)/zeep/http/webapp
# 	for f in `find zeep -name "*.hpp"`; do install $$f $(INCDIR)/$$f; done
# 	install ./libzeep.a $(LIBDIR)/libzeep.a
# 	strip -SX $(LIBDIR)/libzeep.a

# install-doc: doc
# 	install -d $(MANDIR) $(DOCDIR)/html
# 	install doc/libzeep.3 $(MANDIR)/libzeep.3
# 	cd doc; for d in `find html -type d`; do install -d $(DOCDIR)/$$d; done
# 	cd doc; for f in `find html -type f`; do install $$f $(DOCDIR)/$$f; done

# install: install-libs install-dev install-doc

# dist: doc
# 	rm -rf $(DIST_NAME)
# 	mkdir -p $(DIST_NAME)
# 	git archive master | tar xC $(DIST_NAME)
# 	find doc/html -depth | cpio -pd $(DIST_NAME)
# 	tar czf $(DIST_NAME).tgz $(DIST_NAME)
# 	rm -rf $(DIST_NAME)

# doc: FORCE
# 	cd doc; bjam

# obj/%.o: %.cpp | obj
# 	$(CXX) -MD -c -o $@ $< $(CXXFLAGS)

# obj:
# 	mkdir -p obj

# -include $(OBJECTS:%.o=%.d)

# $(OBJECTS:.o=.d):

# test: libzeep.a
# 	make -C tests

# clean:
# 	rm -rf obj/* libzeep.a libzeep.so* zeep-test $(DIST_NAME) $(DIST_NAME).tgz
# 	cd doc; bjam clean
# 	rm -rf doc/bin doc/html
# 	$(MAKE) -C tests clean

# FORCE:
