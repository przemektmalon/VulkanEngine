
# Clones dependecies from github and checks out working branches

import os
from subprocess import call
import urllib.request
import zipfile

if __name__ == "__main__":

    git_vdu = "https://github.com/przemektmalon/VulkanDevUtility.git"
    git_freetype = "https://github.com/winlibs/freetype.git"
    git_chaiscript = "https://github.com/ChaiScript/ChaiScript.git"
    git_glm = "https://github.com/g-truc/glm.git"
    git_rapidxml = "https://github.com/dwd/rapidxml.git"
    git_assimp = "https://github.com/assimp/assimp.git"
    url_stb_image = "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    url_stb_image_write = "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"

    call("git clone " + git_vdu + " ./lib/vdu --quiet", shell=True)
    call("git clone " + git_freetype + " ./lib/freetype --quiet", shell=True)
    call("git clone " + git_chaiscript + " ./lib/chaiscript --quiet", shell=True)
    call("git clone " + git_glm + " ./lib/glm --quiet", shell=True)
    call("git clone " + git_rapidxml + " ./lib/rapidxml --quiet", shell=True)
    call("git clone " + git_assimp + " ./lib/assimp --quiet", shell=True)

    os.makedirs("lib/stb/stb")
    urllib.request.urlretrieve(url_stb_image, "lib/stb/stb/stb_image.h")
    urllib.request.urlretrieve(url_stb_image_write, "lib/stb/stb/stb_image_write.h")

    os.chdir("lib/chaiscript")
    call("git checkout release-6.x", shell=True)

    os.chdir("../glm")

    # The newest versions of glm don't seem to work (models don't draw)
    # TODO: find out why ?
    call("git checkout 8f39bb8", shell=True)