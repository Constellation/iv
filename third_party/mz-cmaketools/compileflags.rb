#!/usr/bin/env ruby

# Searches for a flags.make in a CMake build tree and prints the compile flags.

def search_dir(dir, &block)
    Dir.foreach(dir) do |filename|
        next if (filename == ".") || (filename == "..")
        path ="#{dir}/#{filename}"
        if File.directory?(path)
            search_dir(path, &block)
        else
            search_file(path, &block)
        end
    end
end

def search_file(filename)
    return if File.basename(filename) != "flags.make"

    File.open(filename) do |io|
        io.read.scan(/[a-zA-Z]+_(?:FLAGS|DEFINES)\s*=\s*(.*)$/) do |match|
            yield(match.first.split(/\s+/))
        end
    end
end

root = ARGV.empty? ? Dir.pwd : ARGV[0]
params = to_enum(:search_dir, root).reduce { |a, b| a | b }
puts params