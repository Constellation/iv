# -*- coding: utf-8 -*-

class String
  def indent n
    if n >= 0
      gsub(/^/, ' ' * n)
    else
      gsub(/^ {0,#{-n}}/, "")
    end
  end
end

module IV
  module Phonic
    # TODO(Constellation) check encoding
    def self.parse_to_ast src
      AST::Program.new parse(src), src
    end
    module AST
      class Node < Object
        def initialize parent, node
          @parent = parent
          @program = parent.program
          @begin = node[:begin]
          @end = node[:end]
        end

        def source
          @program.source
        end

        def program
          @program
        end

        def begin_position
          @begin
        end

        def end_position
          @end
        end
      end

      class Program < Node
        def initialize array, source
          @source = source
          @body = array.map{|stmt| Statement.as self, stmt }
        end

        def program
          self
        end

        def source
        end

        def to_source lv=0
          @body.map {|stmt|
            stmt.to_source lv
          }.join('\n').indent(lv)
        end
      end

      class Statement < Node
        def self.as parent, stmt
          return StatementType2Class[stmt[:type]].new parent, stmt
        end
      end

      class Block < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = stmt[:body].map{|item| Statement.as self, item }
        end

        def to_source lv
          "{\n#{@body.map{|stmt| stmt.to_source lv }.join('\n')}\n}".indent(lv)
        end
      end

      class FunctionStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = FunctionLiteral.new self, stmt[:body]
        end
      end

      class FunctionDeclaration < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = FunctionLiteral.new self, stmt[:body]
        end
      end

      class VariableStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = stmt[:body].map{|decl| Declaration.new self, decl }
          @const = stmt[:const]
        end

        def to_source lv
          "#{@const ? 'const' : 'var' } #{ @body.map{|decl| decl.to_source lv }.join(', ') };".indent(lv)
        end
      end

      class Declaration < Node
        def initialize parent, stmt
          super parent, stmt
          @name = Identifier.new self, stmt[:name]
          if stmt[:expr]
            @expr = Expression.as self, stmt[:expr]
          else
            @expr = nil
          end
        end
        def to_source lv
          if @expr
            "#{@name.to_source lv} = #{@expr.to_source lv}"
          else
            "#{@name.to_source lv}"
          end
        end
      end

      class EmptyStatement < Statement
        def initialize parent, stmt
          super parent, stmt
        end
      end

      class IfStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @cond = Expression.as self, stmt[:cond]
          @then = Statement.as self, stmt[:then]
          if stmt[:else]
            @else = Statement.as self, stmt[:else]
          else
            @else = nil
          end
        end
      end

      class DoWhileStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @cond = Expression.as self, stmt[:cond]
          @body = Statement.as self, stmt[:body]
        end
      end

      class WhileStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @cond = Expression.as self, stmt[:cond]
          @body = Statement.as self, stmt[:body]
        end
      end

      class ForStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          if stmt[:init]
            @init = Statement.as self, stmt[:init]
          else
            @init = nil
          end
          if stmt[:cond]
            @cond = Expression.as self, stmt[:cond]
          else
            @cond = nil
          end
          if stmt[:next]
            @next = Statement.as self, stmt[:next]
          else
            @next = nil
          end
          @body = Statement.as self, stmt[:body]
        end
      end

      class ForInStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @each = Statement.as self, stmt[:each]
          @enum = Expression.as self, stmt[:enum]
          @body = Statement.as self, stmt[:body]
        end
      end

      class ContinueStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          if stmt[:label]
            @label = Identifier.new self, stmt[:label]
          else
            @label = nil
          end
        end
      end

      class BreakStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          if stmt[:label]
            @label = Identifier.new self, stmt[:label]
          else
            @label = nil
          end
        end
      end

      class ReturnStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          if stmt[:expr]
            @expr = Expression.as self, stmt[:expr]
          else
            @expr = nil
          end
        end
      end

      class WithStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @context = Expression.as self, stmt[:context]
          @body = Statement.as self, stmt[:body]
        end
      end

      class LabelledStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @label = Identifier.new self, stmt[:label]
          @body = Statement.as self, stmt[:body]
        end
      end

      class SwitchStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @cond = Expression.as self, stmt[:cond]
          @clauses = stmt[:clauses].map{|clause| CaseClause.new self, clause }
        end
      end

      class CaseClause < Node
        def initialize parent, clause
          super parent, clause
          @kind = clause[:kind]
          if @kind == :Case
            @expr = Expression.as self, clause[:expr]
          else
            @expr = nil
          end
          @body = clause[:body].map{|stmt| Statement.as self, stmt }
        end
      end

      class ThrowStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @expr = Expression.as self, stmt[:expr]
        end
      end

      class TryStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = Statement.as self, stmt[:body]
          if stmt[:catch_name]
            @catch_name = Identifier.new self, stmt[:catch_name]
            @catch_block = Block.new self, stmt[:catch_block]
          else
            @catch_name = nil
            @catch_block = nil
          end
          if stmt[:finally_block]
            @finally_block = Block.new self, stmt[:finally_block]
          else
            @finally_block = nil
          end
        end
      end

      class DebuggerStatement < Statement
        def initialize parent, stmt
          super parent, stmt
        end
      end

      class ExpressionStatement < Statement
        def initialize parent, stmt
          super parent, stmt
          @body = Expression.as self, stmt[:body]
        end
      end

      class Expression < Node
        def self.as parent, expr
          return ExpressionType2Class[expr[:type]].new parent, expr
        end
      end

      class Assignment < Expression
        def initialize parent, expr
          super parent, expr
          @op = expr[:op]
          @left = Expression.as self, expr[:left]
          @right = Expression.as self, expr[:right]
        end
      end

      class BinaryOperation < Expression
        def initialize parent, expr
          super parent, expr
          @op = expr[:op]
          @left = Expression.as self, expr[:left]
          @right = Expression.as self, expr[:right]
        end
      end

      class ConditionalExpression < Expression
        def initialize parent, expr
          super parent, expr
          @cond = Expression.as self, expr[:cond]
          @left = Expression.as self, expr[:left]
          @right = Expression.as self, expr[:right]
        end
      end

      class UnaryOperation < Expression
        def initialize parent, expr
          super parent, expr
          @op = expr[:op]
          @expr = Expression.as self, expr[:expr]
        end
      end

      class PostfixExpression < Expression
        def initialize parent, expr
          super parent, expr
          @op = expr[:op]
          @expr = Expression.as self, expr[:expr]
        end
      end

      class Literal < Expression
      end

      class StringLiteral < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value]
        end
      end

      class NumberLiteral < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value]
        end
      end

      class Identifier < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value]
          @kind = expr[:kind]
        end

        def to_source lv
          if @kind == :STRING || @kind == :NUMBER
            source.slice(begin_position, end_position)
          else
            @value
          end
        end
      end

      class ThisLiteral < Literal
        def initialize parent, expr
          super parent, expr
        end
      end

      class NullLiteral < Literal
        def initialize parent, expr
          super parent, expr
        end
      end

      class TrueLiteral < Literal
        def initialize parent, expr
          super parent, expr
        end
      end

      class FalseLiteral < Literal
        def initialize parent, expr
          super parent, expr
        end
      end

      class RegExpLiteral < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value]
          @flags = expr[:flags]
        end
      end

      class ArrayLiteral < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value].map{|item| Expression.as self, item }
        end
      end

      class ArrayHole < Expression
        def initialize parent, expr
          super parent, expr
        end
      end

      class ObjectLiteral < Literal
        def initialize parent, expr
          super parent, expr
          @value = expr[:value].map{|item| Property.as self, item }
        end
      end

      class Property < Node
        def self.as parent, expr
          return PropertyType2Class[expr[:kind]].new parent, expr
        end
        def initialize parent, prop
          super parent, prop
          @key = Identifier.new self, prop[:key]
          @value = Expression.as self, prop[:value]
        end
      end

      class DataProperty < Property
        def initialize parent, prop
          super parent, prop
        end
      end

      class GetterProperty < Property
        def initialize parent, prop
          super parent, prop
        end
      end

      class SetterProperty < Property
        def initialize parent, prop
          super parent, prop
        end
      end

      PropertyType2Class = {
        :Data => DataProperty,
        :Getter => GetterProperty,
        :Setter => SetterProperty
      }

      class FunctionLiteral < Literal
        def initialize parent, expr
          super parent, expr
          if expr[:name]
            @name = Identifier.new self, expr[:name]
          else
            @name = nil
          end
          @params = expr[:params].map {|param| Identifier.new self, param }
          @body = expr[:body].map {|stmt| Statement.as self, stmt }
        end
      end

      class IndexAccess < Expression
        def initialize parent, expr
          super parent, expr
          @target = Expression.as self, expr[:target]
          @key = Expression.as self, expr[:key]
        end

        def to_source
          "#{@target.to_source}[#{@key.to_source}]"
        end
      end

      class IdentifierAccess < Expression
        def initialize parent, expr
          super parent, expr
          @target = Expression.as self, expr[:target]
          @key = Identifier.new self, expr[:key]
        end

        def to_source
          "#{@target.to_source}.#{@key.to_source}"
        end
      end

      class FunctionCall < Expression
        def initialize parent, expr
          super parent, expr
          @target = Expression.as self, expr[:target]
          @args = expr[:args].map{|arg| Expression.as self, arg }
        end

        def to_source
          "#{@target.to_source}(#{args.map{|arg| arg.to_source }.join(', ')})"
        end
      end

      class ConstructorCall < Expression
        def initialize parent, expr
          super parent, expr
          @target = Expression.as self, expr[:target]
          @args = expr[:args].map{|arg| Expression.as self, arg }
        end

        def to_source
          "new #{@target.to_source}(#{args.map{|arg| arg.to_source }.join(', ')})"
        end
      end

      StatementType2Class = {
          :Block => Block,
          :FunctionStatement => FunctionStatement,
          :FunctionDeclaration => FunctionDeclaration,
          :VariableStatement => VariableStatement,
          :EmptyStatement => EmptyStatement,
          :IfStatement => IfStatement,
          :DoWhileStatement => DoWhileStatement,
          :WhileStatement => WhileStatement,
          :ForStatement => ForStatement,
          :ForInStatement => ForInStatement,
          :ContinueStatement => ContinueStatement,
          :BreakStatement => BreakStatement,
          :ReturnStatement => ReturnStatement,
          :WithStatement => WithStatement,
          :LabelledStatement => LabelledStatement,
          :SwitchStatement => SwitchStatement,
          :ThrowStatement => ThrowStatement,
          :TryStatement => TryStatement,
          :DebuggerStatement => DebuggerStatement,
          :ExpressionStatement => ExpressionStatement
        }

      ExpressionType2Class = {
          :Assignment => Assignment,
          :BinaryOperation => BinaryOperation,
          :ConditionalExpression => ConditionalExpression,
          :UnaryOperation => UnaryOperation,
          :PostfixExpression => PostfixExpression,
          :StringLiteral => StringLiteral,
          :NumberLiteral => NumberLiteral,
          :Identifier => Identifier,
          :ThisLiteral => ThisLiteral,
          :NullLiteral => NullLiteral,
          :TrueLiteral => TrueLiteral,
          :FalseLiteral => FalseLiteral,
          :RegExpLiteral => RegExpLiteral,
          :ObjectLiteral => ObjectLiteral,
          :ArrayLiteral => ArrayLiteral,
          :ArrayHole => ArrayHole,
          :FunctionLiteral => FunctionLiteral,
          :IndexAccess => IndexAccess,
          :IdentifierAccess => IdentifierAccess,
          :FunctionCall => FunctionCall,
          :ConstructorCall => ConstructorCall
      }
    end
  end
end
