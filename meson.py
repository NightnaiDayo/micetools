import sys
import os
from mesonbuild import mesonmain

# https://bugs.python.org/issue36112
os.path.realpath = lambda path: path

sys.exit(mesonmain.main())
