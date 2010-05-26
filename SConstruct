# vim: fileencoding=utf-8
import platform


def GetVariables():
  return Variables()

def Build():
  var = GetVariables()
  var.AddVariables(
    BoolVariable('debug', '', 0),
    BoolVariable('gprof', '', 0)
  )
  env = Environment(options=var)
  if env['gprof']:
    env.Append(
      CCFLAGS=["-pg"],
      LINKFLAGS=["-pg"]
    )
  if env['debug']:
    env.Append(
      CCFLAGS=["-g3"]
    )
  env.Append(
    CCFLAGS=["-Wall", "-W", "-Werror", "-Wno-unused-parameter"],
    CCDEFINES=[],
    LIBPATH=["/usr/lib"])
  env.Replace(YACC='bison', YACCFLAGS='--name-prefix=yy_loaddict_')
  Help(var.GenerateHelpText(env))
  env.ParseConfig('icu-config --cppflags --ldflags --ldflags-icuio')
  env.ParseConfig('llvm-config all --cppflags --ldflags --libs')
  env.Program('js', [ 'src/lexer.cc',
                      'src/main.cc',
                      'src/parser.cc',
                      'src/ast-visitor.cc',
                      'src/ast-factory.cc',
                      'src/ast.cc',
                      'src/token.cc',
                      'src/utils.cc',
                      'src/alloc.cc'])

Build()

