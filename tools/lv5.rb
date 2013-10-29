# lv5 homebrew formula file written by azu

require 'formula'

class Lv5 < Formula
  homepage 'https://github.com/Constellation/iv'
  url 'https://github.com/Constellation/iv/archive/69f89912a7c9160da1484347253015af782f9ac8.zip'
  head 'https://github.com/Constellation/iv.git'
  sha1 '7ea9bdd6394ac49d6293d5be34c7aa9af5d52067'

  version '1.0.0'
  depends_on 'cmake' => :build
  depends_on 'bdw-gc'

  def install
    system "cmake", ".", *std_cmake_args
    system "make lv5"
    bin.install ['iv/lv5/lv5']
  end
end
