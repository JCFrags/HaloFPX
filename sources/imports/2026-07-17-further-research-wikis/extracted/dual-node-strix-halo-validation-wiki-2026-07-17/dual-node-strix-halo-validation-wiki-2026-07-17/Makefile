PYTHON ?= python3
ROOT := $(CURDIR)
SYN := examples/synthetic-non-machine

.PHONY: check schemas synthetic inventory package clean-derived

check:
	$(PYTHON) tools/check_bundle.py --report BUNDLE-CHECK-REPORT.md

schemas:
	$(PYTHON) tools/validate_records.py schemas/run-manifest.schema.json $(SYN)/manifest.json
	$(PYTHON) tools/validate_records.py schemas/request-trace.schema.json $(SYN)/requests.jsonl --jsonl
	$(PYTHON) tools/validate_records.py schemas/token-event.schema.json $(SYN)/tokens.jsonl --jsonl
	$(PYTHON) tools/validate_records.py schemas/telemetry-sample.schema.json $(SYN)/telemetry.jsonl --jsonl
	$(PYTHON) tools/validate_records.py schemas/fault-event.schema.json $(SYN)/faults.jsonl --jsonl
	$(PYTHON) tools/validate_records.py schemas/correctness-record.schema.json $(SYN)/correctness.jsonl --jsonl
	$(PYTHON) tools/validate_records.py schemas/summary.schema.json $(SYN)/summary.json
	$(PYTHON) tools/validate_records.py schemas/release-decision.schema.json $(SYN)/release-decision.json

synthetic:
	mkdir -p $(SYN)/derived
	$(PYTHON) tools/aggregate_requests.py --requests $(SYN)/requests.jsonl --tokens $(SYN)/tokens.jsonl --output $(SYN)/derived/request-summary.json
	@set +e; $(PYTHON) tools/evaluate_gates.py $(SYN)/summary.json --stage G4 --output $(SYN)/derived/gate-evaluation.json; code=$$?; set -e; \
	  test $$code -eq 1; grep -q 'INSUFFICIENT_EVIDENCE' $(SYN)/derived/gate-evaluation.json
	@set +e; $(PYTHON) tools/audit_provenance.py $(SYN)/manifest.json --run-dir $(SYN) --output $(SYN)/derived/provenance-audit.json; code=$$?; set -e; \
	  test $$code -eq 1; grep -q 'INSUFFICIENT_EVIDENCE' $(SYN)/derived/provenance-audit.json
	$(PYTHON) tools/upstream_watch.py --dry-run > $(SYN)/derived/upstream-watch-plan.json

inventory:
	$(PYTHON) tools/build_inventory.py $(ROOT)

package: check synthetic inventory
	cd .. && zip -qr "$(notdir $(ROOT)).zip" "$(notdir $(ROOT))" -x '*/__pycache__/*'

clean-derived:
	rm -rf $(SYN)/derived BUNDLE-CHECK-REPORT.md FILE-INVENTORY.csv MANIFEST.sha256
