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

def GTestLib(context):
  return context.SConscript(
    join(root_dir, 'test', 'gtest', 'SConscript'),
    variant_dir=join(root_dir, 'obj', 'test', 'gtest'),
    src=join(root_dir, 'iv', 'test', 'gtest'),
    duplicate=False,
    exports="context root_dir"
  )

def Test(context, gtest):
  test_task = context.SConscript(
    'test/SConscript',
    variant_dir=join(root_dir, 'obj', 'test'),
    src=join(root_dir, 'test'),
    duplicate=False,
    exports="context root_dir gtest"
  )
  context.AlwaysBuild(test_task)
  return test_task

def TestLv5(context, gtest, object_files, libs):
  test_task = context.SConscript(
    'test/lv5/SConscript',
    variant_dir=join(root_dir, 'obj', 'test', 'lv5'),
    src=join(root_dir, 'test', 'lv5'),
    duplicate=False,
    exports="context root_dir libs object_files gtest"
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

def Build():
  options = {}
  var = GetVariables()
  var.AddVariables(
    BoolVariable('debug', '', 0),
    BoolVariable('debug_iterator', '', 0),
    BoolVariable('prof', '', 0),
    BoolVariable('gcov', '', 0),
    BoolVariable('clang', '', 0),
    BoolVariable('cxx0x', '', 0),
    EnumVariable('sse', 'sse option', 'no',
                 allowed_values=('no', 'sse', 'sse2', 'sse3', 'sse4', 'sse4.1', 'sse4.2'),
                 map={}, ignorecase=2),
    BoolVariable('disable_jit', '', 0),
    BoolVariable('cxx11', '', 0),
    BoolVariable('direct_threading', '', 0),
    BoolVariable('release', '', 0),
    BoolVariable('i18n', '', 0)
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
    env.Append(CPPFLAGS=['-ferror-limit=1000']);

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

  if env['sse'] != 'no':
    env.Append(CCFLAGS=['-m' + env['sse']])

  if env['disable_jit'] == 'yes':
    env.Append(CPPDEFINES=['IV_DISABLE_JIT'])

  if env['gcov']:
    env.Append(
      CCFLAGS=["-coverage"],
      LINKFLAGS=["-coverage"]
    )

  if env['cxx0x'] or env['cxx11']:
    if env['CC'] == 'clang':
      # use libc++
      env.Append(CXXFLAGS=["-std=c++11"])
      env.Append(CXXFLAGS=["-stdlib=libc++"])
      env.Append(LIBS=["c++"])
    else:
      env.Append(CXXFLAGS=["-std=c++0x"])

  if env['debug']:
    # -Werror is defined in debug mode only
    env.Append(CCFLAGS=["-g3"])
    # env.Append(CCFLAGS=["-Werror"])
  else:
    env.Append(CCFLAGS=["-fomit-frame-pointer"], CPPDEFINES=["NDEBUG"])
    if env['clang']:
      env.Append(CCFLAGS=["-O4"])
    else:
      env.Append(CCFLAGS=["-O3"])

  if env['debug_iterator']:
    env.Append(CPPDEFINES=['_GLIBCXX_DEBUG'])

  if env['direct_threading']:
    env.Append(CPPDEFINES="IV_USE_DIRECT_THREADED_CODE")
  else:
    env.Append(CCFLAGS="-pedantic")

  if env['i18n']:
    env.Append(CPPDEFINES=["IV_ENABLE_I18N"])
    env.ParseConfig('icu-config --ldflags --cppflags')

  # XCode 4.3 clang allow this option
  env.Append(CCFLAGS=["-fno-operator-names"])

  # old shared_ptr require RTTI
  # env.Append(CCFLAGS=["-fno-rtti"])

  env.Append(
    CCFLAGS=[
      "-Wall", "-Wextra", '-pipe',
      "-Wno-unused-parameter", "-Wwrite-strings", "-Wreturn-type", "-Wpointer-arith",
      "-Wno-unused-variable",
      "-Wwrite-strings", "-Wno-long-long", "-Wno-missing-field-initializers"],
    CPPPATH=[root_dir, "/usr/local/include"],
    CPPDEFINES=[
      "__STDC_LIMIT_MACROS",
      "__STDC_CONSTANT_MACROS"],
    LIBPATH=["/usr/lib", "/usr/local/lib"])
  env['ENV']['GTEST_COLOR'] = os.environ.get('GTEST_COLOR')
  env['ENV']['HOME'] = os.environ.get('HOME')
  env.Replace(YACC='bison', YACCFLAGS='--name-prefix=yy_loaddict_')
  Help(var.GenerateHelpText(env))

  lv5_prog, lv5_objs, lv5_libs = Lv5(env)
  env.Alias('lv5', [lv5_prog])

  gtest = GTestLib(env)
  test_prog = Test(env, gtest)
  test_lv5_prog = TestLv5(env, gtest, lv5_objs, lv5_libs)
  test_alias = env.Alias('test', test_prog, test_prog[0].abspath)
  test_lv5_alias = env.Alias('testlv5', test_lv5_prog, test_lv5_prog[0].abspath)
  env.Default('lv5')

Build()
