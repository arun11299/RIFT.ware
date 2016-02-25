# to automate this in a "grunt-like" fashion, run this 
# command in a terminal
#   watch make
COMPONENT_SRC = \
	$(shell find ridgets/css/elements -name '*.less')

BASE_SRC = \
	integration/ridgets/css/base/base.less \
	integration/ridgets/css/base/mixins.less \
	integration/ridgets/css/base/variables.less

CSS_SRC = \
	$(wildcard ridgets/*.less integration/ridgets/css/*.less tools/*.less)

CSS = $(CSS_SRC:.less=.css)

all : $(CSS);

debug:
	@echo $(CSS_SRC)

$(CSS) : %.css : %.less $(BASE_SRC) $(COMPONENT_SRC)
	lessc $< > $@.tmp
	mv $@.tmp $@
