Frozen verified artifact archive — do not edit.
  file        : tctool-unlock-0.2.0-rc2-2BC9BB1C.exe  (copy of tcyunlock\dist\tctool-unlock.exe)
  size        : 41483515
  sha256      : 2BC9BB1C78398BD2F09F551E11FC15C9D0712373A6F2A1AF9C9D652DB64FF392
  app version : 0.2.0-rc2 (protocol 1)
  source      : git 17047b4 (tcyunlock/src/Program.cs blob sha256 B26E8D7E63AB97C788F9EABADDB4A3A803A76CA6DABFDC1E4C8BC51E0D3BFCEC)
  why kept    : dist\tctool-unlock.exe is git-ignored (*.exe) and this binary is NOT
                bit-reproducible (see tcyunlock/README.md 1.4), so overwriting dist
                (rc3 build) would destroy the only copy of the verified rc2 artifact.
  scope       : NOT inside dist\ on purpose - the installer packs dist\*, and this
                copy must not end up in the package.

WARNING — DO NOT DELETE THIS FILE.
  This executable is the ONLY surviving copy of the frozen, verified v0.2.0-rc2
  artifact. It cannot be rebuilt bit-for-bit from source (measured: a faithful
  rebuild from the same commit differs; see tcyunlock/README.md 1.4), and the
  original path dist\tctool-unlock.exe is git-ignored (*.exe), so deleting this
  copy loses the verified binary permanently.
  If it needs to be kept long-term, attach it to a GitHub Release or add it to
  version control deliberately (the *.exe ignore rule currently prevents that).

KNOWN FUNCTIONAL DEFECT IN THIS BINARY (rc2; fixed in rc3) —
do NOT ship rc2 as "unlock works":
  InputInjector.Send injected the entire keystroke sequence in ONE SendInput call:
  keyDelayMs was spent while *assembling* the event array, never between
  injections. A real target therefore dropped characters (measured during rc3
  work: only 6 of 13 characters arrived), so the password would be typed
  truncated and the unlock would fail - looking like a secure-desktop problem.
  rc3 changed it to one SendInput call per keystroke with a real pause
  (measured 16/16 events). Full note in tcyunlock/README.md 6.
  This copy is retained for hash/provenance records, NOT as a recommended build.
