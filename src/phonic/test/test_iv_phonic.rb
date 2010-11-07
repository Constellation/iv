# -*- coding: utf-8 -*-
require "test/unit"
require "iv/phonic"
require "pp"

class TestPhonic < Test::Unit::TestCase
  def test_phonic
    assert(Object.const_defined? :IV)
    assert(IV.const_defined? :Phonic)
  end
  def test_parse
    assert_respond_to(IV::Phonic, :parse)
    assert_raise(TypeError) {
      IV::Phonic::parse(100);
    }
    assert_raise(TypeError) {
      IV::Phonic::parse(/TEST/);
    }
    assert_raise(TypeError) {
      IV::Phonic::parse(IV);
    }
    assert(IV::Phonic::parse("FILE"))
    assert(IV::Phonic::parse("T"))
    assert(IV::Phonic::parse("var test = \"おはようございます\";"))
    assert_raise(IV::Phonic::ParseError) {
      IV::Phonic::parse("var test = var;")
    }
    assert_nothing_raised {
      IV::Phonic::parse("var test = /test/;")
    }
  end
end
