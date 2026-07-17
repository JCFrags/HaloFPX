.PHONY: build validate test serve all

build:
	python tools/build_site.py

validate:
	python tools/validate_wiki.py

test:
	python -m pytest -q

serve:
	python tools/serve.py

all: build validate test
