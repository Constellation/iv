# vim: fileencoding=utf-8
import platform


def GetVariables():
  return Variables()

def Build():
  var = GetVariables()
  env = Environment(options=var)
  env.Append(
    CCFLAGS=["-g"],
    CCDEFINES=[],
    LIBPATH=["/usr/lib"])
  env.Replace(YACC='bison', YACCFLAGS='--name-prefix=yy_loaddict_')
  Help(var.GenerateHelpText(env))
  env.ParseConfig('icu-config --cppflags --cxxflags --ldflags --ldflags-icuio')
  env.ParseConfig('llvm-config all --cppflags --cxxflags --ldflags --libs')
  env.Program('js', [ 'src/lexer.cc',
                      'src/main.cc',
                      'src/parser.cc',
                      'src/ast-factory.cc',
                      'src/ast.cc',
                      'src/token.cc',
                      'src/utils.cc',
                      'src/alloc.cc'])

Build()

