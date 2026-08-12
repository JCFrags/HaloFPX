SHELL := /usr/bin/env bash
ROOT := $(CURDIR)

.PHONY: validate render package clean-site

validate: render
	python3 scripts/validate-data.py
	python3 -m py_compile scripts/*.py
	@for f in scripts/*.sh; do bash -n "$$f"; done
	python3 scripts/check-local-links.py

render:
	python3 scripts/render-wiki.py

package: validate
	python3 scripts/build-release.py

clean-site:
	rm -rf site/*
