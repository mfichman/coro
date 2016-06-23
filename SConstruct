
import pkgboot

class Coro(pkgboot.Package):
    defines = {}
    includes = [
        '/usr/local/opt/openssl/include',
    ]
    libs = [
        pkgboot.Lib('ws2_32', 'win32'),
        pkgboot.Lib('advapi32', 'win32'),
        pkgboot.Lib('user32', 'win32'),
        pkgboot.Lib('gdi32', 'win32'),
        pkgboot.Lib('libeay', 'win32'),
        pkgboot.Lib('ssleay', 'win32'),
        pkgboot.Lib('ssl', 'darwin'),
        pkgboot.Lib('crypto', 'darwin'),
    ]
    major_version = '0'
    minor_version = '0'
    patch = '0'

Coro()
