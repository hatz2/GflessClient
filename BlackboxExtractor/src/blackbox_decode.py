"""Pure blackbox decoding, kept free of Frida so it can be unit-tested."""

import base64
import hashlib

BLACKBOX_PREFIX = "tra:"


def xor_blackbox(data: bytes, key: bytes) -> bytes:
    """XOR every byte with the key byte at its position and the mirrored key byte.

    The operation is its own inverse, so the same call encodes and decodes.
    """
    if not key:
        raise ValueError("Key cannot be empty")

    result = bytearray()
    key_len = len(key)
    for i, b in enumerate(data):
        key_index = i % key_len
        result.append(b ^ key[key_index] ^ key[key_len - key_index - 1])
    return bytes(result)


def blackbox_key(gsid: str, platform_game_account_id: str) -> bytes:
    """The XOR key of a thin/codes request: hex sha512 of '<gsid>-<platformGameAccountId>'."""
    seed = f"{gsid}-{platform_game_account_id}".encode()
    return hashlib.sha512(seed).hexdigest().encode()


def decode_request(request: dict) -> tuple[str | None, str | None]:
    """Return (blackbox, error) for the JSON body of an auth request.

    The client login (auth/sessions) and auth/iovation carry the blackbox in the
    clear ('tra:...'); auth/thin/codes carries it base64-encoded and XORed with
    a key derived from gsid and platformGameAccountId.
    """
    fields = ", ".join(sorted(str(name) for name in request))
    blackbox = request.get("blackbox")
    if not isinstance(blackbox, str) or not blackbox:
        return None, f"the request has no 'blackbox' field; fields: {fields}"

    if blackbox.startswith(BLACKBOX_PREFIX):
        return blackbox, None

    gsid = request.get("gsid")
    account_id = request.get("platformGameAccountId")
    if not gsid or not account_id:
        return None, f"cannot decode the blackbox: missing gsid/platformGameAccountId; fields: {fields}"

    try:
        raw = base64.b64decode(blackbox.encode())
        decoded = xor_blackbox(raw, blackbox_key(str(gsid), str(account_id))).decode("utf-8")
    except Exception as e:  # binascii.Error, UnicodeDecodeError, ...
        return None, f"could not decode the blackbox ({type(e).__name__}: {e})"

    if not decoded.startswith(BLACKBOX_PREFIX):
        return None, "decoded value does not start with 'tra:'"
    return decoded, None
