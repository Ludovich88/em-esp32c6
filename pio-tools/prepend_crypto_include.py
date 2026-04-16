# Robonomics / rweather Crypto: на Windows #include <Ed25519.h> совпадает с
# tasmota/include/ed25519.h (регистр путей). Ставим Crypto в начало CPPPATH.
Import("env")
import os
libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
pioenv = env["PIOENV"]
crypto = os.path.join(libdeps, pioenv, "Crypto")
if os.path.isdir(crypto):
    env.Prepend(CPPPATH=[crypto])
