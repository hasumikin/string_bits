# frozen_string_literal: true

Gem::Specification.new do |s|
  s.name        = "string_bits"
  s.version     = "0.1.0"
  s.license     = "MIT"
  s.homepage    = "https://github.com/hasumikin/string_bits"
  s.summary     = "Bit-level operations on Ruby String"
  s.description = "Extends String with methods for reading, iterating, and manipulating individual bits in packed binary data."
  s.authors     = ["HASUMI Hitoshi"]
  s.email       = ["hasumikin@gmail.com"]
  s.files       = Dir["lib/**/*.rb", "ext/**/*.{rb,c,h}", "README.md", "docs/**/*.rb"]
  s.require_paths = ["lib"]
  s.extensions  = ["ext/string_bits/extconf.rb"]
  s.required_ruby_version = ">= 3.0"

  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "minitest"
  s.add_development_dependency "benchmark-driver"
  s.add_development_dependency "benchmark"
end
