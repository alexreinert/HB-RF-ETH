import shutil
from os import SEEK_CUR, SEEK_END
from os.path import basename, isfile, join
import gzip

from SCons.Script import Builder

Import("env")

with open("version.txt") as fp:
    version = fp.readline()
    env.Replace(PROGNAME="firmware_%s" % version.replace(".", "_"))

board = env.BoardConfig()

files = []
if "build.embed_files" in board:
    files.extend(
        [
            join("$PROJECT_DIR", f)
            for f in board.get("build.embed_files", "").split()
            if f
        ]
    )

for f in files:
    if (f.endswith(".gzip")):
        with open(env.subst(f)[:-5], "rb") as fp:
            data = fp.read()
            bindata = bytearray(data)
            with gzip.open(env.subst(f), "wb") as gp:
                gp.write(bindata)
