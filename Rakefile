# frozen_string_literal: true

require "rake/extensiontask"
require "rake/testtask"

spec = Gem::Specification.load("string_bits.gemspec")

Rake::ExtensionTask.new("string_bits", spec) do |t|
  t.ext_dir = "ext/string_bits"
  t.lib_dir = "lib/string_bits"
end

Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.test_files = FileList["test/test_*.rb"]
end

desc "Run benchmarks (baseline vs string_bits, with and without YJIT). Optionally specify a name: rake benchmark[bitmap_font]"
task :benchmark, [:name] => :compile do |_, args|
  yjit_ok = system(RbConfig.ruby, "--yjit", "-e", "exit 0",
                   out: IO::NULL, err: IO::NULL)

  arrow_ok = system(RbConfig.ruby, "-e", "require 'arrow'",
                    out: IO::NULL, err: IO::NULL)

  runs = [["ruby", RbConfig.ruby]]
  runs << ["yjit", "#{RbConfig.ruby} --yjit"] if yjit_ok

  yamls = if args[:name]
    ["benchmark/#{args[:name]}.yaml"]
  else
    Dir["benchmark/*.yaml"].sort
  end

  yamls.each do |yaml|
    next if File.basename(yaml).start_with?("arrow_red_arrow") && !arrow_ok
    puts "\n=== #{File.basename(yaml, '.yaml').upcase} ==="
    runs.each do |label, cmd|
      sh "RUBYLIB=#{File.expand_path('lib')} bundle exec benchmark-driver #{yaml} " \
         "--executables '#{label}::#{cmd}' --output faster"
    end
  end

  puts
  sh "#{RbConfig.ruby} benchmark/allocation.rb" unless args[:name]
end

namespace :test do
  desc "Run the test suite under a 32-bit (i386/ILP32) Ruby via Docker. " \
       "Override with IMAGE=... and RAKE_ARGS=... (e.g. RAKE_ARGS='test TEST=test/test_bit_count.rb')"
  task :i386 do
    image     = ENV.fetch("IMAGE", "i386/ruby:3.3-bookworm")
    rake_args = ENV.fetch("RAKE_ARGS", "compile test")
    repo      = __dir__
    # The repo is mounted read-only and copied into the container, so the build
    # (Makefile, *.o, the i386 lib/string_bits/string_bits.so, .bundle) stays
    # inside the container and the host x86_64 build is left untouched.
    sh "docker", "run", "--rm", "--platform", "linux/386",
       "-v", "#{repo}:/work:ro",
       "-e", "RAKE_ARGS=#{rake_args}",
       image, "bash", "-c",
       "set -e; cp -a /work /build; cd /build; " \
       "bundle install --quiet; exec bundle exec rake $RAKE_ARGS"
  end
end

task default: [:compile, :test]
