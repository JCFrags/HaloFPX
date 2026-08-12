.PHONY: lint deep-validate package

lint:
	python3 scripts/lint-wiki.py

deep-validate:
	python3 scripts/deep-validate.py

package: lint
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
	find . -type f -name '*.pyc' -delete
	cd .. && zip -r HaloKV-LLM-Wiki.zip HaloKV-LLM-Wiki -x '*/.DS_Store' '*/__pycache__/*' '*.pyc'
