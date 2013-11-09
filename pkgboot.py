
import platform
import os
import sys
import shutil
import subprocess
import shlex

try:
    from SCons.Script import *
except ImportError:
    pass

def run_test(target, source, env):
# Runs a single unit test and checks that the return code is 0
    try:
        subprocess.check_call(source[0].abspath)
    except subprocess.CalledProcessError, e:
        return 1

class Package:
    """
    Defines a workspace for a package.  Automatically sets up all the usual SCons 
    stuff, including a precompiled header file.
    """
    defines = {}
    includes = []
    libs = []
    path = []
    lib_path = []
    major_version = '0'
    minor_version = '0'
    patch = '0'
    kind = 'lib'

    def __init__(self):

        (system, _, release, version, machine, proc) = platform.uname()
        self.name = self.__class__.__name__.lower()
        self.pch = '%s/Common.hpp' % self.name
        self.build_mode = ARGUMENTS.get('mode', 'debug')
        self.version = '.'.join((self.major_version, self.minor_version, self.patch))
        self.branch = os.popen('git rev-parse --abbrev-ref HEAD').read().strip()
        self.revision = os.popen('git rev-parse HEAD').read().strip()
        self.defines.update({
            'VERSION': self.version,
            'REVISION': self.revision,
            'BRANCH': self.branch,
        })
        self.includes.extend([
            'include',
            'src', 
            'C:\\WinBrew\\include',
        ])
        self.lib_path.extend([
            'C:\\WinBrew\\lib',
        ])
        self.path.extend([
            os.environ['PATH'],
            'C:\\WinBrew\\lib', 
            'C:\\WinBrew\\bin', 
        ])

        self.env = Environment(CPPPATH=['build/src'])
        self.env.Append(ENV=os.environ)
        self.env.VariantDir('build/src', 'src', duplicate=0)
        self.env.VariantDir('build/test', 'test', duplicate=0)

        self.env.Append(CPPDEFINES=self.defines)
        self.env.Append(CPPPATH=self.includes)
        self.env.Append(LIBPATH=self.lib_path)
        self.env.Append(LIBS=self.libs)

        if self.env['PLATFORM'] == 'win32':
            self.env.Append(CXXFLAGS='/MT /EHsc /Zi /Gm /FS')
            self.env.Append(CXXFLAGS='/Fpbuild/Common.pch')
            self.env.Append(LINKFLAGS='/DEBUG')
            self.env.Append(CXXFLAGS='/Yu%s' % self.pch)

            pchenv=self.env.Clone()
            pchenv.Append(CXXFLAGS='/Yc%s' % self.pch)
            pch=pchenv.StaticObject('build/src/Common', 'build/src/Common.cpp')

        src = self.env.Glob('build/src/**.cpp')+self.env.Glob('build/src/**.c')+self.env.Glob('build/src/**.asm')
        src = filter(lambda x: 'Common.cpp' not in x.name, src)
        self.env.Depends(src, pch) # Wait for pch to build

        self.lib = self.env.StaticLibrary('lib/%s' % self.name, (src, pch))
        if self.kind == 'bin':
            main = self.env.Glob('Main.cpp')
            self.program = self.env.Program('bin/%s' % self.name, (self.lib, main))

        self.env.Append(BUILDERS={'Test': Builder(action=run_test)})
        self.tests = []
        for test in self.env.Glob('build/test/**.cpp'):
            self.env.Depends(test, pch)
            name = test.name.replace('.cpp', '')
            prog = self.env.Program('bin/test/%s' % name, (test, self.lib, pch))
            if 'check' in COMMAND_LINE_TARGETS:
                self.tests.append(self.env.Test(name, prog))
        if 'check' in COMMAND_LINE_TARGETS:
            self.env.Alias('check', self.tests)
        
        self.build()

    def build(self):
        pass
        
        






