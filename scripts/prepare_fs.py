Import("env")
import os
import shutil

env_name = env["PIOENV"]
src_dir = "data_tag" if "tag" in env_name else "data_anchor"
dest_dir = "data"

if os.path.isdir(dest_dir):
    shutil.rmtree(dest_dir)

shutil.copytree(src_dir, dest_dir)
print(f"Filesystem source: {src_dir} -> {dest_dir}")
