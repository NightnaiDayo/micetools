import sys
import os
from mesonbuild import mesonmain
from mesonbuild.linkers.linkers import StaticLinker, VisualStudioLikeLinker, VisualStudioLinker

# https://bugs.python.org/issue36112
os.path.realpath = lambda path: path

sys.exit(mesonmain.main())
