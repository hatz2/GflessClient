"""Unit tests of the blackbox decoding (no Frida or game client needed).

Run from the BlackboxExtractor folder:
    .venv\\Scripts\\python.exe -m unittest discover -s tests -p "test_*.py"
"""

import base64
import hashlib
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src"))

from blackbox_decode import blackbox_key, decode_request, xor_blackbox  # noqa: E402

GSID = "11111111-2222-3333-4444-555555555555-1234"
ACCOUNT_ID = "66666666-7777-8888-9999-000000000000"
PLAIN = "tra:" + "A1b2C3d4_-" * 20


def encode(plain: str, gsid: str = GSID, account_id: str = ACCOUNT_ID) -> str:
    """What the client puts in a thin/codes request."""
    return base64.b64encode(xor_blackbox(plain.encode(), blackbox_key(gsid, account_id))).decode()


class XorBlackboxTest(unittest.TestCase):
    def test_round_trip_with_a_known_key(self):
        key = b"0123456789abcdef"
        data = PLAIN.encode()
        scrambled = xor_blackbox(data, key)
        self.assertNotEqual(scrambled, data)
        self.assertEqual(xor_blackbox(scrambled, key), data)

    def test_known_value(self):
        # b ^ key[i] ^ key[len - i - 1] with key "ab": 'x' ^ 'a' ^ 'b', 'y' ^ 'b' ^ 'a'
        self.assertEqual(xor_blackbox(b"xy", b"ab"), bytes([ord("x") ^ 3, ord("y") ^ 3]))

    def test_empty_key_is_rejected(self):
        with self.assertRaises(ValueError):
            xor_blackbox(b"x", b"")

    def test_key_is_the_hex_sha512_of_gsid_and_account(self):
        key = blackbox_key("a", "b")
        self.assertEqual(len(key), 128)
        self.assertEqual(key, hashlib.sha512(b"a-b").hexdigest().encode())


class DecodeRequestTest(unittest.TestCase):
    def test_thin_codes_blackbox_is_decoded(self):
        request = {"blackbox": encode(PLAIN), "gsid": GSID, "platformGameAccountId": ACCOUNT_ID}
        self.assertEqual(decode_request(request), (PLAIN, None))

    def test_plain_tra_value_passes_through(self):
        # The client login and auth/iovation carry the blackbox in the clear, without gsid.
        request = {"identity": "user@example.com", "locale": "en_GB", "blackbox": PLAIN}
        self.assertEqual(decode_request(request), (PLAIN, None))

    def test_missing_field(self):
        blackbox, error = decode_request({"gsid": GSID, "platformGameAccountId": ACCOUNT_ID})
        self.assertIsNone(blackbox)
        self.assertIn("has no 'blackbox' field", error)
        self.assertIn("gsid, platformGameAccountId", error)

    def test_empty_or_non_string_field(self):
        for value in ("", None, 42):
            blackbox, error = decode_request({"blackbox": value})
            self.assertIsNone(blackbox)
            self.assertIn("has no 'blackbox' field", error)

    def test_missing_gsid(self):
        blackbox, error = decode_request({"blackbox": encode(PLAIN), "platformGameAccountId": ACCOUNT_ID})
        self.assertIsNone(blackbox)
        self.assertIn("missing gsid/platformGameAccountId", error)

    def test_missing_account_id(self):
        blackbox, error = decode_request({"blackbox": encode(PLAIN), "gsid": GSID})
        self.assertIsNone(blackbox)
        self.assertIn("missing gsid/platformGameAccountId", error)

    def test_bad_base64(self):
        request = {"blackbox": "abc", "gsid": GSID, "platformGameAccountId": ACCOUNT_ID}
        blackbox, error = decode_request(request)
        self.assertIsNone(blackbox)
        self.assertTrue(error.startswith("could not decode the blackbox (Error: "), error)

    def test_wrong_key_does_not_decode(self):
        request = {"blackbox": encode(PLAIN), "gsid": "another-gsid", "platformGameAccountId": ACCOUNT_ID}
        blackbox, error = decode_request(request)
        self.assertIsNone(blackbox)
        self.assertIsNotNone(error)

    def test_decoded_value_not_starting_with_tra(self):
        request = {"blackbox": encode("xyz:something"), "gsid": GSID, "platformGameAccountId": ACCOUNT_ID}
        self.assertEqual(decode_request(request), (None, "decoded value does not start with 'tra:'"))


if __name__ == "__main__":
    unittest.main()
