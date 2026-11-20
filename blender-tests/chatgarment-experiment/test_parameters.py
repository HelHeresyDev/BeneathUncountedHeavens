import unittest
from unittest.mock import patch

from generate_pattern import apply_parameters
from qwen_parameters import json_schema


class PatternParametersTests(unittest.TestCase):
    def setUp(self):
        self.design = {"shirt": {"width": {"type": "float", "range": [0.8, 1.3], "v": 1.0},
                                 "openfront": {"type": "bool", "range": [True, False], "v": False}},
                       "meta": {"upper": {"type": "select_null", "range": ["Shirt", None], "v": None}},
                       "sleeve": {"sleeve_angle": {"type": "int", "range": [10, 50], "v": 10}}}

    def test_valid_values(self):
        apply_parameters(self.design, {"meta.upper": "Shirt", "shirt.width": 1.3, "shirt.openfront": True})
        self.assertEqual(self.design["meta"]["upper"]["v"], "Shirt")
        self.assertTrue(self.design["shirt"]["openfront"]["v"])

    def test_rejects_unknown_fields_and_invalid_values(self):
        for payload in ({"shirt.width": 1.5}, {"shirt.width": True}, {"shirt.width": float("nan")},
                        {"shirt.openfront": 1}, {"sleeve.sleeve_angle": 20.5}, {"meta.upper": None},
                        {"meta.upper": "Unknown"}, {"robe.wrap": True}):
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                apply_parameters(self.design, payload)

    def test_schema_carries_ranges_and_disallows_unknown_fields(self):
        with patch("qwen_parameters.PARAMETERS", ("shirt.width", "meta.upper")):
            schema = json_schema(self.design)
        self.assertEqual(schema["properties"]["shirt.width"], {"type": "number", "minimum": 0.8, "maximum": 1.3})
        self.assertEqual(schema["properties"]["meta.upper"]["enum"], ["Shirt"])
        self.assertFalse(schema["additionalProperties"])

    def test_invalid_values_leave_design_intact(self):
        with self.assertRaises(ValueError):
            apply_parameters(self.design, {"shirt.openfront": True, "shirt.width": 2.0})
        self.assertFalse(self.design["shirt"]["openfront"]["v"])


if __name__ == "__main__":
    unittest.main()
