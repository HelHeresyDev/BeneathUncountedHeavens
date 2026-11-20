"""Read tensor names and shapes from a bounded prefix of a PyTorch ZIP checkpoint."""

import argparse
from collections import OrderedDict
import io
import json
from pathlib import Path
import pickle
import pickletools
import struct
from urllib.request import Request, urlopen


PREFIX_BYTES = 1024**2
STORAGE_NAMES = {"FloatStorage", "BFloat16Storage"}


class LayoutReader(pickle.Unpickler):
    def find_class(self, module, name):
        if (module, name) == ("collections", "OrderedDict"):
            return OrderedDict
        if (module, name) == ("torch._utils", "_rebuild_tensor_v2"):
            return lambda storage, _offset, shape, _stride, *_: (storage, shape)
        if module == "torch" and name in STORAGE_NAMES:
            return name
        raise pickle.UnpicklingError(f"Unsupported metadata global {module}.{name}")

    def persistent_load(self, storage):
        if storage[0] != "storage" or storage[1] not in STORAGE_NAMES:
            raise pickle.UnpicklingError("Invalid storage metadata")
        return storage[1]


def layout(prefix):
    magic, _, _, compression, *_, name_length, extra_length = struct.unpack_from("<4s5H3I2H", prefix)
    member = prefix[30:30 + name_length]
    if magic != b"PK\x03\x04" or compression or not member.endswith(b"/data.pkl"):
        raise ValueError("Expected uncompressed PyTorch metadata in the first ZIP member")
    data = prefix[30 + name_length + extra_length:]
    stop = None
    for opcode, _, offset in pickletools.genops(data):
        if opcode.name in {"EXT1", "EXT2", "EXT4", "STACK_GLOBAL", "INST", "OBJ", "NEWOBJ", "NEWOBJ_EX"}:
            raise ValueError(f"Unsupported metadata opcode {opcode.name}")
        if opcode.name == "STOP":
            stop = offset + 1
            break
    if stop is None:
        raise ValueError("The bounded prefix did not contain complete tensor metadata")
    return LayoutReader(io.BytesIO(data[:stop])).load()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    inp = parser.add_mutually_exclusive_group(required=True)
    inp.add_argument("--url")
    inp.add_argument("--file", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    if args.url:
        request = Request(args.url, headers={"Range": f"bytes=0-{PREFIX_BYTES - 1}"})
        with urlopen(request, timeout=30) as response:
            if response.status != 206 or not response.headers.get("Content-Range", "").startswith("bytes 0-"):
                raise RuntimeError("The model host did not confirm a prefix-only download")
            data = response.read(PREFIX_BYTES)
    else:
        with args.file.open("rb") as handle:
            data = handle.read(PREFIX_BYTES)
    with args.out.open("x") as handle:
        json.dump(layout(data), handle)
        handle.write("\n")


if __name__ == "__main__":
    main()
