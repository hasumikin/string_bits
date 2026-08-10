# frozen_string_literal: true

Gem::Specification.new do |s|
  s.name        = "string_bits"
  s.version     = "0.4.0"
  s.license     = "MIT"
  s.homepage    = "https://github.com/hasumikin/string_bits"
  s.summary     = "Bit-level operations on Ruby String"
  s.description = "Extends String with methods for reading, iterating, and manipulating individual bits in packed binary data."
  s.authors     = ["HASUMI Hitoshi"]
  s.email       = ["hasumikin@gmail.com"]
  s.files       = Dir["lib/**/*.rb", "ext/**/*.{rb,c,h}", "README.md", "docs/**/*.rb"]
  s.require_paths = ["lib"]
  s.extensions  = ["ext/string_bits/extconf.rb"]
  # Capped below 4.1: that is where the bit API this gem prototypes lands in
  # core, and the gem would then redefine the very String methods it proposed.
  # The bound is "4.1.dev" rather than "4.1" so that head builds are excluded
  # too -- RubyGems reports those as 4.1.0.dev, which sorts below 4.1 and would
  # otherwise satisfy the requirement, even though head is the first place the
  # core methods appear.
  s.required_ruby_version = [">= 3.0", "< 4.1.dev"]

  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "minitest"
  s.add_development_dependency "benchmark-driver"
  s.add_development_dependency "benchmark"
end
