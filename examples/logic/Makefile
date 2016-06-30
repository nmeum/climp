M4 ?= m4
M4FLAGS ?=

all: logic.li
logic.li: logic.li.m4
	$(M4) $(M4FLAGS) $< | sed 's:#.*$$::g' \
		| sed '/./!d' > $@

../../climp: ../../
	$(MAKE) -C $<

run: ../../climp logic.li
	$< logic.li

clean:
	$(RM) logic.li

.PHONY: all run clean
