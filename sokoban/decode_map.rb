#!/usr/bin/env ruby

# Structure of the binary data file (like 'sokoban.dat')
#
# Sequence of 306 bytes (per level)
# - 1 unsigned 16 bit in little endian: player position
# - 304 unsigned 8 bit: array of 16 rows and 19 columns
#   - 00h	 empty
#   - 01h	 wall
#   - 03h	 target position
#   - 14h	 box
#   - 17h	 box on target position

if ARGV.length != 1 then
  puts "usage: #{$0} 'sokoban.dat'"
  exit
end

file = File.open ARGV[0]

nr = 1

while map = file.read(306)
  break if map.length != 306

  puts
  puts "Map #{nr}"
  puts

  array = map.unpack "SC304"
  # S: unsigned 16bit in little-endian order
  # C: unsigned 8bit

  player_pos = array[0]
  array.shift

  pos = 0
  array.each_index do | i |
    puts if i % 19 == 0

    if pos == player_pos then
      print "@@"
    else
      case array[i]
      when 0x00
	print "  "
      when 0x01
	print "##"
      when 0x03
	print ".."
      when 0x14
	print "[]"
      when 0x17
	print "{}"
      else
	print "??"
      end
    end

    pos += 1
  end
  puts

  nr += 1
end

file.close
