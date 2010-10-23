# vim: fileencoding=utf-8
import platform
import sys
import os
from os.path import join, dirname, abspath

root_dir = dirname(File('SConstruct').rfile().abspath)

def GetVariables():
  return Variables()

def Test(context, object_files):
  test_task = context.SConscript(
    'test/SConscript',
    variant_dir=join(root_dir, 'obj', 'test'),
    duplicate=False,
    exports="context object_files"
  )
  context.AlwaysBuild(test_task)
  return test_task

def Lv5(context, object_files):
  lv5_task = context.SConscript(
    'src/lv5/SConscript',
    variant_dir=join(root_dir, 'obj', 'lv5'),
    duplicate=False,
    exports="context object_files root_dir"
  )
  return lv5_task

def Main(context):
  return context.SConscript(
    'src/SConscript',
    variant_dir=join(root_dir, 'obj', 'src'),
    duplicate=False,
    exports='root_dir context'
  )

def Build():
  options = {}
  var = GetVariables()
  var.AddVariables(
    BoolVariable('debug', '', 0),
    BoolVariable('gprof', '', 0)
  )
  env = Environment(options=var)
  env.VariantDir(join(root_dir, 'obj'), join(root_dir, 'src'), 0)

  if os.path.exists(join(root_dir, '.config')):
    env.SConscript(
      join(root_dir, '.config'),
      duplicate=False,
      exports='root_dir env options')

  if env['gprof']:
    env.Append(
      CCFLAGS=["-pg"],
      LINKFLAGS=["-pg"]
    )

#  if env['debug']:
#    env.Append(
#      CCFLAGS=["-g3"]
#    )
#  else:
#    env.Append(
#      #CPPDEFINES=["NDEBUG"]
#    )

  env.Append(CCFLAGS=["-g"])
  env.Append(
    CCFLAGS=["-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wwrite-strings"],
    CPPPATH=[join(root_dir, "src")],
    CPPDEFINES=[
      "_GNU_SOURCE",
      "__STDC_LIMIT_MACROS",
      "__STDC_CONSTANT_MACROS"],
    LIBPATH=["/usr/lib"])
  env['ENV']['GTEST_COLOR'] = os.environ.get('GTEST_COLOR')
  env['ENV']['HOME'] = os.environ.get('HOME')
  if options.get('cache'):
    env.CacheDir('cache')
  env.Replace(YACC='bison', YACCFLAGS='--name-prefix=yy_loaddict_')
  Help(var.GenerateHelpText(env))
  env.ParseConfig('icu-config --cxxflags --cppflags --ldflags --ldflags-icuio')
  # env.ParseConfig('llvm-config all --ldflags --libs')

  (object_files, main_prog) = Main(env)
  test_prog = Test(env, object_files)
  env.Alias('main', [main_prog])
  test_alias = env.Alias('test', test_prog, test_prog[0].abspath)
  lv5_prog = Lv5(env, object_files)
  env.Alias('lv5', [lv5_prog])
  env.Default('lv5')

Build()
