#!/usr/bin/ruby -w
# (C) Copyright 2006, 2011 Stephan Creutz

machine = `uname -m`.chomp

unistd = nil

if machine =~ /^i\d86/
  unistd = 'unistd_32.h'
elsif machine == 'x86_64'
  unistd = 'unistd_64.h'
end

if unistd.nil?
  $stderr.puts 'no suitible unistd header file found'
  exit 1
end

file = File.new('/usr/include/asm/' + unistd, 'r')
syscall_map = []
file.each_line do |line|
  if line =~ /#define __NR_/
    name, nr = line.split(/\s+/)[1, 2]
    name.sub!('__NR_', '')
    syscall_map[nr.to_i] = name if nr.to_i <= 259
  end
end
file.close

file = File.new('syscall_table.h', 'w')
file.puts "/*\n * automatically generated!!!\n * #{Time.now}\n */\n\n"
file.puts "#ifndef SYSCALL_TABLE_H\n#define SYSCALL_TABLE_H\n\n"
file.puts "static const char syscall_map[][#{syscall_map.length}] = {"
syscall_map.each { |name| file.puts "\t\"#{name}\"," }
file.puts "};\n\n"
file.puts "#define LAST_SYSCALL_NR #{syscall_map.length - 1}\n\n"
file.puts "#endif"
file.close
