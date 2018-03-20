.PHONY: tirtos-release tirtos-debug

# rules for building
all: release debug

release: tirtos-release
debug: tirtos-debug

tirtos-release:
	@ $(MAKE) -C tirtos release

tirtos-debug:
	@ $(MAKE) -C tirtos debug

# rules for cleaning
clean: clean-release clean-debug

clean-release: clean-tirtos-release

clean-debug: clean-tirtos-debug

clean-tirtos-release:
	@ $(MAKE) -C tirtos clean-release

clean-tirtos-debug:
	@ $(MAKE) -C tirtos clean-debug

# the following content will be active only if a 'freertos' subdir exists
ifeq ($(wildcard freertos), freertos)

.PHONY: freertos-release

# rules for building
release: freertos-release

freertos-release:
	@ $(MAKE) -C freertos release

# rules for cleaning
clean-release: clean-freertos-release

clean-freertos-release:
	@ $(MAKE) -C freertos clean-release

endif

# the following content will be active only if a 'nortos' subdir exists
ifeq ($(wildcard nortos), nortos)

.PHONY: nortos-release

# rules for building
release: nortos-release

nortos-release:
	@ $(MAKE) -C nortos release

# rules for cleaning
clean-release: clean-nortos-release

clean-nortos-release:
	@ $(MAKE) -C nortos clean-release

endif
