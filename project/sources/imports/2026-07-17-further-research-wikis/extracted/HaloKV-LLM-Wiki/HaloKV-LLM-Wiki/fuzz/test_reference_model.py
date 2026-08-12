import json
import tempfile
import unittest
from pathlib import Path

from fuzz.reference_model import Model, run_trace


class ModelTests(unittest.TestCase):
    def test_partial_commit_rejected(self):
        m = Model()
        m.apply({"kind": "begin", "epoch": 0})
        m.apply({"kind": "durable", "rank": 0})
        m.apply({"kind": "prepare", "rank": 0, "epoch": 0})
        self.assertEqual(m.apply({"kind": "commit"}), "REJECT_PARTIAL")

    def test_cancel_is_terminal(self):
        m = Model()
        m.apply({"kind": "begin", "epoch": 0})
        self.assertEqual(m.apply({"kind": "cancel"}), "ABORTED")
        self.assertEqual(m.apply({"kind": "commit"}), "REJECT_TERMINAL_OR_BAD_STATE")

    def test_corruption_rejects_read(self):
        m = Model()
        m.apply({"kind": "begin", "epoch": 0})
        for rank in (0, 1):
            m.apply({"kind": "durable", "rank": rank})
            m.apply({"kind": "prepare", "rank": rank, "epoch": 0})
        self.assertEqual(m.apply({"kind": "commit"}), "COMMITTED")
        m.apply({"kind": "corrupt", "rank": 1})
        self.assertEqual(m.apply({"kind": "read"}), "REJECTED")

    def test_single_node_requires_all_predicates(self):
        m = Model()
        base = {"kind":"single_node_decision","full_weights":True,"model_fits":True,"supported_topology":True,"complete_state":True,"fenced_old_generation":True}
        self.assertEqual(m.apply(base), "PERMIT")
        base["complete_state"] = False
        self.assertEqual(m.apply(base), "REJECT")


if __name__ == "__main__":
    unittest.main()
