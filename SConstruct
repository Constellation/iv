# vim: fileencoding=utf-8
import platform
import sys
import os
from os.path import join, dirname, abspath
root_dir = dirname(File('SConstruct').rfile().abspath)
sys.path.append(join(root_dir, 'tools'))
from GenerateFile import TOOL_SUBST

def GetVariables():
  return Variables()

def Test(context):
  test_task = context.SConscript(
    'test/SConscript',
    variant_dir=join(root_dir, 'obj', 'test'),
    src=join(root_dir, 'test'),
    duplicate=False,
    exports="context"
  )
  context.AlwaysBuild(test_task)
  return test_task

def TestLv5(context, object_files, libs):
  test_task = context.SConscript(
    'test/lv5/SConscript',
    variant_dir=join(root_dir, 'obj', 'test', 'lv5'),
    src=join(root_dir, 'test', 'lv5'),
    duplicate=False,
    exports="context libs object_files"
  )
  context.AlwaysBuild(test_task)
  return test_task

def Lv5(context):
  lv5_task, lv5_objs, lv5_libs = context.SConscript(
    'iv/lv5/SConscript',
    variant_dir=join(root_dir, 'obj', 'lv5'),
    src=join(root_dir, 'iv', 'lv5'),
    duplicate=False,
    exports="context root_dir"
  )
  return lv5_task, lv5_objs, lv5_libs

def Main(context, deps):
  return context.SConscript(
    'iv/SConscript',
    variant_dir=join(root_dir, 'obj', 'iv'),
    src=join(root_dir, 'iv'),
    duplicate=False,
    exports='root_dir context deps'
  )

def Build():
  options = {}
  var = GetVariables()
  var.AddVariables(
    BoolVariable('debug', '', 0),
    BoolVariable('prof', '', 0),
    BoolVariable('gcov', '', 0),
    BoolVariable('clang', '', 0),
    BoolVariable('cxx0x', '', 0),
    BoolVariable('cxx1x', '', 0),
    BoolVariable('nosse', '', 0),
    BoolVariable('direct_threading', '', 0),
    BoolVariable('release', '', 0)
  )
  env = Environment(options=var, tools = ['default', TOOL_SUBST])
  env.VariantDir(join(root_dir, 'obj'), join(root_dir, 'iv'), 0)

  env.PrependENVPath('PATH', os.environ['PATH']) #especially MacPorts's /opt/local/bin

  if os.path.exists(join(root_dir, '.config')):
    env.SConscript(
      join(root_dir, '.config'),
      duplicate=False,
      exports='root_dir env options')

  if env['clang']:
    env.Replace(CXX='clang++', CC='clang')
    if os.environ.get('CLANG_COLOR'):
      env.Append(CCFLAGS=["-fcolor-diagnostics"])

  if not env.GetOption('clean'):
    conf = Configure(env)
#    if not conf.CheckFunc('snprintf'):
#      print 'snprintf must be provided'
#      Exit(1)
    conf.CheckLibWithHeader('m', 'cmath', 'cxx')
    env = conf.Finish()

  if env["CXXVERSION"] >= "4.4.3":
    # MacOSX etc... tr1/cmath problem
    env.Append(
        CXXFLAGS=["-ansi"])

  if options.get('cache'):
    env.CacheDir('cache')

  if env['prof']:
    env.Append(CCFLAGS=['-g3'])

  if env['gcov']:
    env.Append(
      CCFLAGS=["-coverage"],
      LINKFLAGS=["-coverage"]
    )

  if env['cxx0x'] or env['cxx1x']:
    env.Append(CXXFLAGS=["-std=c++0x"])
    if env['CC'] == 'clang':
      # use libc++
      env.Append(CXXFLAGS=["-stdlib=libc++"])
      env.Append(LIBS=["c++"])

  if not env['nosse']:
    env.Append(
        CCFLAGS=["-msse2"],
        CPPDEFINES=["IV_USE_SSE"])
    if env["CC"] == "gcc":
      env.Append(CCFLAGS="-mfpmath=sse")

  if env['debug']:
    # -Werror is defined in debug mode only
    env.Append(CCFLAGS=["-g3", "-Werror"])
  else:
    env.Append(
        CCFLAGS=["-O3", "-fomit-frame-pointer"],
        CPPDEFINES=["NDEBUG"])

  if env['direct_threading']:
    env.Append(CPPDEFINES="IV_USE_DIRECT_THREADED_CODE")
  else:
    env.Append(CCFLAGS="-pedantic")

  # XCode 4.3 clang allow this option
  env.Append(CCFLAGS="-fno-operator-names")

  env.Append(
    CCFLAGS=[
      "-Wall", "-Wextra", '-pipe',
      "-Wno-unused-parameter", "-Wwrite-strings", "-Wreturn-type", "-Wpointer-arith",
      "-Wwrite-strings", "-Wno-long-long", "-Wno-missing-field-initializers"],
    CPPPATH=root_dir,
    CPPDEFINES=[
      "__STDC_LIMIT_MACROS",
      "__STDC_CONSTANT_MACROS"],
    LIBPATH=["/usr/lib"])
  env['ENV']['GTEST_COLOR'] = os.environ.get('GTEST_COLOR')
  env['ENV']['HOME'] = os.environ.get('HOME')
  env.Replace(YACC='bison', YACCFLAGS='--name-prefix=yy_loaddict_')
  Help(var.GenerateHelpText(env))

  lv5_prog, lv5_objs, lv5_libs = Lv5(env)
  env.Alias('lv5', [lv5_prog])

  test_prog = Test(env)
  test_lv5_prog = TestLv5(env, lv5_objs, lv5_libs)
  test_alias = env.Alias('test', test_prog, test_prog[0].abspath)
  test_lv5_alias = env.Alias('testlv5', test_lv5_prog, test_lv5_prog[0].abspath)
  env.Default('lv5')

Build()
