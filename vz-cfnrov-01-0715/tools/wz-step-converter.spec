# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['step_converter.py'],
    pathex=[],
    binaries=[('C:\\Users\\13471\\Desktop\\vz-cfnrov-01-0715\\tools\\step_runtime\\OCP.cp312-win_amd64.pyd', '.'), ('C:\\Users\\13471\\Desktop\\vz-cfnrov-01-0715\\tools\\step_runtime\\*.dll', '.')],
    datas=[],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='wz-step-converter',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
