# lv5 homebrew formula file written by azu

require 'formula'

class Lv5 < Formula
  homepage 'https://github.com/Constellation/iv'
  url 'https://github.com/Constellation/iv/archive/1.0.1.zip'
  head 'https://github.com/Constellation/iv.git'
  sha1 '7520d877434f4c3bc2ac93a68ed778df6fe8b5ea'

  version '1.0.1'
  depends_on 'cmake' => :build
  depends_on 'bdw-gc'

  def install
    system "cmake", ".", *std_cmake_args
    system "make lv5"
    bin.install ['iv/lv5/lv5']
  end
end
