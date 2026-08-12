.PHONY: verify integrity test list clean

verify:
	python3 scripts/verify-suite.py

integrity:
	python3 scripts/verify-package-manifest.py

test:
	PYTHONPATH=harness/src python3 -m pytest harness/tests

list:
	python3 scripts/select-tests.py --include-failure-injection

clean:
	rm -rf .pytest_cache harness/.pytest_cache harness/**/__pycache__ .venv .venv-*
