"""Download Unicode character data for host tools.

The kernel and the bootloader do not run Python. Call this only on the
development machine when a tool needs the Unicode character database.
"""

import pathlib
import urllib.request

UNICODE_DATA_URL = "https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt"


def Destination():
    return pathlib.Path(__file__).resolve().parent / "Build" / "Unicode" / "UnicodeData.txt"


def Main():
    destination = Destination()
    destination.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(UNICODE_DATA_URL, destination)
    print(destination)


if __name__ == "__main__":
    Main()
